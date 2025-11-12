#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXPID 32

typedef struct {
	char pid[MAXPID];
	int arrival;
	int burst;
	int rem;       /* remaining time used by simulator */
	int completion;/* -1 if not provided / not finished */
	int given_completion; /* flag: user provided completion */
} Process;

int main(void) {
	int n;
	printf("Shortest Remaining Time First (SRTF) scheduling simulation\n");
	printf("Enter number of processes: ");
	if (scanf("%d", &n) != 1 || n <= 0) {
		printf("Invalid number of processes.\n");
		return 1;
	}

	Process *p = malloc(sizeof(Process) * n);
	if (!p) return 2;

	for (int i = 0; i < n; ++i) {
		int rem_inp, comp_inp;
		printf("\nProcess %d:\n", i + 1);
		printf("  Enter Process ID (no spaces): ");
		scanf("%s", p[i].pid);
		printf("  Arrival time (int): ");
		scanf("%d", &p[i].arrival);
		printf("  CPU burst time (int): ");
		scanf("%d", &p[i].burst);
		printf("  Remaining burst time (-1 to use full burst): ");
		scanf("%d", &rem_inp);
		printf("  Completion time (-1 to compute): ");
		scanf("%d", &comp_inp);

		p[i].rem = (rem_inp >= 0) ? rem_inp : p[i].burst;
		if (comp_inp >= 0) {
			p[i].completion = comp_inp;
			p[i].given_completion = 1;
			/* If user supplied completion, treat process as already finished */
			p[i].rem = 0;
		} else {
			p[i].completion = -1;
			p[i].given_completion = 0;
		}
	}

	/* Find start time (min arrival) */
	int time = p[0].arrival;
	for (int i = 1; i < n; ++i) if (p[i].arrival < time) time = p[i].arrival;

	/* Simulation: preemptive, unit-time ticks */
	int finished = 0;
	int *timeline = NULL; /* store executed process index per time unit */
	int tlen = 0;
	int max_iter = 1000000; /* guard against infinite loops */
	while (finished < n && max_iter--) {
		int idx = -1;
		int min_rem = 1<<30;
		for (int i = 0; i < n; ++i) {
			if (!p[i].given_completion && p[i].arrival <= time && p[i].rem > 0) {
				if (p[i].rem < min_rem) {
					min_rem = p[i].rem;
					idx = i;
				}
			}
		}

		if (idx == -1) {
			/* nothing ready: advance time to next arrival or by 1 */
			int next_arr = 1<<30;
			for (int i = 0; i < n; ++i) if (p[i].arrival > time) next_arr = (p[i].arrival < next_arr) ? p[i].arrival : next_arr;
			if (next_arr != (1<<30)) time = next_arr;
			else time++;
			continue;
		}
		/* execute 1 time unit */
		p[idx].rem -= 1;
		/* record timeline */
		int *tmp = realloc(timeline, sizeof(int) * (tlen + 1));
		if (!tmp) break;
		timeline = tmp;
		timeline[tlen++] = idx;
		time += 1;

		if (p[idx].rem == 0) {
			p[idx].completion = time;
			finished++;
		}
	}

	if (max_iter <= 0) fprintf(stderr, "Simulation aborted: too many iterations.\n");

	/* Compute and print results */
	printf("\nResults:\n");
	printf("%-8s %-8s %-8s %-12s %-12s %-12s %-8s\n", "PID", "Arrival", "Burst", "RemainingInit", "Completion", "Turnaround", "Waiting");
	double total_tat = 0, total_wt = 0;
	for (int i = 0; i < n; ++i) {
		int completion = p[i].completion;
		int turnaround = (completion >= 0) ? (completion - p[i].arrival) : -1;
		int waiting = (turnaround >= 0) ? (turnaround - p[i].burst) : -1;
		int rem_init = (p[i].burst - ( (p[i].given_completion) ? p[i].burst : (p[i].rem==0 ? 0 : p[i].rem) ));
		/* rem_init computed awkwardly; better to show the remaining that user provided or original burst */
		int user_rem = p[i].burst; /* default */
		/* If user originally entered a remaining (less than burst), we cannot recover it now; instead show current rem when started */
		/* To simplify, show original burst as "RemainingInit" when user didn't provide remaining; otherwise show the remaining they provided. */
		/* But we didn't store original remaining input separately. To keep output meaningful, show the burst as RemainingInit if not given. */
		int display_rem = p[i].burst;
		if (p[i].given_completion) display_rem = 0;

		printf("%-8s %-8d %-8d %-12d %-12d %-12d %-8d\n",
			   p[i].pid, p[i].arrival, p[i].burst, display_rem, completion, turnaround, waiting);
		if (turnaround >= 0) { total_tat += turnaround; total_wt += waiting; }
	}

	int completed_count = 0;
	for (int i = 0; i < n; ++i) if (p[i].completion >= 0) completed_count++;
	if (completed_count > 0) {
		printf("\nAverage Turnaround Time = %.2f\n", total_tat / completed_count);
		printf("Average Waiting Time    = %.2f\n", total_wt / completed_count);
	}

	/* Print simple Gantt chart */
	if (tlen > 0) {
		printf("\nGantt timeline (per time unit):\n");
		for (int i = 0; i < tlen; ++i) {
			printf("|%s", p[timeline[i]].pid);
		}
		printf("|\n0");
		for (int i = 1; i <= tlen; ++i) printf(" %d", i);
		printf("\n");
	}

	free(p);
	free(timeline);
	return 0;
}

