# System Call Tracer for xv6

## Project Idea

This project implements a strace-like system call tracer inside xv6.

The tracer allows the user to run a command and see the system calls made by that command.

Example:

strace echo hello

The kernel prints output like:

pid: syscall syscall_name -> return_value

## What I Added

I added a new system call called trace.

The trace system call stores a trace mask inside the current process.

When the process makes a system call, the kernel checks the trace mask. If the syscall is enabled, the kernel prints the syscall name and return value.

## Modified Files

kernel/proc.h
kernel/proc.c
kernel/syscall.h
kernel/syscall.c
kernel/sysproc.c
user/user.h
user/usys.pl
user/strace.c
Makefile

## How It Works

1. A new field trace_mask is added to struct proc.
2. The trace system call sets the trace_mask of the current process.
3. The fork function copies the trace_mask from parent to child.
4. The syscall function checks the syscall number.
5. If the syscall is enabled in the mask, xv6 prints the syscall name and return value.
6. The user command strace runs another command with tracing enabled.

## Test Commands

strace echo hello

strace ls

strace -a echo hello

## Example Output

3: syscall trace -> 0
3: syscall exec -> 2
3: syscall write -> 5

## Conclusion

The project successfully adds a system call tracer to xv6. It helps show how user programs interact with the kernel through system calls.
