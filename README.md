# OSC-Project: SRTF Scheduler Simulator

Interactive Windows console program that simulates the Shortest Remaining Time First (preemptive SJF) CPU scheduling algorithm. The scheduler runs on its own thread using WinAPI synchronization primitives and prints performance metrics plus a Gantt chart for each run.

## What’s here
- `SRTF.c`: main simulation with input prompts, scheduling loop, stats, and Gantt chart.
- `SRTF.exe`: prebuilt Windows binary.

## Requirements
- Windows with a C compiler that supports `windows.h` (MinGW-w64 or MSVC recommended).
- If you’re on macOS/Linux, cross-compile for Windows (e.g., `x86_64-w64-mingw32-gcc`) or build/run inside a Windows VM; the code uses WinAPI threads and won’t build with POSIX headers alone.

## Build
Using MinGW-w64 (example):
```bash
gcc SRTF.c -o SRTF.exe -Wall -Wextra
```
`windows.h` functions link against `kernel32` by default, so no extra libraries are needed.

## Run
```bash
./SRTF.exe
```
You’ll be prompted to:
1) Enter the number of processes (1–10).
2) Provide each process’ arrival time (>= 0) and burst time (> 0).
3) Choose whether to run another scenario after results are shown.

## Output
- Results table: PID, arrival, burst, completion, turnaround, waiting, and response times, plus averages.
- Gantt chart: execution order over time. `IDLE` marks gaps when no process is ready.

## Example session
```
SRTF Process Scheduling Simulation
Enter number of processes (1-10): 3

Process P1:
  Arrival Time (>=0): 0
  Burst Time (>0): 7
Process P2:
  Arrival Time (>=0): 2
  Burst Time (>0): 4
Process P3:
  Arrival Time (>=0): 4
  Burst Time (>0): 1

[System] Starting Scheduler Thread...
[System] Simulation Completed.

SRTF Performance Results:
-----------------------------------------------------------------------------
| PID | Arrival | Burst | Completion | Turnaround | Waiting | Response |
|-----|---------|-------|------------|------------|---------|----------|
| P1  |       0 |     7 |         12 |         12 |       5 |        0 |
| P2  |       2 |     4 |          7 |          5 |       1 |        0 |
| P3  |       4 |     1 |          5 |          1 |       0 |        0 |
------------------------------------------------------------------------------
Average Turnaround Time = 6.00
Average Waiting Time    = 2.00

Gantt Chart:
 ----- ----- -- ----- -------
| P1  | P2  |P3| P2 |   P1  |
0     2     4  5    7      12
```

## Configuration tips
- `TIME_UNIT_DELAY` (in `SRTF.c`) can be raised from `0` to slow the simulation for easier visual tracing.
- Gantt history is stored in `gantt_chart`; increase its size if you expect very long schedules.
