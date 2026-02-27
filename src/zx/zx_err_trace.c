#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_sys.h>

ZXStatus zx_err_trace_collect(ZXAlloc *alloc, ZXErrStack const *stack,
                              char const **trace, zx_usize *trace_len,
                              ZXErrStack *err_stack) {
  zx_usize const depth = stack->depth;
  char const *const *infos = stack->infos;
  zx_u16 const *info_lengths = stack->info_lengths;

  zx_usize total_len = 0;
  for (zx_u32 i = 0; i < depth; i++) {
    total_len += info_lengths[i];
  }

  constexpr char prefix[] = "Error Trace: \n";
  constexpr zx_usize prefix_len = sizeof(prefix) - 1;

  zx_usize const alloc_size = prefix_len + total_len + 1;

  char *buf = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alloc_size, (void **)&buf, err_stack)),
             err_stack);

  __builtin_memcpy(buf, prefix, prefix_len);

  zx_usize offset = prefix_len;
  for (zx_u32 i = 0; i < depth; i++) {
    zx_usize const info_len = info_lengths[i];
    char const *info = infos[i];
    __builtin_memcpy(&buf[offset], info, info_len);
    offset += info_len;
  }

  buf[offset] = '\0';

  ZX_ASSERT((offset + 1 == alloc_size));

  *trace = buf;
  *trace_len = offset;

  return ZXStatus_Ok;
}

// Outputs the trace to stderr and aborts the program
[[noreturn]] void zx_err_trace_abort(ZXErrStack const *stack) {
  ZX_ASSERT((stack != nullptr));

  char const *head = "Aborting with stack trace:\n";

  zx_usize head_nw = 0;
  zx_usize const head_status =
      zx_sys_write(ZX_STDERR, head, __builtin_strlen(head), &head_nw);
  // maybe should check nw == strlen here?
  if (ZX_SYS_STATUS_SUCCESS != head_status) {
    zx_sys_abort();
  }

  for (zx_u32 i = 0; i < stack->depth; i++) {
    char const *info = stack->infos[i];

    zx_usize nw = 0;
    zx_usize const status =
        zx_sys_write(ZX_STDERR, info, __builtin_strlen(info), &nw);
    // maybe should check nw == strlen here?
    if (ZX_SYS_STATUS_SUCCESS != status) {
      zx_sys_abort();
    }
  }

  zx_sys_abort();
}
