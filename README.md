# Simple Operating System Simulation (SimpleOS)

## Overview
This project is a comprehensive simulation of core operating system components, developed as part of the Operating Systems course at **HCMC University of Technology (HCMUT)**. The simulation focuses on the implementation of a multi-level queue scheduler, synchronization mechanisms, and a paging-based virtual memory system.

The system emulates the interaction between software processes, an OS kernel, and virtualized hardware (CPUs and Physical RAM).


---

## Key Features

### 1. Multi-Level Queue (MLQ) Scheduler
The scheduler manages process execution across multiple CPUs using a priority-based MLQ algorithm.
* **Priority Levels:** Supports up to `MAX_PRIO` (140) levels.
* **Time Slicing:** Each process runs in a specific time slice before being enqueued back to its respective priority queue.
* **Slot Calculation:** Execution time is allocated based on priority using a fixed formula: $slot = (MAX\_PRIO - prio)$.

### 2. Paging-Based Memory Management
The memory engine isolates process spaces and handles virtual-to-physical address translation.
* **Dual Architecture Support:** Implements both 32-bit and 64-bit (5-level paging) address schemes.
* **Memory Hierarchy:** Manages a singleton physical RAM device and up to 4 SWAP devices.
* **Dynamic Allocation:** Supports `ALLOC` and `FREE` operations, managing virtual memory areas (`vm_area_struct`) and region lists.
* **Page Swapping:** Handles page faults by moving frames between RAM and SWAP using a FIFO-based replacement policy.

### 3. Kernel Interface (System Calls)
Provides a structured interface between user applications and the kernel.
* **Syscall Table:** Uses a `syscall_tbl` to route requests to appropriate handlers like `_sys_xxxhandler()`.
* **Standard Operations:** Includes system calls for memory mapping (`memmap`), I/O operations, and process listing.

---

## Project Structure

### Header Files (`include/`)
* `sched.h`: Definitions for the scheduler and dispatcher.
* `mem.h` & `mm.h`: Virtual Memory Engine and paging structures.
* `cpu.h`: Virtual CPU implementation functions.
* `common.h`: Core structures including the Process Control Block (PCB).

### Source Files (`src/`)
* `sched.c`: Implementation of MLQ scheduling and `get_proc` logic.
* `mm.c` & `mm-vm.c`: Paging-based memory management implementation.
* `os.c`: The main entry point that initializes and boots the OS.

---

## Instruction Set
The simulated processes support five primary instructions:
| Instruction | Syntax | Description |
| :--- | :--- | :--- |
| **CALC** | `calc` | Performs CPU calculations. |
| **ALLOC** | `alloc [size] [reg]` | Allocates RAM and stores the address in a register. |
| **FREE** | `free [reg]` | Deallocates memory at the address held in the register. |
| **READ** | `read [source] [offset] [dest]` | Reads a byte from memory to a destination register. |
| **WRITE** | `write [data] [dest] [offset]` | Writes data to a specific memory address. |

---

## Getting Started

### Prerequisites
* GCC Compiler
* Make build tool

### Building the Project
To compile the kernel and virtual hardware:
```bash
make all
