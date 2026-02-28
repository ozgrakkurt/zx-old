#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

ZXStatus zx_fuzz_setup(ZXErrStack *err_stack);
ZXStatus zx_fuzz_run(zx_u8 const *fuzz_input, zx_usize fuzz_input_len,
                     zx_usize *n_consumed, ZXErrStack *err_stack);

// global fuzz state for implementing the crash signal handler
// this kind of global state is needed because we can't pass arguments to the
// crash signal handler.
typedef struct ZXFuzzState {
  char const *fuzz_input;
  zx_usize fuzz_input_len;
  char const *crash_msg;
  zx_usize crash_msg_capacity;
  ZXErrStack err_stack;
  bool is_init;
  bool was_called;
} ZXFuzzState;

static ZXFuzzState ZX_FUZZ_STATE = {
    .fuzz_input = nullptr,
    .fuzz_input_len = 0,
    .crash_msg = nullptr,
    .crash_msg_capacity = 0,
    .err_stack = {0},
    .is_init = false,
    .was_called = false,
};

void crash_handler(int signum) {
  bool was_called = ZX_FUZZ_STATE.was_called;
  ZX_FUZZ_STATE.was_called = true;
  if (was_called) {
    zx_sys_exit(ZX_SYS_EXIT_FAILURE);
  }

  if ((ZX_FUZZ_STATE.fuzz_input == nullptr) ||
      (ZX_FUZZ_STATE.fuzz_input_len == 0) ||
      (ZX_FUZZ_STATE.crash_msg == nullptr) ||
      (ZX_FUZZ_STATE.crash_msg_capacity == 0) || (!ZX_FUZZ_STATE.is_init)) {
    char const *msg =
        "Global fuzz state isn't initialized properly. Crash handler can't run "
        "properly. Prematurely exiting the process now... Check code in "
        "file " __FILE__ " line " __LINE__ "\n";
    zx_usize n_written = 0;
    zx_sys_write(ZX_STDERR, msg, __builtin_strlen(msg), &n_written);
    (void)n_written;
    zx_sys_exit(ZX_SYS_EXIT_FAILURE);
  }

  ZXFmtBuf fmt_buf =
      ZXFmtBuf_init(ZX_FUZZ_STATE.crash_msg, ZX_FUZZ_STATE.crash_msg_capacity);

  char const *signame = "unknown";
  switch (signum) {
  case ZX_SYS_SIGFPE: {
    signame = "SIGFPE";
    break;
  }
  case ZX_SYS_SIGABRT: {
    signame = "SIGABRT";
    break;
  }
  case ZX_SYS_SIGSEGV: {
    signame = "SIGSEGV";
    break;
  }
  default: {
    (void)ZXFmtBuf_format_cstr(
        &fmt_buf, "received unexpected signum: ", &ZX_FUZZ_STATE.err_stack);
    (void)ZXFmtBuf_format_i64(&fmt_buf, (zx_i64){signum},
                              &ZX_FUZZ_STATE.err_stack);
    (void)ZXFmtBuf_format_cstr(&fmt_buf,
                               "aborting crash handler prematurely now...\n",
                               &ZX_FUZZ_STATE.err_stack);
    goto quit;
  }
  }

  // write file containing crashing fuzz input.
  // determine file name and path somehow.
  // input should be put into a static variable every time before a fuzz run.
  // maybe also write a "formatted" file by converting the raw fuzz input into
  // user readable string using a function given by fuzz target.

  (void)ZXFmtBuf_format_cstr(
      &fmt_buf, "Crashing because of signal name: ", &ZX_FUZZ_STATE.err_stack);
  (void)ZXFmtBuf_format_cstr(&fmt_buf, signame, &ZX_FUZZ_STATE.err_stack);
  (void)ZXFmtBuf_format_cstr(&fmt_buf,
                             " (signal code: ", &ZX_FUZZ_STATE.err_stack);
  (void)ZXFmtBuf_format_i64(&fmt_buf, (zx_i64){signum},
                            &ZX_FUZZ_STATE.err_stack);
  (void)ZXFmtBuf_format_cstr(&fmt_buf, ")\n", &ZX_FUZZ_STATE.err_stack);

quit:
  (void)ZXFmtBuf_write(&fmt_buf, ZX_SYS_STDERR, &err_stack);
  zx_sys_exit(ZX_SYS_EXIT_FAILURE);
}

int main(void) {
  ZX_ASSERT((zx_sys_sigaction(ZX_SYS_SIGFPE, crash_handler)));
  ZX_ASSERT((zx_sys_sigaction(ZX_SYS_SIGSEGV, crash_handler)));
  // register the abort handler last so all registration errors trigger the
  // default abort handler.
  ZX_ASSERT((zx_sys_sigaction(ZX_SYS_SIGABRT, crash_handler)));

  ZXErrStack err_stack = ZXErrStack_init();
  ZX_ERR_TRACE_ABORT((zx_fuzz_setup(&err_stack)), &err_stack);
  while (true) {
    zx_usize n_consumed = 0;
    ZX_ERR_TRACE_ABORT(zx_fuzz_run((zx_u8 const *)"anan",
                                   __builtin_strlen("anan"), &n_consumed,
                                   &err_stack),
                       &err_stack);
  }

  return 0;
}

#endif
