#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <windows.h> // Library for threading and synchronization

// Constants & Definitions 
#define MAX_PROCESSES 10
#define TIME_UNIT_DELAY 0 // in milliseconds

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

// Global Variables (Shared Resources)
Process processes[MAX_PROCESSES];
int num_processes;

// Gantt Chart Storage
GanttLog gantt_chart[1000];
int gantt_index = 0;

// Synchronization
CRITICAL_SECTION data_mutex;

// --- Function Prototypes ---
DWORD WINAPI srtf_simulation_thread(LPVOID arg);
void calculate_metrics();
void print_gantt_chart();
void print_table();

int main() {
    // Initialize Synchronization ONCE at the start
    InitializeCriticalSection(&data_mutex);

    char user_choice;

    do {
        //RESET STATE FOR NEW RUN 
        gantt_index = 0; // Reset chart history
        // Global 'processes' array is overwritten by input loop below
        // Global 'num_processes' is overwritten by input below

        
        printf("\nSRTF Process Scheduling Simulation\n");

        // 2. Input Section
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

        // 3. Start Simulation Thread
        HANDLE hThread;
        DWORD threadID;
        
        printf("\n[System] Starting Scheduler Thread...\n");
        
        hThread = CreateThread(
            NULL,                   // Default security
            0,                      // Default stack size
            srtf_simulation_thread, // Thread function
            NULL,                   // Argument
            0,                      // Default flags
            &threadID               // Returns ID
        );

        if (hThread == NULL) {
            fprintf(stderr, "Failed to create thread. Error: %lu\n", GetLastError());
            DeleteCriticalSection(&data_mutex);
            return 1;
        }

        // 4. Wait for Simulation to Finish
        WaitForSingleObject(hThread, INFINITE);
        
        printf("[System] Simulation Completed.\n");
        CloseHandle(hThread); // Clean up thread handle

        // 5. Output Results
        calculate_metrics();
        print_table();
        print_gantt_chart();

        // 6. Ask User to Continue
        printf("\nDo you wish to Enter new processes? (y/n): ");
        
        scanf(" %c", &user_choice);

    } while (user_choice == 'y' || user_choice == 'Y');

    // Cleanup Synchronization
    DeleteCriticalSection(&data_mutex);
    
    printf("\nProgram terminated.\n");
    return 0;
}

//  Worker Thread (scheduler logic)
DWORD WINAPI srtf_simulation_thread(LPVOID arg) {
    int completed_count = 0;
    int current_time = 0;
    int prev_process_id = -1;

    // Simulation Loop
    while (completed_count < num_processes) {
        int shortest_idx = -1;
        int min_remaining = INT_MAX;

        EnterCriticalSection(&data_mutex); // Lock

        // Find process with Shortest Remaining Time
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].arrival_time <= current_time && !processes[i].is_completed) {
                if (processes[i].remaining_time < min_remaining) {
                    min_remaining = processes[i].remaining_time;
                    shortest_idx = i;
                }
                // Tie-breaker: FCFS
                if (processes[i].remaining_time == min_remaining) {
                    if (processes[i].arrival_time < processes[shortest_idx].arrival_time) {
                        shortest_idx = i;
                    }
                }
            }
        }

        if (shortest_idx != -1) {
            Process *p = &processes[shortest_idx];

            // Response Time Logic
            if (!p->is_started) {
                p->start_time = current_time;
                p->response_time = current_time - p->arrival_time;
                p->is_started = true;
            }

            // Logging for Gantt Chart
            if (p->pid != prev_process_id) {
                gantt_chart[gantt_index].time = current_time;
                gantt_chart[gantt_index].pid = p->pid;
                gantt_index++;
                prev_process_id = p->pid;
            }

            // Execute
            p->remaining_time--;
            current_time++;

            // Check completion
            if (p->remaining_time == 0) {
                p->completion_time = current_time;
                p->is_completed = true;
                completed_count++;
            }
        } else {
            // IDLE
            if (prev_process_id != 0) {
                gantt_chart[gantt_index].time = current_time;
                gantt_chart[gantt_index].pid = 0; // 0 = IDLE
                gantt_index++;
                prev_process_id = 0;
            }
            current_time++;
        }

        LeaveCriticalSection(&data_mutex); // Unlock
        
        if (TIME_UNIT_DELAY > 0) Sleep(TIME_UNIT_DELAY);
    }

    // Final log entry
    gantt_chart[gantt_index].time = current_time;
    gantt_chart[gantt_index].pid = -1; // End marker
    
    return 0;
}

// Helper Functions

void calculate_metrics() {
    for (int i = 0; i < num_processes; i++) {
        processes[i].turnaround_time = processes[i].completion_time - processes[i].arrival_time;
        processes[i].waiting_time = processes[i].turnaround_time - processes[i].burst_time;
    }
}

void print_table() {
    printf("\nSRTF Performance Results:\n");
    printf("-----------------------------------------------------------------------------\n");
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
    printf("------------------------------------------------------------------------------\n");
    printf("Average Turnaround Time = %.2f\n", avg_tat / num_processes);
    printf("Average Waiting Time    = %.2f\n", avg_wt / num_processes);
}

void print_gantt_chart() {
    printf("\nGantt Chart:\n");
    
    // Top Border
    printf(" ");
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        for(int k=0; k<duration; k++) printf("--");
        printf("-");
    }
    printf("\n|");

    // Process IDs
    for (int i = 0; i < gantt_index; i++) {
        int duration = gantt_chart[i+1].time - gantt_chart[i].time;
        
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
        if (gantt_chart[i+1].time > 9) printf("\b");
        printf("%d", gantt_chart[i+1].time);
    }
    printf("\n");
}