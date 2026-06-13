# NEXUS OS — Operating System Simulator

A Linux-based OS simulator written in C++ that demonstrates core operating system concepts through real process management, IPC, scheduling, and synchronization.

## Overview

NEXUS OS boots with a custom loading screen and accepts hardware resources (RAM, HDD, CPU cores) as CLI arguments. Each of the 20+ built-in applications runs as an independent child process in its own `xterm` window, communicating with the kernel via **pipes** to negotiate memory before execution.

```bash
g++ os.cpp -o NEXUS -lpthread
./NEXUS <RAM_GB> <HDD_GB> <CPU_CORES>

# Example
./NEXUS 2 256 8
```

## Architecture

```
┌──────────────────────────────────────────┐
│               NEXUS OS Kernel            │
│                                          │
│  ┌─────────────┐   ┌──────────────────┐  │
│  │  Resource   │   │   PCB Table      │  │
│  │  Manager    │◄──│ PID|State|Memory │  │
│  └──────┬──────┘   └──────────────────┘  │
│         │                                │
│  ┌──────▼──────────────────────────────┐ │
│  │        Multilevel Ready Queue       │ │
│  │  L1: Round Robin  │  L2: Priority   │ │
│  └──────┬──────────────────────────────┘ │
└─────────┼────────────────────────────────┘
          │  fork() + execlp()
    ┌─────▼─────┐  ┌───────────┐  ┌───────┐
    │ xterm(P1) │  │ xterm(P2) │  │ xterm │
    │Calculator │  │  Notepad  │  │ Clock │
    └───────────┘  └───────────┘  └───────┘
```

**Process lifecycle:** `NEW → READY → RUNNING → BLOCKED → READY → TERMINATED`

## OS Concepts Implemented

| Concept | How |
|---|---|
| Process Creation | `fork()` + `execlp()` per task |
| IPC | Pipes for RAM/HDD negotiation |
| Scheduling | Round Robin + Multilevel Queue |
| Synchronization | `pthread_mutex_t`, `sem_t`, `pthread_cond_t` |
| Context Switching | `SIGSTOP` / `SIGCONT` |
| Memory Management | Runtime RAM & HDD allocation tracking |
| User / Kernel Mode | Kernel mode gates process termination and memory ops |
| Interrupts | Close / minimize signals mid-execution |

## Tasks

**Foreground** — Calculator, Notepad (autosave), File Editor, Text Search, Mini Game

**Background** — Music Simulation, File Copy, File Move, File Delete, Print Simulation

**System / Utility** — Clock, Calendar, File Creation, File Info, RAM Viewer, Process Viewer, Log Generator, Random Number Generator, Timer/Alarm, Auto-Backup

## Prerequisites

```bash
sudo apt install g++ xterm
```

## Project Structure

```
nexus-os/
├── os.cpp
├── process.cpp / .h
├── resource_manager.cpp / .h
├── kernel.cpp / .h
├── ready_queue.cpp / .h
├── round_robin_scheduler.cpp / .h
├── multilevel_queue_scheduler.cpp / .h
└── tasks/          # 20 separate task binaries
```

---