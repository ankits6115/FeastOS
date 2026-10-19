#ifndef OS_RESOURCE_H
#define OS_RESOURCE_H

#include "common.h"

/* =========================================================================
 * Module: Multi-Resource Allocation & Deadlock Engine
 * Author: Shreyash Rawat (24021627) - Team Lead (TCS-502 OS)
 * Description:
 *   Manages multi-resource allocation for FeastOS:
 *     - Resource Types:
 *         R0: Kitchen Slots (Limited cooking capacity)
 *         R1: Delivery Drivers (Limited fleet)
 *         R2: Thermal Insulated Bags (Limited hot equipment)
 *         R3: Cold Storage Containers (Limited cryogenic containers)
 *   Provides:
 *     - Banker's Safety Algorithm to prevent unsafe states.
 *     - Resource Allocation Graph (RAG) cycle detection for deadlock detection.
 * ========================================================================= */

#define NUM_RESOURCE_TYPES 4
#define RES_KITCHEN_SLOT    0
#define RES_DRIVER          1
#define RES_INSULATED_BAG   2
#define RES_COLD_CONTAINER  3

typedef struct {
    int total[NUM_RESOURCE_TYPES];
    int available[NUM_RESOURCE_TYPES];
    int max_claim[MAX_ORDERS][NUM_RESOURCE_TYPES];
    int allocation[MAX_ORDERS][NUM_RESOURCE_TYPES];
    int need[MAX_ORDERS][NUM_RESOURCE_TYPES];
    int active_processes;
    int process_ids[MAX_ORDERS];
} SystemResourceManager;

/* Initializes system resource pools */
void resource_manager_init(SystemResourceManager *mgr, int kitchen_slots, int drivers, int bags, int cold_boxes);

/* Banker's Safety Algorithm */
bool banker_check_safe_state(const SystemResourceManager *mgr, int safe_sequence[]);

/* Attempt resource request using Banker's Algorithm */
bool banker_request_resources(SystemResourceManager *mgr, int process_idx, int request[NUM_RESOURCE_TYPES]);

/* Release allocated resources */
void resource_manager_release(SystemResourceManager *mgr, int process_idx);

/* Resource Allocation Graph (RAG) Deadlock Detection */
bool rag_detect_deadlock(const SystemResourceManager *mgr, int deadlocked_processes[], int *deadlocked_count);

/* Injects a realistic circular wait deadlock situation into the resource manager */
void resource_manager_inject_deadlock(SystemResourceManager *mgr);

/* Display Resource state table */
void resource_manager_print_state(const SystemResourceManager *mgr);

#endif /* OS_RESOURCE_H */
