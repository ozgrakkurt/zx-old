#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_page_alloc.h>
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

  constexpr zx_usize TRACE_CAP = 1 << 12;
  void *trace_mem = zx_page_alloc_4kb(TRACE_CAP);

  ZXErrStack dummy_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(trace_mem, TRACE_CAP);
  char const *trace = nullptr;
  zx_usize trace_len = 0;
  ZXStatus const status =
      zx_err_trace_collect(&alloc, stack, &trace, &trace_len, &dummy_stack);
  ZX_ASSERT((status == ZXStatus_Ok));

  if (trace_len > 0) {
    zx_usize nw = 0;
    zx_usize const status = zx_sys_write(ZX_STDERR, trace, trace_len, &nw);
    (void)status;
  }

  zx_sys_abort();
}
