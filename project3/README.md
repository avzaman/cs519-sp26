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

## Rebuilding the Kernel

After applying the patch, rebuild and install from the kernel source root:

```bash
make -j$(nproc)
sudo make modules_install
sudo make install
sudo reboot
```
