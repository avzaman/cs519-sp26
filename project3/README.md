# Cooperative Scheduler Patch

Kernel version: 5.15.0

## Applying the Patch

From the directory containing both the patch file and the kernel source folder:

```bash
patch -p1 -d linux-5.15.0 < cooperative_sched.patch
```

## Verifying the Patch

To confirm the patch applied cleanly:

```bash
patch --dry-run -p1 -d linux-5.15.0 < cooperative_sched.patch
```

## Files Modified

- `include/linux/sched.h` — adds `inactive` flag to `task_struct`
- `include/linux/syscalls.h` — declares `sys_set_inactive`
- `arch/x86/entry/syscalls/syscall_64.tbl` — registers syscall number 449
- `kernel/sched/core.c` — implements `sys_set_inactive`
- `kernel/sched/fair.c` — adds vruntime penalty in `put_prev_entity`

## Patch 2 — Variable Penalty

Extends the system call to accept a penalty duration in seconds, enabling
runtime tuning of the vruntime penalty without recompiling the kernel.
Apply on top of Patch 1.

```bash
patch -p1 -d linux-5.15.0 < cooperative_sched_p2.patch
```

### Kernel Files Modified
- `include/linux/sched.h` — adds `inactive_penalty` field to `task_struct`
- `include/linux/syscalls.h` — updates prototype to `sys_set_inactive(int penalty_secs)`
- `kernel/sched/core.c` — stores penalty in `task_struct` on deactivation
- `kernel/sched/fair.c` — reads per-task penalty instead of hardcoded constant

