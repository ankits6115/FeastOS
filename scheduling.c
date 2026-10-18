

#include "os_scheduling.h"

static void copy_orders(const Order src[], Order dest[], int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
        /* Burst time is modeled as kitchen prep time + transit time */
        if (dest[i].burst_time <= 0) {
            int travel_units = (int)(dest[i].distance_km * 2.0f);
            if (travel_units < 1) travel_units = 1;
            dest[i].burst_time = dest[i].prep_time_min + travel_units;
        }
        dest[i].remaining_time = dest[i].burst_time;
        dest[i].waiting_time = 0;
        dest[i].turnaround_time = 0;
        dest[i].start_time = -1;
        dest[i].completion_time = 0;
        dest[i].response_time = -1;
    }
}

/* -------------------------------------------------------------------------
 * Algorithm 1: First-Come, First-Served (FCFS) Scheduling
 * Non-preemptive scheduling in order of arrival time.
 * ------------------------------------------------------------------------- */
void schedule_fcfs(Order orders[], int n, SchedulingMetrics *metrics) {
    if (n <= 0) return;

    Order proc[MAX_ORDERS];
    copy_orders(orders, proc, n);

    /* Sort by arrival time (stable bubble sort) */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (proc[j].arrival_time > proc[j + 1].arrival_time ||
               (proc[j].arrival_time == proc[j + 1].arrival_time && proc[j].id > proc[j + 1].id)) {
                Order tmp = proc[j];
                proc[j] = proc[j + 1];
                proc[j + 1] = tmp;
            }
        }
    }

    metrics->timeline_count = 0;
    strncpy(metrics->algorithm_name, "FCFS (First-Come First-Served)", sizeof(metrics->algorithm_name) - 1);
    metrics->total_orders = n;
    metrics->starvation_count = 0;

    int current_time = 0;
    float total_wt = 0.0f;
    float total_tat = 0.0f;
    float total_rt = 0.0f;
    int total_busy_time = 0;

    for (int i = 0; i < n; i++) {
        if (current_time < proc[i].arrival_time) {
            /* Idle period */
            if (metrics->timeline_count < MAX_TIMELINE_STEPS) {
                GanttSegment *seg = &metrics->timeline[metrics->timeline_count++];
                seg->order_id = -1;
                seg->start_time = current_time;
                seg->end_time = proc[i].arrival_time;
                snprintf(seg->label, sizeof(seg->label), "IDLE");
            }
            current_time = proc[i].arrival_time;
        }

        proc[i].start_time = current_time;
        proc[i].waiting_time = current_time - proc[i].arrival_time;
        proc[i].response_time = proc[i].waiting_time;
        proc[i].completion_time = current_time + proc[i].burst_time;
        proc[i].turnaround_time = proc[i].completion_time - proc[i].arrival_time;

        /* Record timeline segment */
        if (metrics->timeline_count < MAX_TIMELINE_STEPS) {
            GanttSegment *seg = &metrics->timeline[metrics->timeline_count++];
            seg->order_id = proc[i].id;
            seg->start_time = current_time;
            seg->end_time = proc[i].completion_time;
            snprintf(seg->label, sizeof(seg->label), "ORD-%02d", proc[i].id);
        }

        current_time = proc[i].completion_time;
        total_busy_time += proc[i].burst_time;
        total_wt += proc[i].waiting_time;
        total_tat += proc[i].turnaround_time;
        total_rt += proc[i].response_time;

        /* Check for starvation (> 3 * burst_time) */
        if (proc[i].waiting_time > (proc[i].burst_time * 3)) {
            metrics->starvation_count++;
        }
    }

    metrics->total_schedule_time = current_time;
    metrics->avg_waiting_time = total_wt / (float)n;
    metrics->avg_turnaround_time = total_tat / (float)n;
    metrics->avg_response_time = total_rt / (float)n;
    metrics->resource_utilization = (current_time > 0) ? ((float)total_busy_time / (float)current_time) * 100.0f : 0.0f;
}

/* -------------------------------------------------------------------------
 * Algorithm 2: Priority Scheduling
 * Higher priority score executes first. Ties resolved by FCFS arrival time.
 * ------------------------------------------------------------------------- */
void schedule_priority(Order orders[], int n, SchedulingMetrics *metrics) {
    if (n <= 0) return;

    Order proc[MAX_ORDERS];
    copy_orders(orders, proc, n);

    metrics->timeline_count = 0;
    strncpy(metrics->algorithm_name, "Priority Scheduling", sizeof(metrics->algorithm_name) - 1);
    metrics->total_orders = n;
    metrics->starvation_count = 0;

    int current_time = 0;
    int completed = 0;
    bool is_completed[MAX_ORDERS] = {false};
    int total_busy_time = 0;

    while (completed < n) {
        int best_idx = -1;
        int highest_priority = -1000;

        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && proc[i].arrival_time <= current_time) {
                if (proc[i].priority_score > highest_priority) {
                    highest_priority = proc[i].priority_score;
                    best_idx = i;
                } else if (proc[i].priority_score == highest_priority) {
                    if (best_idx == -1 || proc[i].arrival_time < proc[best_idx].arrival_time) {
                        best_idx = i;
                    }
                }
            }
        }

        if (best_idx == -1) {
            /* Dispatcher is idle; find next arriving order */
            int next_arr = 1000000;
            for (int i = 0; i < n; i++) {
                if (!is_completed[i] && proc[i].arrival_time < next_arr) {
                    next_arr = proc[i].arrival_time;
                }
            }

            if (metrics->timeline_count < MAX_TIMELINE_STEPS) {
                GanttSegment *seg = &metrics->timeline[metrics->timeline_count++];
                seg->order_id = -1;
                seg->start_time = current_time;
                seg->end_time = next_arr;
                snprintf(seg->label, sizeof(seg->label), "IDLE");
            }
            current_time = next_arr;
            continue;
        }

        /* Execute the selected order */
        proc[best_idx].start_time = current_time;
        proc[best_idx].waiting_time = current_time - proc[best_idx].arrival_time;
        proc[best_idx].response_time = proc[best_idx].waiting_time;
        proc[best_idx].completion_time = current_time + proc[best_idx].burst_time;
        proc[best_idx].turnaround_time = proc[best_idx].completion_time - proc[best_idx].arrival_time;

        /* Record timeline segment */
        if (metrics->timeline_count < MAX_TIMELINE_STEPS) {
            GanttSegment *seg = &metrics->timeline[metrics->timeline_count++];
            seg->order_id = proc[best_idx].id;
            seg->start_time = current_time;
            seg->end_time = proc[best_idx].completion_time;
            snprintf(seg->label, sizeof(seg->label), "ORD-%02d (P:%d)", proc[best_idx].id, proc[best_idx].priority_score);
        }

        current_time = proc[best_idx].completion_time;
        total_busy_time += proc[best_idx].burst_time;
        is_completed[best_idx] = true;
        completed++;

        /* Check for low priority starvation */
        if (proc[best_idx].waiting_time > (proc[best_idx].burst_time * 3)) {
            metrics->starvation_count++;
        }
    }

    float total_wt = 0.0f, total_tat = 0.0f, total_rt = 0.0f;
    for (int i = 0; i < n; i++) {
        total_wt += proc[i].waiting_time;
        total_tat += proc[i].turnaround_time;
        total_rt += proc[i].response_time;
    }

    metrics->total_schedule_time = current_time;
    metrics->avg_waiting_time = total_wt / (float)n;
    metrics->avg_turnaround_time = total_tat / (float)n;
    metrics->avg_response_time = total_rt / (float)n;
    metrics->resource_utilization = (current_time > 0) ? ((float)total_busy_time / (float)current_time) * 100.0f : 0.0f;
}

/* -------------------------------------------------------------------------
 * Visual ASCII Gantt Chart Display
 * ------------------------------------------------------------------------- */
void print_gantt_chart(const SchedulingMetrics *metrics) {
    if (metrics->timeline_count == 0) {
        printf("  [Empty Timeline]\n");
        return;
    }

    printf("\n  " ANSI_CYAN ANSI_BOLD "Execution Timeline (%s):" ANSI_RESET "\n", metrics->algorithm_name);

    if (metrics->timeline_count <= 6) {
        printf("   ");
        for (int i = 0; i < metrics->timeline_count; i++) {
            const GanttSegment *seg = &metrics->timeline[i];
            if (seg->order_id == -1) {
                printf("[t=%d]--IDLE--[t=%d] ", seg->start_time, seg->end_time);
            } else {
                printf("[t=%d]--%s--[t=%d] ", seg->start_time, seg->label, seg->end_time);
            }
        }
        printf("\n");
    } else {
        printf("   ");
        for (int i = 0; i < metrics->timeline_count; i++) {
            const GanttSegment *seg = &metrics->timeline[i];
            printf("[%d]%s[%d] ", seg->start_time, seg->label, seg->end_time);
            if ((i + 1) % 5 == 0 && (i + 1) < metrics->timeline_count) {
                printf("\n   ");
            }
        }
        printf("\n");
    }
}

/* -------------------------------------------------------------------------
 * Metrics Summary Printer
 * ------------------------------------------------------------------------- */
void print_metrics_summary(const SchedulingMetrics *metrics) {
    printf("\n  " ANSI_BOLD "Execution Metrics:" ANSI_RESET "\n");
    printf("    * Average Waiting Time (AWT)    : " ANSI_CYAN "%.2f time units" ANSI_RESET "\n", metrics->avg_waiting_time);
    printf("    * Average Turnaround Time (ATAT): " ANSI_CYAN "%.2f time units" ANSI_RESET "\n", metrics->avg_turnaround_time);
    printf("    * Average Response Time (ART)   : " ANSI_CYAN "%.2f time units" ANSI_RESET "\n", metrics->avg_response_time);
}

/* -------------------------------------------------------------------------
 * Comparative Performance Analysis Table
 * Evaluates FCFS vs Priority Scheduling
 * ------------------------------------------------------------------------- */
void compare_all_algorithms(Order orders[], int n) {
    if (n <= 0) {
        printf(ANSI_RED "\n  [!] No orders in database to simulate. Please create or seed orders first!\n" ANSI_RESET);
        return;
    }

    SchedulingMetrics fcfs_m, prio_m;
    schedule_fcfs(orders, n, &fcfs_m);
    schedule_priority(orders, n, &prio_m);

    /* Section 1: FCFS Execution */
    printf("\n" ANSI_BOLD ANSI_BLUE "=========================================================================================\n");
    printf(" [ALGORITHM 1/2] FIRST-COME, FIRST-SERVED (FCFS) SCHEDULING\n");
    printf("=========================================================================================" ANSI_RESET "\n");
    print_gantt_chart(&fcfs_m);
    print_metrics_summary(&fcfs_m);

    /* Section 2: Priority Execution */
    printf("\n" ANSI_BOLD ANSI_MAGENTA "=========================================================================================\n");
    printf(" [ALGORITHM 2/2] PRIORITY SCHEDULING (FOOD PERISHABILITY & URGENCY HEURISTIC)\n");
    printf("=========================================================================================" ANSI_RESET "\n");
    print_gantt_chart(&prio_m);
    print_metrics_summary(&prio_m);

    /* Section 3: Comparative Benchmark Table */
    printf("\n" ANSI_BOLD ANSI_CYAN "=========================================================================================\n");
    printf("                  FEASTOS OS SCHEDULING COMPARISON BENCHMARK (N = %d Orders)\n", n);
    printf("=========================================================================================" ANSI_RESET "\n");
    printf(" %-32s | %-16s | %-18s | %-16s\n",
           "Algorithm", "Avg Wait (WT)", "Avg Turnaround", "Avg Response");
    printf("----------------------------------+------------------+--------------------+------------------\n");

    printf(" %-32s | " ANSI_YELLOW "%-16.2f" ANSI_RESET " | " ANSI_YELLOW "%-18.2f" ANSI_RESET " | " ANSI_CYAN "%-16.2f" ANSI_RESET "\n",
           fcfs_m.algorithm_name, fcfs_m.avg_waiting_time, fcfs_m.avg_turnaround_time,
           fcfs_m.avg_response_time);

    printf(" %-32s | " ANSI_YELLOW "%-16.2f" ANSI_RESET " | " ANSI_YELLOW "%-18.2f" ANSI_RESET " | " ANSI_CYAN "%-16.2f" ANSI_RESET "\n",
           prio_m.algorithm_name, prio_m.avg_waiting_time, prio_m.avg_turnaround_time,
           prio_m.avg_response_time);
    printf("=========================================================================================\n");

    /* Section 4: Analytical summary */
    printf("\n" ANSI_BOLD ">> Automated Dispatcher Comparative Analysis:" ANSI_RESET "\n");
    printf("  1. " ANSI_BLUE "FCFS:" ANSI_RESET " Strictly orders processes by order arrival timestamp. Convoy effect occurs if long prep orders arrive early.\n");
    printf("  2. " ANSI_MAGENTA "Priority Scheduling:" ANSI_RESET " Dispatches urgent/perishable orders first (Gelato, Sizzlers, VIP). Prevents food spoilage.\n");
    if (prio_m.avg_waiting_time <= fcfs_m.avg_waiting_time) {
        printf("  => " ANSI_GREEN "Priority Scheduling achieved lowest average waiting time (%.2f vs %.2f FCFS) for high-urgency orders.\n" ANSI_RESET,
               prio_m.avg_waiting_time, fcfs_m.avg_waiting_time);
    }
}
