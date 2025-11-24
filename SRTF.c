#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>

// --- Constants & Definitions ---
#define MAX_PROCESSES 10
#define TIME_UNIT_DELAY 0 // Set to 100 or similar if you want to see it run in real-time (ms)

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    int start_time;
    bool is_started;
    bool is_completed;
} Process;

typedef struct {
    int time;
    int pid;
} GanttLog;

// --- Global Variables (Shared Resources) ---
Process processes[MAX_PROCESSES];
int num_processes;
int total_time = 0;

// Gantt Chart Storage
GanttLog gantt_chart[1000];
int gantt_index = 0;

// Synchronization
pthread_mutex_t data_mutex;

// --- Function Prototypes ---
void* srtf_simulation_thread(void* arg);
void calculate_metrics();
void print_gantt_chart();
void print_table();

int main() {
    printf("   SRTF Process Scheduling Simulation (Multithreaded)\n");

    // 1. Input Section (Main Thread / UI)
    while (1) {
        printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
        if (scanf("%d", &num_processes) == 1 && num_processes >= 1 && num_processes <= MAX_PROCESSES) {
            break;
        }
        printf("Invalid input. Please enter a number between 1 and %d.\n", MAX_PROCESSES);
        while(getchar() != '\n'); // clear buffer
    }

    for (int i = 0; i < num_processes; i++) {
        processes[i].pid = i + 1;
        printf("\nProcess P%d:\n", i + 1);
        
        // Input Validation Loop for Arrival Time
        while (1) {
            printf("  Arrival Time (>=0): ");
            if (scanf("%d", &processes[i].arrival_time) == 1 && processes[i].arrival_time >= 0) break;
            printf("  Invalid Arrival Time.\n");
            while(getchar() != '\n');
        }

        // Input Validation Loop for Burst Time
        while (1) {
            printf("  Burst Time (>0): ");
            if (scanf("%d", &processes[i].burst_time) == 1 && processes[i].burst_time > 0) break;
            printf("  Invalid Burst Time. Must be greater than 0.\n");
            while(getchar() != '\n');
        }

        // Initialize dynamic fields
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].is_completed = false;
        processes[i].is_started = false;
    }

    // 2. Initialize Synchronization
    if (pthread_mutex_init(&data_mutex, NULL) != 0) {
        fprintf(stderr, "Mutex init failed\n");
        return 1;
    }

    // 3. Start Simulation Thread (Kernel Thread)
    pthread_t tid;
    printf("\n[System] Starting Scheduler Thread...\n");
    if (pthread_create(&tid, NULL, srtf_simulation_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create thread\n");
        return 1;
    }

    // 4. Wait for Simulation to Finish
    pthread_join(tid, NULL);
    printf("[System] Simulation Completed.\n");

    // 5. Output Results (UI)
    calculate_metrics();
    print_table();
    print_gantt_chart();

    // Cleanup
    pthread_mutex_destroy(&data_mutex);
    return 0;
}

// --- Scheduler Logic (Worker Thread) ---
void* srtf_simulation_thread(void* arg) {
    int completed_count = 0;
    int current_time = 0;
    int prev_process_id = -1;

    // Simulation Loop
    while (completed_count < num_processes) {
        int shortest_idx = -1;
        int min_remaining = INT_MAX;

        // Critical Section: Reading Shared Data
        pthread_mutex_lock(&data_mutex);

        // Find process with Shortest Remaining Time among those arrived
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].arrival_time <= current_time && !processes[i].is_completed) {
                if (processes[i].remaining_time < min_remaining) {
                    min_remaining = processes[i].remaining_time;
                    shortest_idx = i;
                }
                // Tie-breaker: If remaining time is equal, FCFS (implicit by loop order or arrival)
                if (processes[i].remaining_time == min_remaining) {
                    if (processes[i].arrival_time < processes[shortest_idx].arrival_time) {
                        shortest_idx = i;
                    }
                }
            }
        }

        if (shortest_idx != -1) {
            // Process found
            Process *p = &processes[shortest_idx];

            // Response Time Logic: First time CPU touches this process
            if (!p->is_started) {
                p->start_time = current_time;
                p->response_time = current_time - p->arrival_time;
                p->is_started = true;
            }

            // Logging for Gantt Chart (Compression logic: only log on switch)
            if (p->pid != prev_process_id) {
                gantt_chart[gantt_index].time = current_time;
                gantt_chart[gantt_index].pid = p->pid;
                gantt_index++;
                prev_process_id = p->pid;
            }

            // Execute for 1 unit
            p->remaining_time--;
            current_time++;

            // Check completion
            if (p->remaining_time == 0) {
                p->completion_time = current_time;
                p->is_completed = true;
                completed_count++;
            }
        } else {
            // IDLE CPU (No process arrived yet)
            if (prev_process_id != 0) {
                gantt_chart[gantt_index].time = current_time;
                gantt_chart[gantt_index].pid = 0; // 0 represents IDLE
                gantt_index++;
                prev_process_id = 0;
            }
            current_time++;
        }

        pthread_mutex_unlock(&data_mutex);
        // End Critical Section
    }

    // Capture final time for Gantt chart end
    gantt_chart[gantt_index].time = current_time;
    gantt_chart[gantt_index].pid = -1; // End marker
    
    return NULL;
}

// --- Helper Functions ---

void calculate_metrics() {
    for (int i = 0; i < num_processes; i++) {
        // Turnaround Time = Completion - Arrival
        processes[i].turnaround_time = processes[i].completion_time - processes[i].arrival_time;
        
        // Waiting Time = Turnaround - Burst
        processes[i].waiting_time = processes[i].turnaround_time - processes[i].burst_time;
    }
}

void print_table() {
    printf("\nSRTF Performance Results:\n");
    printf("----------------------------------------------------------------------------------\n");
    printf("| PID | Arrival | Burst | Completion | Turnaround | Waiting | Response |\n");
    printf("|-----|---------|-------|------------|------------|---------|----------|\n");
    
    float avg_tat = 0, avg_wt = 0;
    
    for (int i = 0; i < num_processes; i++) {
        printf("| P%d  | %7d | %5d | %10d | %10d | %7d | %8d |\n", 
               processes[i].pid, 
               processes[i].arrival_time, 
               processes[i].burst_time, 
               processes[i].completion_time, 
               processes[i].turnaround_time, 
               processes[i].waiting_time, 
               processes[i].response_time);
        
        avg_tat += processes[i].turnaround_time;
        avg_wt += processes[i].waiting_time;
    }
    printf("----------------------------------------------------------------------------------\n");
    printf("Average Turnaround Time = %.2f\n", avg_tat / num_processes);
    printf("Average Waiting Time    = %.2f\n", avg_wt / num_processes);
}

void print_gantt_chart() {
    printf("\nGantt Chart:\n");
    
    // Top Border
    printf(" ");
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        // simple scaling for display
        for(int k=0; k<duration; k++) printf("--");
        printf("-");
    }
    printf("\n|");

    // Process IDs
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        int mid = duration; 
        
        for(int k=0; k<duration-1; k++) printf(" ");
        if (gantt_chart[i].pid == 0) printf("IDLE");
        else printf("P%d", gantt_chart[i].pid);
        for(int k=0; k<duration-1; k++) printf(" ");
        printf("|");
    }
    
    // Bottom Border
    printf("\n ");
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        for(int k=0; k<duration; k++) printf("--");
        printf("-");
    }
    
    // Timeline
    printf("\n0");
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        for(int k=0; k<duration; k++) printf("  ");
        if (gantt_chart[i+1].time > 9) printf("\b"); // adjust backspace for 2 digit numbers
        printf("%d", gantt_chart[i+1].time);
    }
    printf("\n");
}