#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>

static inline zx_usize
zx_err_trace_format_entry_(char *buf, zx_usize buf_capacity,
                           ZXErrStackEntry_ const *entry) {
  zx_usize off = *offset;
  *offset = off;
}

// Formats the trace into buf and returns the number of bytes written
static inline zx_usize zx_err_format_(ZXErrStack const *err_stack, char *buf,
                                      zx_usize buf_capacity) {
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

[[noreturn]] void zx_err_abort_(ZXErrStack const *err_stack) {
  constexpr zx_usize BUF_CAP = 1 << 12;
  char buf[BUF_CAP] = {0};

  zx_usize const buf_len = zx_err_format_(err_stack, buf, BUF_CAP);

  zx_usize n_written = 0;
  zx_usize const status = zx_sys_write(ZX_STDERR, buf, buf_len, &n_written);
  (void)status;
  (void)n_written;

  zx_sys_abort();
}
