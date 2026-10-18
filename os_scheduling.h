#ifndef OS_SCHEDULING_H
#define OS_SCHEDULING_H

#include "common.h"

/* =========================================================================
 * Module: OS Scheduling Algorithms (FCFS and Priority Scheduling)
 * Author: Shreyash Rawat (24021627) - Team Lead (TCS-502 OS)
 * Description:
 *   Implements process scheduling models for food orders treated as OS tasks:
 *     1. First-Come, First-Served (FCFS)
 *     2. Food Urgency & Perishability Priority Scheduling
 *   Calculates standard operating system performance metrics:
 *     - Average Waiting Time (AWT)
 *     - Average Turnaround Time (ATAT)
 *     - Average Response Time (ART)
 * ========================================================================= */

#define MAX_TIMELINE_STEPS 500

typedef struct {
    int order_id;
    int start_time;
    int end_time;
    char label[32];
} GanttSegment;

typedef struct {
    char algorithm_name[32];
    int total_orders;
    float avg_waiting_time;
    float avg_turnaround_time;
    float avg_response_time;
    float resource_utilization;
    int starvation_count;
    int total_schedule_time;
    
    GanttSegment timeline[MAX_TIMELINE_STEPS];
    int timeline_count;
} SchedulingMetrics;

/* Core Scheduling Functions */
void schedule_fcfs(Order orders[], int n, SchedulingMetrics *metrics);
void schedule_priority(Order orders[], int n, SchedulingMetrics *metrics);

/* Comparative Analysis and Visualizers (FCFS vs Priority) */
void compare_all_algorithms(Order orders[], int n);
void print_gantt_chart(const SchedulingMetrics *metrics);
void print_metrics_summary(const SchedulingMetrics *metrics);

#endif /* OS_SCHEDULING_H */
