// These definitions are taken from
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/perf_event.h.
// Types are changed to zx_* types.
//
// These defitions are only definitions used for interfacing with the kernel
// through syscalls. So this is not derived work.
//
// Original license notice:
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Performance events:
 *
 *    Copyright (C) 2008-2009, Linutronix GmbH, Thomas Gleixner
 * <tglx@kernel.org> Copyright (C) 2008-2011, Red Hat, Inc., Ingo Molnar
 *    Copyright (C) 2008-2011, Red Hat, Inc., Peter Zijlstra
 *
 * Data type definitions, declarations, prototypes.
 *
 *    Started by: Thomas Gleixner and Ingo Molnar
 *
 * For licencing details see kernel-base/COPYING
 */

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

/*
 * Structure of the page that can be mapped via mmap
 */
struct perf_event_mmap_page {
  zx_u32 version;        /* version number of this structure */
  zx_u32 compat_version; /* lowest version this is compat with */

  /*
   * Bits needed to read the HW events in user-space.
   *
   *   u32 seq, time_mult, time_shift, index, width;
   *   u64 count, enabled, running;
   *   u64 cyc, time_offset;
   *   s64 pmc = 0;
   *
   *   do {
   *     seq = pc->lock;
   *     barrier()
   *
   *     enabled = pc->time_enabled;
   *     running = pc->time_running;
   *
   *     if (pc->cap_usr_time && enabled != running) {
   *       cyc = rdtsc();
   *       time_offset = pc->time_offset;
   *       time_mult   = pc->time_mult;
   *       time_shift  = pc->time_shift;
   *     }
   *
   *     index = pc->index;
   *     count = pc->offset;
   *     if (pc->cap_user_rdpmc && index) {
   *       width = pc->pmc_width;
   *       pmc = rdpmc(index - 1);
   *     }
   *
   *     barrier();
   *   } while (pc->lock != seq);
   *
   * NOTE: for obvious reason this only works on self-monitoring
   *       processes.
   */
  zx_u32 lock;         /* seqlock for synchronization */
  zx_u32 index;        /* hardware event identifier */
  zx_i64 offset;       /* add to hardware event value */
  zx_u64 time_enabled; /* time event active */
  zx_u64 time_running; /* time event on CPU */
  union {
    zx_u64 capabilities;
    struct {
      zx_u64 cap_bit0 : 1, /* Always 0, deprecated, see commit 860f085b74e9 */
          cap_bit0_is_deprecated : 1, /* Always 1, signals that bit 0 is zero */

          cap_user_rdpmc
          : 1, /* The RDPMC instruction can be used to read counts */
          cap_user_time : 1, /* The time_{shift,mult,offset} fields are used */
          cap_user_time_zero : 1,  /* The time_zero field is used */
          cap_user_time_short : 1, /* the time_{cycle,mask} fields are used */
          cap_____res : 58;
    };
  };

  /*
   * If cap_user_rdpmc this field provides the bit-width of the value
   * read using the rdpmc() or equivalent instruction. This can be used
   * to sign extend the result like:
   *
   *   pmc <<= 64 - width;
   *   pmc >>= 64 - width; // signed shift right
   *   count += pmc;
   */
  zx_u16 pmc_width;

  /*
   * If cap_usr_time the below fields can be used to compute the time
   * delta since time_enabled (in ns) using RDTSC or similar.
   *
   *   u64 quot, rem;
   *   u64 delta;
   *
   *   quot = (cyc >> time_shift);
   *   rem = cyc & (((u64)1 << time_shift) - 1);
   *   delta = time_offset + quot * time_mult +
   *              ((rem * time_mult) >> time_shift);
   *
   * Where time_offset,time_mult,time_shift and cyc are read in the
   * seqcount loop described above. This delta can then be added to
   * enabled and possible running (if index), improving the scaling:
   *
   *   enabled += delta;
   *   if (index)
   *     running += delta;
   *
   *   quot = count / running;
   *   rem  = count % running;
   *   count = quot * enabled + (rem * enabled) / running;
   */
  zx_u16 time_shift;
  zx_u32 time_mult;
  zx_u64 time_offset;
  /*
   * If cap_usr_time_zero, the hardware clock (e.g. TSC) can be calculated
   * from sample timestamps.
   *
   *   time = timestamp - time_zero;
   *   quot = time / time_mult;
   *   rem  = time % time_mult;
   *   cyc = (quot << time_shift) + (rem << time_shift) / time_mult;
   *
   * And vice versa:
   *
   *   quot = cyc >> time_shift;
   *   rem  = cyc & (((u64)1 << time_shift) - 1);
   *   timestamp = time_zero + quot * time_mult +
   *               ((rem * time_mult) >> time_shift);
   */
  zx_u64 time_zero;

  zx_u32 size; /* Header size up to __reserved[] fields. */
  zx_u32 __reserved_1;

  /*
   * If cap_usr_time_short, the hardware clock is less than 64bit wide
   * and we must compute the 'cyc' value, as used by cap_usr_time, as:
   *
   *   cyc = time_cycles + ((cyc - time_cycles) & time_mask)
   *
   * NOTE: this form is explicitly chosen such that cap_usr_time_short
   *       is a correction on top of cap_usr_time, and code that doesn't
   *       know about cap_usr_time_short still works under the assumption
   *       the counter doesn't wrap.
   */
  zx_u64 time_cycles;
  zx_u64 time_mask;

  /*
   * Hole for extension of the self monitor capabilities
   */

  zx_u8 __reserved[116 * 8]; /* align to 1k. */

  /*
   * Control data for the mmap() data buffer.
   *
   * User-space reading the @data_head value should issue an smp_rmb(),
   * after reading this value.
   *
   * When the mapping is PROT_WRITE the @data_tail value should be
   * written by user-space to reflect the last read data, after issuing
   * an smp_mb() to separate the data read from the ->data_tail store.
   * In this case the kernel will not over-write unread data.
   *
   * See perf_output_put_handle() for the data ordering.
   *
   * data_{offset,size} indicate the location and size of the perf record
   * buffer within the mmapped area.
   */
  zx_u64 data_head;   /* head in the data section */
  zx_u64 data_tail;   /* user-space written tail */
  zx_u64 data_offset; /* where the buffer starts */
  zx_u64 data_size;   /* data buffer size */

  /*
   * AUX area is defined by aux_{offset,size} fields that should be set
   * by the user-space, so that
   *
   *   aux_offset >= data_offset + data_size
   *
   * prior to mmap()ing it. Size of the mmap()ed area should be aux_size.
   *
   * Ring buffer pointers aux_{head,tail} have the same semantics as
   * data_{head,tail} and same ordering rules apply.
   */
  zx_u64 aux_head;
  zx_u64 aux_tail;
  zx_u64 aux_offset;
  zx_u64 aux_size;
};

/*
 * 'struct perf_event_attr' contains various attributes that define
 * a performance event - most of them hardware related configuration
 * details, but also a lot of behavioral switches and values implemented
 * by the kernel.
 */
struct perf_event_attr {
  /*
   * Major type: hardware/software/tracepoint/etc.
   */
  zx_u32 type;

  /*
   * Size of the attr structure, for forward/backwards compatibility.
   */
  zx_u32 size;

  /*
   * Type specific configuration information.
   */
  zx_u64 config;

  union {
    zx_u64 sample_period;
    zx_u64 sample_freq;
  };

  zx_u64 sample_type;
  zx_u64 read_format;

  zx_u64 disabled : 1,    /* off by default        */
      inherit : 1,        /* children inherit it   */
      pinned : 1,         /* must always be on PMU */
      exclusive : 1,      /* only group on PMU     */
      exclude_user : 1,   /* don't count user      */
      exclude_kernel : 1, /* ditto kernel          */
      exclude_hv : 1,     /* ditto hypervisor      */
      exclude_idle : 1,   /* don't count when idle */
      mmap : 1,           /* include mmap data     */
      comm : 1,           /* include comm data     */
      freq : 1,           /* use freq, not period  */
      inherit_stat : 1,   /* per task counts       */
      enable_on_exec : 1, /* next exec enables     */
      task : 1,           /* trace fork/exit       */
      watermark : 1,      /* wakeup_watermark      */
      /*
       * precise_ip:
       *
       *  0 - SAMPLE_IP can have arbitrary skid
       *  1 - SAMPLE_IP must have constant skid
       *  2 - SAMPLE_IP requested to have 0 skid
       *  3 - SAMPLE_IP must have 0 skid
       *
       *  See also PERF_RECORD_MISC_EXACT_IP
       */
      precise_ip : 2,    /* skid constraint       */
      mmap_data : 1,     /* non-exec mmap data    */
      sample_id_all : 1, /* sample_type all events */

      exclude_host : 1,  /* don't count in host   */
      exclude_guest : 1, /* don't count in guest  */

      exclude_callchain_kernel : 1, /* exclude kernel callchains */
      exclude_callchain_user : 1,   /* exclude user callchains */
      mmap2 : 1,                    /* include mmap with inode data     */
      comm_exec : 1,      /* flag comm events that are due to an exec */
      use_clockid : 1,    /* use @clockid for time fields */
      context_switch : 1, /* context switch data */
      write_backward : 1, /* write ring buffer from end to beginning */
      namespaces : 1,     /* include namespaces data */
      ksymbol : 1,        /* include ksymbol events */
      bpf_event : 1,      /* include BPF events */
      aux_output : 1,     /* generate AUX records instead of events */
      cgroup : 1,         /* include cgroup events */
      text_poke : 1,      /* include text poke events */
      build_id : 1,       /* use build ID in mmap2 events */
      inherit_thread
      : 1, /* children only inherit if cloned with CLONE_THREAD */
      remove_on_exec : 1,  /* event is removed from task on exec */
      sigtrap : 1,         /* send synchronous SIGTRAP on event */
      defer_callchain : 1, /* request PERF_RECORD_CALLCHAIN_DEFERRED records */
      defer_output : 1,    /* output PERF_RECORD_CALLCHAIN_DEFERRED records */
      __reserved_1 : 24;

  union {
    zx_u32 wakeup_events;    /* wake up every n events */
    zx_u32 wakeup_watermark; /* bytes before wakeup   */
  };

  zx_u32 bp_type;
  union {
    zx_u64 bp_addr;
    zx_u64 kprobe_func; /* for perf_kprobe */
    zx_u64 uprobe_path; /* for perf_uprobe */
    zx_u64 config1;     /* extension of config */
  };
  union {
    zx_u64 bp_len;
    zx_u64 kprobe_addr;  /* when kprobe_func == NULL */
    zx_u64 probe_offset; /* for perf_[k,u]probe */
    zx_u64 config2;      /* extension of config1 */
  };
  zx_u64 branch_sample_type; /* enum perf_branch_sample_type */

  /*
   * Defines set of user regs to dump on samples.
   * See asm/perf_regs.h for details.
   */
  zx_u64 sample_regs_user;

  /*
   * Defines size of the user stack to dump on samples.
   */
  zx_u32 sample_stack_user;

  zx_i32 clockid;
  /*
   * Defines set of regs to dump for each sample
   * state captured on:
   *  - precise = 0: PMU interrupt
   *  - precise > 0: sampled instruction
   *
   * See asm/perf_regs.h for details.
   */
  zx_u64 sample_regs_intr;

  /*
   * Wakeup watermark for AUX area
   */
  zx_u32 aux_watermark;

  /*
   * Max number of frame pointers in a callchain, should be
   * lower than /proc/sys/kernel/perf_event_max_stack.
   *
   * Max number of entries of branch stack should be lower
   * than the hardware limit.
   */
  zx_u16 sample_max_stack;

  zx_u16 __reserved_2;
  zx_u32 aux_sample_size;

  union {
    zx_u32 aux_action;
    struct {
      zx_u32 aux_start_paused : 1, /* start AUX area tracing paused */
          aux_pause : 1,           /* on overflow, pause AUX area tracing */
          aux_resume : 1,          /* on overflow, resume AUX area tracing */
          __reserved_3 : 29;
    };
  };

  /*
   * User provided data if sigtrap=1, passed back to user via
   * siginfo_t::si_perf_data, e.g. to permit user to identify the event.
   * Note, siginfo_t::si_perf_data is long-sized, and sig_data will be
   * truncated accordingly on 32 bit architectures.
   */
  zx_u64 sig_data;

  zx_u64 config3; /* extension of config2 */
  zx_u64 config4; /* extension of config3 */
};

} // extern "C"

#endif
