Simple OS Simulation 🚀

A high-fidelity simulation of core Operating System components, including a Multi-Level Queue (MLQ) scheduler, a 5-level paging Memory Management Unit (MMU), and a extensible System Call interface. This project mimics real-world kernel behaviors for resource isolation and task orchestration.

📑 Table of Contents

Overview

Core Architecture

MLQ Scheduler

Paging & MMU (32/64-bit)

Kernel System Calls

Instruction Set Architecture

Installation & Execution

Configuration

🧐 Overview

This project simulates the interaction between software processes and virtualized hardware. It manages two critical resources:

CPU Time: Orchestrated via a prioritized MLQ dispatcher.

Physical Memory: Isolated through a Virtual Memory engine supporting RAM and multi-instance SWAP storage.

🏗 Core Architecture

1. MLQ Scheduler

The scheduler implements a Multi-Level Queue policy inspired by the Linux kernel:

Priority Range: 0 to 139 (140 levels).

Time Quantum: Round-Robin execution within each priority queue.

Dynamic Slot Allocation: Each priority level is assigned a specific time slot based on:

slot = MAX_PRIO - priority

Dispatcher: Handles context switching and process enqueuing back to the appropriate priority level.

2. Paging & MMU (32/64-bit)

The system supports sophisticated memory mapping techniques:

Address Schemes: Traditional 32-bit and modern 64-bit 5-level paging (PGD, P4D, PUD, PMD, PT).

Memory Hierarchy:

RAM: Primary storage for active pages.

SWAP: Secondary storage (up to 4 instances) for evicted pages.

Demand Paging: Supports page replacement algorithms and page fault handling.

Segmentation: Virtual memory is divided into VMA (Virtual Memory Areas) to manage code, heap, and stack segments.

3. Kernel System Calls

The interface between user-space and kernel-space is strictly maintained via system calls:

listsyscall: Enumerates supported kernel functions.

memmap: High-level memory management for allocation and VMA boundary adjustments.

Extensibility: Custom handlers can be added by modifying the syscall.tbl and implementing handlers in src/.

💻 Instruction Set Architecture

Processes in the simulation execute the following instructions:

Mnemonic

Arguments

Description

CALC

None

Simulated CPU-bound computation.

ALLOC

[size] [reg]

Allocates size bytes; stores pointer in reg.

FREE

[reg]

Frees memory pointed to by reg.

READ

[src] [off] [dst]

Reads byte from [src] + off into dst register.

WRITE

[data] [dst] [off]

Writes data to memory at [dst] + off.

🚀 Installation & Execution

Build Requirements

GNU Make

GCC (C99 standard recommended)

Build Steps

# Clone the repository
git clone [https://github.com/yourusername/simple-os-sim.git](https://github.com/yourusername/simple-os-sim.git)
cd simple-os-sim

# Compile the project
make all


Running the Simulator

To execute a simulation scenario defined in an input file:

./os input/[scenario_name]


Example: ./os input/os_1

⚙️ Configuration

The simulation's behavior can be toggled in include/os-cfg.h:

#define MLQ_SCHED: Enable/Disable Multi-Level Queue scheduling.

#define MM_PAGING: Enable/Disable the paging memory system.

#define MM64: Switch between 32-bit and 64-bit address translation.

🎓 Academic Credit

Course: Operating Systems (CO2018)

Institution: HCMC University of Technology (HCMUT)

Faculty: Computer Science & Engineering
