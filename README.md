# CS 519 — Operating Systems Theory (Rutgers, Spring 2026)

Coursework for a graduate operating systems class that was structured around a
handful of large projects instead of exams. Every project involved modifying,
compiling, and booting a custom **Linux 5.15** kernel, then benchmarking the
result. Topics covered here: system call implementation and the user/kernel
copy boundary, IPC and multicore scalability, lock implementations and
contention, virtual memory and page-fault handling, and the Completely Fair
Scheduler (CFS).

All experiments were run on [CloudLab](https://www.cloudlab.us/) bare-metal
nodes (m510-class, 16 cores) so that timing numbers were not polluted by a
hypervisor or a noisy shared machine.

> **Note on what is tracked here:** kernel source trees are deliberately
> `.gitignore`d. What is committed is the *diff* — the patch files, the
> user-space benchmarks, the build/run scripts, and the raw result logs.

---

## Repository layout

| Directory | Project | What it is |
|---|---|---|
| [`locks-bench/`](locks-bench/) | Homework 1 | Five lock implementations benchmarked under contention and profiled with `perf` |
| [`project1/`](project1/) | Project 1, Part 1 | A custom `app_helper` system call in `mm/mmap.c` + syscall latency measurement |
| [`project1-part2/`](project1-part2/) | Project 1, Part 2 | Parallel matrix multiplication over pipes, System V shared memory, and message queues; multicore scaling study |
| [`project2/`](project2/) | Project 2 | An extent-based software page table (red-black tree) built inside the kernel page-fault path |
| [`project3/`](project3/) | Project 3 | A cooperative scheduler: CFS extended so idle-spinning threads can yield priority without changing nice values |

Each directory keeps the original assignment spec (`*.md`) alongside the
implementation, so the requirements and the solution sit next to each other.

---

## Building a kernel

Projects 1–3 share the same kernel build workflow. From inside the kernel
source tree:

```bash
# first time on a fresh node — installs build dependencies
../install_packages.sh

# full build, ~30-40 minutes; only needed once per node
../compile_os_nopackages.sh

# every rebuild after a source change
../compile_os_quick.sh

sudo reboot
```

Kernel source is fetched with `apt source linux-image-unsigned-$(uname -r)`
against Ubuntu's 5.15 tree.

---

## Homework 1 — Lock implementations and `perf` profiling

[`locks-bench/`](locks-bench/)

A shared-counter benchmark run under five different mutual-exclusion
primitives, each implemented as a self-contained header:

| Header | Lock |
|---|---|
| [`compare_and_swap.h`](locks-bench/compare_and_swap.h) | CAS spin lock |
| [`test_and_set.h`](locks-bench/test_and_set.h) | TAS spin lock |
| [`ticket_lock.h`](locks-bench/ticket_lock.h) | Ticket lock (with cache-line padding between `next_ticket` and `now_serving`) |
| [`mutex_lock.h`](locks-bench/mutex_lock.h) | `pthread_mutex` |
| [`semaphore_lock.h`](locks-bench/semaphore_lock.h) | POSIX semaphore |

```bash
cd locks-bench && make
./locks_bench <lock_type 1-5> [num_threads] [iterations] [work_length]

# profile where the cycles actually go
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
perf record -a -g ./locks_bench 1
perf report -g "graph,0.5,caller"
```

The point of the exercise is reading the `perf` call graph: how much of the
runtime is burned in `acquire_lock` versus real work, and how the split between
userspace spinning and kernel-side blocking changes as thread count rises. The
lock knowledge fed directly into the scalability work in Project 1 Part 2.

---

## Project 1, Part 1 — A custom system call

[`project1/`](project1/)

Adds `app_helper` as syscall **449** in `mm/mmap.c`. It takes a user-space
buffer plus a length, `copy_from_user()`s it into a `kzalloc`'d kernel buffer,
rewrites every byte, and `copy_to_user()`s it back — a minimal but honest
exercise of the user/kernel data-transfer boundary.

```c
SYSCALL_DEFINE2(app_helper, void *, buffer, int, size)
{
        int i;
        char *kernelBuffer = kzalloc(size, GFP_KERNEL);
        copy_from_user(kernelBuffer, buffer, size);
        for (i = 0; i < size; i++)
                kernelBuffer[i] = 1;
        copy_to_user(buffer, kernelBuffer, size);
        return 0;
}
```

**Kernel changes** (unified diffs under
[`project1/linux-5.15.0/`](project1/linux-5.15.0/)): `mm/mmap.c`,
`include/linux/syscalls.h`, `arch/x86/entry/syscalls/syscall_{32,64}.tbl`.

**Measuring it** — [`test_app_helper.c`](project1/test_app_helper.c) allocates a
buffer, fills it with `4`, invokes the syscall 100,000 times, reports average
per-call latency, and validates that every byte came back as `1`:

```bash
cd project1
./run_tests.sh              # sweeps 256, 512, 1024, 2048 byte buffers
./run_tests.sh 4096 8192    # or pass your own sizes
```

---

## Project 1, Part 2 — IPC and multicore scalability

[`project1-part2/`](project1-part2/)

Parallel matrix multiplication where the parent forks N children and the work
is split across them, implemented three ways so the IPC mechanisms can be
compared head to head:

- [`IPC-pipe.c`](project1-part2/IPC-pipe.c) — anonymous pipes
- [`IPC-shmem.c`](project1-part2/IPC-shmem.c) — System V shared memory (`shmget`/`shmat`)
- [`IPC-mesg.c`](project1-part2/IPC-mesg.c) — System V message queues (extra, beyond the spec)

All three synchronize with System V semaphores (`semctl`/`semop`), allocate
matrices dynamically to handle sizes up to 10000×10000, and verify the product
before reporting runtime.

```bash
cd project1-part2 && make

./IPC-shmem 5000               # single run, size 5000x5000

./shmem-bench.sh               # sweep sizes 1000-9000 x cores 2-16
./shmem-perf.sh                # perf record each core count
```

The benchmark scripts pin runs with `taskset --cpu-list 0-N` to sweep core
counts, and the raw output is committed
([`shmem-bench.log`](project1-part2/shmem-bench.log),
[`pipe-bench.log`](project1-part2/pipe-bench.log)).

**Result — scaling has a knee.** Shared memory, 5000×5000:

| Cores | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
|---|---|---|---|---|---|---|---|---|
| Runtime (s) | 883 | 456 | 316 | 250 | 261 | 289 | 328 | 388 |

Near-linear speedup out to 8 cores, then it *reverses* — past the knee the
workload is bound by memory bandwidth and cross-socket traffic rather than
compute, so adding cores adds contention instead of throughput. The `perf`
runs were used to attribute that overhead to specific symbols.

---

## Project 2 — Extent-based software page table

[`project2/`](project2/) · patch: [`project2.patch`](project2/project2.patch)

Instead of tracking every 4 KB page individually, this patch groups
*physically contiguous* pages into **extents** and indexes them in a red-black
tree hung off `mm_struct`. The motivation is TLB pressure: one TLB entry per
extent would cover many pages at once.

```c
struct extent_pagetable {
        struct rb_root  root;
        spinlock_t      lock;      /* extents are touched concurrently */
};
```

- Each `mm_struct` gains an `extent_pt` tree plus a `use_extents` opt-in flag,
  initialized in `kernel/fork.c`.
- The anonymous page-fault path in `mm/memory.c` looks up the faulting page's
  physical address, extends an adjacent extent or creates a new one, and keeps
  the page count and address range current.
- Teardown and page removal are handled on unmap/exit so the tree does not leak.
- Guarded by a per-`mm` spinlock, and validated against a multithreaded
  benchmark for thread safety.
- Syscall **449** is repurposed as `use_extents` to switch extent tracking on
  for a process.

**Kernel files touched:** `include/linux/mm_types.h`, `include/linux/mm.h`,
`include/linux/syscalls.h`, `kernel/fork.c`, `mm/mmap.c`, `mm/memory.c`,
`arch/x86/entry/syscalls/syscall_{32,64}.tbl`.

```bash
cd project2
./benchmark.sh        # sweeps thread counts 4-48 and page counts 16-188,
                      # with and without extents, perf-recording each run

./check_tlb.sh        # reads the extent/page counts out of dmesg and reports
                      # how many TLB entries extents would have saved
```

[`multithread-pagefault-averagetime.c`](project2/multithread-pagefault-averagetime.c)
is the fault-generating benchmark; each configuration is run twice — baseline
and extent-enabled — so the bookkeeping overhead of maintaining the tree can be
separated from the TLB savings it would buy.

---

## Project 3 — Cooperative scheduling in CFS

[`project3/`](project3/) · patch:
[`cooperative_sched.patch`](project3/cooperative_sched.patch) ·
[project notes](project3/README.md)

The problem: CFS has no way to know that a thread spinning in a `while()` loop
isn't doing anything useful. It looks perfectly CPU-hungry, so it keeps getting
scheduled and starves applications doing real work.

The fix here is a `set_inactive` system call (**449**) that lets a thread
volunteer its own idleness, plus a scheduler-side penalty that acts on it:

```c
/* kernel/sched/fair.c — put_prev_entity(), before re-enqueueing */
if (task_of(prev)->inactive)
        prev->vruntime += INACTIVE_PENALTY_NS;   /* 10s */
```

Inflating `vruntime` pushes the task far to the right of the red-black
runqueue, so CFS naturally picks it last — no nice values are touched, which
was a hard requirement. Calling the syscall a second time **reactivates** the
thread by resetting its `vruntime` to the runqueue's `min_vruntime`, so it
rejoins normal scheduling immediately rather than being penalized for the rest
of its life.

**Kernel files touched:** `include/linux/sched.h` (`inactive` flag on
`task_struct`), `include/linux/syscalls.h`, `kernel/sched/core.c` (the syscall),
`kernel/sched/fair.c` (the penalty), `arch/x86/entry/syscalls/syscall_64.tbl`.

A second iteration ([`old_patch/`](project3/old_patch/)) makes the penalty a
runtime parameter — `set_inactive(int penalty_secs)` stores a per-task
`inactive_penalty` — so the aggressiveness can be tuned without recompiling the
kernel.

```bash
cd project3
patch -p1 -d linux-5.15.0 < cooperative_sched.patch   # --dry-run first to verify

gcc -O2 -pthread -o benchmark benchmark.c
gcc -O2 -pthread -o matrix matrix.c

./compare.sh          # spinner alone, default vs cooperative
./run_scaling.sh      # matrix + N spinners, N in {1,2,4,8,12,16}, both modes
python3 plots.py      # publication-ready figures from scaling_results.csv
```

`benchmark.c` is the antagonist — spinning threads that call `set_inactive` when
run with `--cooperative`, reporting per-thread iterations, `vruntime`, and
involuntary context switches. `matrix.c` is the victim doing real work: a
10000×10000 single-precision multiply, cache-tiled at 64 and run against a
transposed `B` so the inner loop stays sequential. `run_scaling.sh` runs them
together and records both sides into
[`scaling_results.csv`](project3/scaling_results.csv).

**Result — the matrix workload stops caring about the spinners.**

| Spinners | Default (GFLOPS) | Cooperative (GFLOPS) |
|---|---|---|
| 0 (alone) | 3.91 | — |
| 1 | 3.67 | 3.75 |
| 2 | 3.48 | 3.87 |
| 4 | 3.11 | 3.85 |
| 8 | 2.48 | 3.83 |
| 16 | 2.01 | 3.88 |

Under default CFS, 16 spinning threads cut the matrix application's throughput
roughly in half (3.91 → 2.01 GFLOPS). With cooperative scheduling enabled, it
holds at 3.88 GFLOPS — within ~1% of running completely alone. The spinners
still make forward progress; they just stop taking CPU time away from work that
matters.

---

## Toolchain

C · Linux kernel 5.15 · `perf` · GDB/`printk`/`dmesg` · bash · GNU Make ·
Python (pandas, matplotlib) for figures · CloudLab bare metal

## License / academic honesty

Course assignment specs (`project*.md`, `locks-bench/README.md`) are the
instructors' material and belong to the Rutgers CS Systems group. The
implementations, patches, benchmarks, and analysis scripts are mine. If you are
currently taking this course: read it, don't copy it.
