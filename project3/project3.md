# Homework 3: Designing a Cooperative Scheduler

**Due:** May 5th, 2026 at 11:55 PM

**GitHub page:** [https://github.com/RutgersCSSystems/cs519-sp26/tree/main/project3/project3.md](https://github.com/RutgersCSSystems/cs519-sp26/tree/main/project3/project3.md)

---

## Overview

In this project, you will implement simple cooperative scheduling in the Linux OS. Part A focuses on understanding the internals of the CFS (Completely Fair Scheduler), and Part B challenges you to extend it with cooperative scheduling support.

---

## Part A — Understanding the Linux CFS Scheduler (15 points)

In class, we discussed the basics of the CFS scheduler. In this part of the project, you will study the CFS source code and write a detailed description addressing each of the following questions:

1. How are tasks added to a per-CPU runqueue, and which file maintains this information?
2. How are tasks scheduled on a CPU, and which functions handle this?
3. How are tasks descheduled or preempted from a CPU?
4. How are runqueues rebalanced across CPUs?
5. What data structures are used to manage runqueues?
6. How does the scheduler convert application-specified nice values to priorities?
7. How are per-task and per-process statistics collected by the scheduler?
8. How are these statistics exposed to userspace?

### Deliverable

A detailed code commentary of the Linux scheduler that identifies the relevant functions and describes what each one does. You should also discuss cases in which the CFS scheduler fails to capture **application idleness** — that is, scenarios where a CPU core is underutilized by an application.

---

## Part B — Changing the CFS Scheduler to Be Cooperative (75 points)

### Step 1: Add a New System Call

Introduce a new system call (or reuse the one you developed in Project 1) that allows application threads to signal their inactivity to the scheduler. This system call should provide a mechanism for a thread to notify the scheduler that it is not actively using the CPU.

Below is a simplified, incomplete example. One option is to implement it in `kernel/sched/core.c`, but this is not strictly required.

```c
syscall_set_inactive(....)
{
    struct task_struct *task = current;  // Get the current task_struct

    // Mark the task as inactive in scheduler data structures
    task->inactive = 1;  // Assume a flag 'inactive' in the task_struct

    // ... (Additional logic for cooperative scheduling)

    return 0;  // Success
}
```

### Step 2: Handle the System Call in the Scheduler

Modify the scheduler to handle the new system call. When invoked, the scheduler should collect information about the calling application — specifically, the **TGID (Thread Group ID)** from the `task_struct`.

Feel free to define your own function signatures. Here is an example skeleton:

```c
static inline void check_preempt_inactive(struct task_struct *p, struct rq *rq, int flags)
{
    if (p->inactive) {
        // Adjust scheduler behavior for inactive tasks
        // ... (Your cooperative scheduling logic)
    } else {
        // Fall back to standard preemption logic
        check_preempt_wakeup(p, rq, flags);
    }
}
```

The skeleton above intentionally leaves the scheduling policy for inactive tasks as an open design decision. How should the scheduler treat a task once it has been marked inactive? Consider the mechanisms CFS already provides — vruntime accounting, timeslice allocation, preemption decisions, task placement — and decide which to leverage. Your report should justify the approach you chose and explain the tradeoffs involved.

You should also design a **re-activation mechanism** — a way for a thread that was previously marked inactive to return to normal scheduling priority once it has real work to do again.

### Step 3: Design a Multi-Threaded Benchmark

Develop a simple multi-threaded benchmark that engages all available cores by continuously spinning in a `while()` loop. Inside the loop, the application should perform either no operations or lightweight dummy operations (e.g., simple vector addition).

### Step 4: Implement Cooperative Scheduling

When the benchmark application is deemed inactive, it should execute the new system call to inform the OS of its reduced priority. This allows the scheduler to deprioritize it in favor of other, actively working applications.

**Important:** Nice values for all applications must remain unchanged throughout this process. The goal is to introduce cooperative scheduling without altering the nice parameters set for individual applications.

### Step 5: Test with the Shared Memory Matrix Application

Run your multi-threaded benchmark alongside the shared memory matrix application from Project 1. Ensure the matrix application performs real work. Execute both applications simultaneously, with cooperative scheduling enabled for the spinning benchmark. Evaluate the performance of the matrix application under these conditions and compare it against its standalone performance.

You are also encouraged to design and include your own additional benchmarks beyond the ones described above.

### Step 6: Report Results

Document and report your experimental results. Specifically:

- Highlight any performance differences observed when cooperative scheduling is enabled for the benchmark, compared to default behavior.
- If possible, compare your approach with the `yield()` system call provided by Linux.
- Discuss any creative insights or unique findings from your experiments.

---

## Submission

Your submission must include the following:

1. **Kernel patch(es) only.** Do not submit full kernel source trees. Submit your changes as patch files generated with `git diff` or `git format-patch`. Clearly document the **kernel version** your patches apply against — this must match the kernel version used in Projects 1 and 2.
2. **Report.** A written report covering your Part A code commentary, your Part B design decisions, and your experimental results.
3. **Test benchmarks.** Include the source code for all user-space benchmarks (the spinning benchmark, any additional benchmarks you designed, and any test harnesses). These must compile and run cleanly.

---

## Resources

Use the same CloudLab facility as in previous assignments.

## Getting Started

Please start working on this homework early. If you have questions, ask them during office hours or post on Piazza.
