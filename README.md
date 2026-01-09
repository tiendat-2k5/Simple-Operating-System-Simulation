Simple Operating System Simulation

A simulation of core Operating System components including multi-level scheduling, memory management (paging/segmentation), and system call interfaces. This project was developed as part of the Operating Systems course at Ho Chi Minh City University of Technology (HCMUT).

Table of Contents

Overview

Key Features

MLQ Scheduler

Memory Management Unit (MMU)

System Calls

Project Structure

Getting Started

Process Instruction Set

Overview

The goal of this assignment is to simulate major components of a simple OS to understand the fundamental principles of resource management. The system manages two virtual resources: CPUs and RAM, using two core modules: a Scheduler/Dispatcher and a Virtual Memory engine.

Key Features

MLQ Scheduler

The system implements a Multi-Level Queue (MLQ) scheduling policy:

Priorities: Supports up to 140 priority levels.

Algorithm: Processes are assigned to queues based on priority. The CPU runs processes in a Round-Robin style within fixed time slices.

Slot Allocation: Each queue has a fixed number of slots to use the CPU, calculated as: slot = (MAX_PRIO - priority).

Memory Management Unit (MMU)

A sophisticated paging-based memory management system:

Address Schemes: Supports both 32-bit and 64-bit (5-level paging) address translation.

Isolation: Each process has its own virtual memory space, isolated from others.

Physical Memory: Manages one RAM device and up to 4 SWAP devices.

Paging Operations: Implements page allocation, swapping (in/out), and page table directory (PGD) management.

System Calls

Provides a fundamental interface between applications and the kernel. Supported calls include:

listsyscall: List all available system calls.

memmap: Map memory, increase VMA limits, and handle swapping.

Project Structure

include/: Header files (sched.h, mem.h, cpu.h, common.h, etc.)

src/: Implementation files (sched.c, mm.c, cpu.c, os.c, etc.)

input/: Configuration files and sample process programs.

output/: Sample outputs for verification.

Getting Started

Prerequisites

GCC Compiler

GNU Make

Installation

Compile the source code using the provided Makefile:

make all


Running the Simulation

Execute the simulation by specifying a configuration file:

./os [path_to_config_file]


Example:

./os input/os_1


Process Instruction Set

The simulated processes support the following instructions:

CALC: Perform CPU calculations.

ALLOC [size] [reg]: Allocate memory and store the address in a register.

FREE [reg]: Deallocate memory.

READ [source_reg] [offset] [dest_reg]: Read a byte from memory.

WRITE [data] [dest_reg] [offset]: Write data to memory.

Course: Operating Systems (CO2018)

Institution: Faculty of Computer Science & Engineering, HCMUT.
