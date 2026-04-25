#ifndef ZX_PERF_EVENT_C_H
#define ZX_PERF_EVENT_C_H

#include <zx/zx_int.h>

extern "C" {

enum perf_type_id {
  PERF_TYPE_HARDWARE = 0,
  PERF_TYPE_SOFTWARE = 1,
  PERF_TYPE_TRACEPOINT = 2,
  PERF_TYPE_HW_CACHE = 3,
  PERF_TYPE_RAW = 4,
  PERF_TYPE_BREAKPOINT = 5,

  PERF_TYPE_MAX, /* non-ABI */
};

enum perf_hw_id {
  /*
   * Common hardware events, generalized by the kernel:
   */
  PERF_COUNT_HW_CPU_CYCLES = 0,
  PERF_COUNT_HW_INSTRUCTIONS = 1,
  PERF_COUNT_HW_CACHE_REFERENCES = 2,
  PERF_COUNT_HW_CACHE_MISSES = 3,
  PERF_COUNT_HW_BRANCH_INSTRUCTIONS = 4,
  PERF_COUNT_HW_BRANCH_MISSES = 5,
  PERF_COUNT_HW_BUS_CYCLES = 6,
  PERF_COUNT_HW_STALLED_CYCLES_FRONTEND = 7,
  PERF_COUNT_HW_STALLED_CYCLES_BACKEND = 8,
  PERF_COUNT_HW_REF_CPU_CYCLES = 9,

  PERF_COUNT_HW_MAX, /* non-ABI */
};

struct perf_event_mmap_page {
  zx_u32 version;        /* version number of this structure */
  zx_u32 compat_version; /* lowest version this is compat with */
  zx_u32 lock;           /* seqlock for synchronization */
  zx_u32 index;          /* hardware counter identifier */
  zx_i64 offset;         /* add to hardware counter value */
  zx_u64 time_enabled;   /* time event active */
  zx_u64 time_running;   /* time event on CPU */
  union {
    zx_u64 capabilities;
    struct {
      zx_u64 cap_bit0 : 1, cap_bit0_is_deprecated : 1, cap_user_rdpmc : 1,
          cap_user_time : 1, cap_user_time_zero : 1, cap_user_time_short : 1,
          cap_____res : 58;
    };
  };
  zx_u64 pmc_width;
  zx_u64 time_shift;
  zx_u32 time_mult;
  zx_u64 time_offset;
  zx_u64 __reserved[120]; /* Pad to 1 k */
  zx_u64 data_head;       /* head in the data section */
  zx_u64 data_tail;       /* user-space written tail */
  zx_u64 data_offset;     /* where the buffer starts */
  zx_u64 data_size;       /* data buffer size */
  zx_u64 aux_head;
  zx_u64 aux_tail;
  zx_u64 aux_offset;
  zx_u64 aux_size;
};

struct perf_event_attr {
  zx_u32 type;   /* Type of event */
  zx_u32 size;   /* Size of attribute structure */
  zx_u64 config; /* Type-specific configuration */

  union {
    zx_u64 sample_period; /* Period of sampling */
    zx_u64 sample_freq;   /* Frequency of sampling */
  };

  zx_u64 sample_type; /* Specifies values included in sample */
  zx_u64 read_format; /* Specifies values returned in read */

  zx_u64 disabled : 1,    /* off by default */
      inherit : 1,        /* children inherit it */
      pinned : 1,         /* must always be on PMU */
      exclusive : 1,      /* only group on PMU */
      exclude_user : 1,   /* don't count user */
      exclude_kernel : 1, /* don't count kernel */
      exclude_hv : 1,     /* don't count hypervisor */
      exclude_idle : 1,   /* don't count when idle */
      mmap : 1,           /* include mmap data */
      comm : 1,           /* include comm data */
      freq : 1,           /* use freq, not period */
      inherit_stat : 1,   /* per task counts */
      enable_on_exec : 1, /* next exec enables */
      task : 1,           /* trace fork/exit */
      watermark : 1,      /* wakeup_watermark */
      precise_ip : 2,     /* skid constraint */
      mmap_data : 1,      /* non-exec mmap data */
      sample_id_all : 1,  /* sample_type all events */
      exclude_host : 1,   /* don't count in host */
      exclude_guest : 1,  /* don't count in guest */
      exclude_callchain_kernel : 1,
      /* exclude kernel callchains */
      exclude_callchain_user : 1,
      /* exclude user callchains */
      mmap2 : 1,          /* include mmap with inode data */
      comm_exec : 1,      /* flag comm events that are
                             due to exec */
      use_clockid : 1,    /* use clockid for time fields */
      context_switch : 1, /* context switch data */
      write_backward : 1, /* Write ring buffer from end
                             to beginning */
      namespaces : 1,     /* include namespaces data */
      ksymbol : 1,        /* include ksymbol events */
      bpf_event : 1,      /* include bpf events */
      aux_output : 1,     /* generate AUX records
                             instead of events */
      cgroup : 1,         /* include cgroup events */
      text_poke : 1,      /* include text poke events */
      build_id : 1,       /* use build id in mmap2 events */
      inherit_thread : 1, /* children only inherit */
                          /* if cloned with CLONE_THREAD */
      remove_on_exec : 1, /* event is removed from task
                             on exec */
      sigtrap : 1,        /* send synchronous SIGTRAP
                             on event */

      __reserved_1 : 26;

  union {
    zx_u32 wakeup_events;    /* wakeup every n events */
    zx_u32 wakeup_watermark; /* bytes before wakeup */
  };

  zx_u32 bp_type; /* breakpoint type */

  union {
    zx_u64 bp_addr;     /* breakpoint address */
    zx_u64 kprobe_func; /* for perf_kprobe */
    zx_u64 uprobe_path; /* for perf_uprobe */
    zx_u64 config1;     /* extension of config */
  };

  union {
    zx_u64 bp_len;       /* breakpoint size */
    zx_u64 kprobe_addr;  /* with kprobe_func == NULL */
    zx_u64 probe_offset; /* for perf_[k,u]probe */
    zx_u64 config2;      /* extension of config1 */
  };
  zx_u64 branch_sample_type; /* enum perf_branch_sample_type */
  zx_u64 sample_regs_user;   /* user regs to dump on samples */
  zx_u32 sample_stack_user;  /* size of stack to dump on
                               samples */
  zx_i32 clockid;            /* clock to use for time fields */
  zx_u64 sample_regs_intr;   /* regs to dump on samples */
  zx_u32 aux_watermark;      /* aux bytes before wakeup */
  zx_u64 sample_max_stack;   /* max frames in callchain */
  zx_u64 __reserved_2;       /* align to u64 */
  zx_u32 aux_sample_size;    /* max aux sample size */
  zx_u32 __reserved_3;       /* align to u64 */
  zx_u64 sig_data;           /* user data for sigtrap */
};

}  // extern "C"

#endif
