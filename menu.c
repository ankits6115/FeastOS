/* =========================================================================
 * Project: FeastOS - Optimized Food Delivery Application
 * Module:  Menu-Driven Command Line Application
 * Author:  Chauhan, Tanvi (240222004) - TCS-502 / TCS-503 Team Foodsters
 * ========================================================================= */

#include "cli_menu.h"
#include "cli_priority.h"
#include "os_scheduling.h"
#include "os_resource.h"
#include "dbms_storage.h"
#include "dbms_transaction.h"

static SystemResourceManager g_res_mgr;
static void sync_resources_with_db(void);

static void clean_input_string(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

void show_main_banner(void) {
    printf("\n" ANSI_BOLD ANSI_CYAN);
    printf("================================================================================\n");
    printf("  FeastOS: Optimized Food delivery Application\n");
    printf("  Courses: DBMS (TCS-503) & OS (TCS-502) | Team 205 (Foodsters)\n");
    printf("================================================================================" ANSI_RESET "\n");
}

void show_team_proposal_info(void) {
    printf("\n" ANSI_BOLD ANSI_CYAN "=========================================================================================\n");
    printf("                   PROJECT & TEAM ARCHITECTURE INFORMATION (TEAM 205)                    \n");
    printf("=========================================================================================" ANSI_RESET "\n");
    printf("  " ANSI_BOLD "Team Name:" ANSI_RESET " Foodsters (Team 205)\n");
    printf("  " ANSI_BOLD "Project  :" ANSI_RESET " FeastOS: Optimized Food delivery Application\n");
    printf("  " ANSI_BOLD "Mapping to Core CS Principles:" ANSI_RESET "\n");
    printf("    * Orders = OS Processes\n");
    printf("    * Dispatcher = Kernel Scheduler\n");
    printf("    * Drivers, Kitchen Slots & Thermal Equipment = Shared OS Hardware Resources\n");
    printf("    * Order State Lifecycle = ACID DBMS Transactions & Write-Ahead Logging (WAL)\n");
    printf("  ---------------------------------------------------------------------------------------\n");
    printf("  " ANSI_BOLD "Module Division Among Team Members:" ANSI_RESET "\n\n");

    printf("  " ANSI_YELLOW "1. Shreyash Rawat (24021627) - Team Lead | OS Architecture" ANSI_RESET "\n");
    printf("     - Module: `src/os/scheduling.c` & `include/os_scheduling.h`\n");
    printf("       * Explicit implementations of FCFS and Priority Scheduling.\n");
    printf("       * OS Metrics (Average Waiting Time, Turnaround Time, Response Time).\n");
    printf("     - Module: `src/os/resource_manager.c` & `include/os_resource.h`\n");
    printf("       * Multi-Resource Allocation (Kitchen Slots, Drivers, Insulated Bags, Cryo Boxes).\n");
    printf("       * Banker's Safety Algorithm & Resource Allocation Graph (RAG) Deadlock Engine.\n\n");

    printf("  " ANSI_YELLOW "2. Ankit Kumar Singh (240211994) | DBMS Architecture" ANSI_RESET "\n");
    printf("     - Module: `src/dbms/storage.c` & `include/dbms_storage.h`\n");
    printf("       * Multi-database architecture (Orders, Drivers, Customers, Restaurants, Dispatches, Inventory).\n");
    printf("       * Normalized storage tables across 6 dedicated files in `data/`.\n");
    printf("     - Module: `src/dbms/transaction.c` & `include/dbms_transaction.h`\n");
    printf("       * Transaction Manager guaranteeing ACID properties.\n");
    printf("       * Write-Ahead Logging (`data/wal.log`), Rollback on failure & ARIES Crash Recovery.\n\n");

    printf("  " ANSI_YELLOW "3. Tanvi Chauhan (240222004) | Priority Engine & CLI Interface" ANSI_RESET "\n");
    printf("     - Module: `src/cli/priority_calc.c` & `include/cli_priority.h`\n");
    printf("       * Priority Heuristic (Food Type/Perishability, Prep Time, Distance, Customer Tier).\n");
    printf("       * Automated Equipment Requirement determination.\n");
    printf("     - Module: `src/cli/menu.c` & `include/cli_menu.h`\n");
    printf("       * Interactive Menu-driven Command Line Application.\n");
    printf("       * Real-time dispatching dashboard and comparative benchmark visualizer.\n");
    printf("=========================================================================================\n");
}

/* -------------------------------------------------------------------------
 * Submenu: Food Order Management
 * ------------------------------------------------------------------------- */
static void menu_add_order(void) {
    printf("\n" ANSI_BOLD ANSI_CYAN "--- ENTER NEW FOOD ORDER (PROCESS SPAWN) ---" ANSI_RESET "\n");
    Order order;
    memset(&order, 0, sizeof(Order));

    order.id = db_get_next_order_id();
    printf("  Generated Order ID: " ANSI_GREEN "ORD-%02d" ANSI_RESET "\n", order.id);

    char buf[128];
    printf("  Enter Customer Name: ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(order.customer_name, buf[0] ? buf : "Guest Customer", sizeof(order.customer_name) - 1);
    }

    printf("  Enter Restaurant Name: ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(order.restaurant_name, buf[0] ? buf : "Central Kitchen", sizeof(order.restaurant_name) - 1);
    }

    printf("  Enter Food Item Description: ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(order.food_item, buf[0] ? buf : "Special Combo", sizeof(order.food_item) - 1);
    }

    /* Ask for priority factors */
    PriorityBreakdown breakdown;
    prompt_priority_parameters(
        &order.food_type,
        &order.prep_time_min,
        &order.distance_km,
        &order.delivery_tier,
        &order.equipment_needed,
        &breakdown
    );

    order.is_express = (order.delivery_tier == TIER_VIP || order.delivery_tier == TIER_EXPRESS) ? 1 : 0;
    order.priority_score = breakdown.calculated_priority;
    order.status = STATUS_PENDING;
    order.assigned_driver_id = 0;
    order.arrival_time = 0;
    int travel_units = (int)(order.distance_km * 2.0f);
    if (travel_units < 1) travel_units = 1;
    order.burst_time = order.prep_time_min + travel_units;
    order.remaining_time = order.burst_time;

    if (db_save_order(&order) == 0) {
        printf(ANSI_GREEN "\n  [SUCCESS] Order #%d stored persistently in database (data/orders.dat)!\n" ANSI_RESET, order.id);
        int dispatched = db_auto_dispatch_orders();
        if (dispatched > 0 && db_find_order_by_id(order.id, &order) == 0 && order.assigned_driver_id > 0) {
            printf(ANSI_GREEN "  [AUTO-DISPATCH] Matched and assigned immediately to Driver DRV-%d for priority delivery!\n" ANSI_RESET, order.assigned_driver_id);
        } else {
            printf(ANSI_YELLOW "  [QUEUED] Order placed in queue with Priority %d (awaiting next compatible driver).\n" ANSI_RESET, order.priority_score);
        }
    } else {
        printf(ANSI_RED "\n  [ERROR] Failed to save order to database!\n" ANSI_RESET);
    }
}

static void menu_view_orders(void) {
    /* Auto-dispatch any pending orders to available drivers first */
    db_auto_dispatch_orders();

    Order orders[MAX_ORDERS];
    int count = db_load_all_orders(orders, MAX_ORDERS);

    printf("\n" ANSI_BOLD ANSI_CYAN "======================================================================================================================\n");
    printf("                                       FEASTOS FOOD ORDERS DATABASE (PRIORITY SORTED)                                  \n");
    printf("======================================================================================================================" ANSI_RESET "\n");

    if (count == 0) {
        printf("  [Database empty. No food orders recorded. Select Option 8 to seed sample orders]\n");
        printf("======================================================================================================================\n");
        return;
    }

    /* Sort orders by priority score DESCENDING (highest priority on top; ties by ID) */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (orders[j].priority_score < orders[j + 1].priority_score ||
               (orders[j].priority_score == orders[j + 1].priority_score && orders[j].id > orders[j + 1].id)) {
                Order tmp = orders[j];
                orders[j] = orders[j + 1];
                orders[j + 1] = tmp;
            }
        }
    }

    printf(" %-6s | %-14s | %-20s | %-5s | %-5s | %-10s | %-4s | %-16s | %-18s | %-9s\n",
           "ID", "Customer", "Item", "Prep", "Dist", "Tier", "Prio", "Equipment", "Status", "Driver");
    printf("-------+----------------+----------------------+-------+-------+------------+------+------------------+--------------------+-----------\n");

    for (int i = 0; i < count; i++) {
        const char *status_color = ANSI_RESET;
        if (orders[i].status == STATUS_PENDING) status_color = ANSI_YELLOW;
        else if (orders[i].status == STATUS_OUT_FOR_DELIVERY) status_color = ANSI_BLUE;
        else if (orders[i].status == STATUS_DELIVERED) status_color = ANSI_GREEN;

        char drv_label[32];
        if (orders[i].assigned_driver_id > 0) {
            snprintf(drv_label, sizeof(drv_label), ANSI_GREEN "DRV-%d" ANSI_RESET, orders[i].assigned_driver_id);
        } else {
            snprintf(drv_label, sizeof(drv_label), ANSI_YELLOW "Waiting" ANSI_RESET);
        }

        printf(" ORD-%02d | %-14.14s | %-20.20s | %2d m | %4.1f | %-10.10s | " ANSI_MAGENTA "%3d" ANSI_RESET "  | %-16.16s | %s%-18.18s" ANSI_RESET " | %s\n",
               orders[i].id, orders[i].customer_name, orders[i].food_item,
               orders[i].prep_time_min, orders[i].distance_km,
               delivery_tier_to_string(orders[i].delivery_tier),
               orders[i].priority_score,
               equipment_to_string(orders[i].equipment_needed),
               status_color, order_status_to_string(orders[i].status),
               drv_label);
    }
    printf("======================================================================================================================\n");
    printf("  Total Orders in Database: %d | Sorted: Highest Priority First | Multi-DB Tables Active\n", count);
    printf("  [Tip: Select Option [7] from main menu to view all 6 databases]\n");
}

/* -------------------------------------------------------------------------
 * Submenu: Delivery Driver Fleet Management
 * ------------------------------------------------------------------------- */
static void menu_add_driver(void) {
    printf("\n" ANSI_BOLD ANSI_CYAN "--- REGISTER NEW DELIVERY DRIVER (HARDWARE RESOURCE) ---" ANSI_RESET "\n");
    Driver driver;
    memset(&driver, 0, sizeof(Driver));

    driver.id = db_get_next_driver_id();
    printf("  Generated Driver ID: " ANSI_GREEN "DRV-%d" ANSI_RESET "\n", driver.id);

    char buf[128];
    printf("  Enter Driver Full Name: ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(driver.name, buf[0] ? buf : "Delivery Partner", sizeof(driver.name) - 1);
    }

    printf("  Enter Contact Phone: ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(driver.phone, buf[0] ? buf : "+91-9999900000", sizeof(driver.phone) - 1);
    }

    printf("  Enter Vehicle Type (e.g. Motorcycle, Electric Scooter, Van): ");
    if (fgets(buf, sizeof(buf), stdin)) {
        clean_input_string(buf);
        strncpy(driver.vehicle, buf[0] ? buf : "Motorcycle", sizeof(driver.vehicle) - 1);
    }

    printf("  Select Delivery Equipment Equipped on Vehicle:\n");
    printf("    [0] Standard Delivery Bag (Room-temp / Dry goods only)\n");
    printf("    [1] Thermal Insulated Bag (For Hot Meals / Soups)\n");
    printf("    [2] Cold Storage Container (For Ice Creams / Cryogenic)\n");
    printf("  Enter choice (0-2): ");
    int eq = 0;
    if (fgets(buf, sizeof(buf), stdin)) {
        eq = atoi(buf);
        if (eq < 0 || eq > 2) eq = 0;
    }
    driver.equipment_capability = (EquipmentType)eq;
    driver.status = DRIVER_AVAILABLE;
    driver.current_order_id = 0;
    driver.total_deliveries = 0;

    if (db_save_driver(&driver) == 0) {
        printf(ANSI_GREEN "\n  [SUCCESS] Driver #%d (%s) enrolled and saved to database (data/drivers.dat)!\n" ANSI_RESET,
               driver.id, driver.name);
        int dispatched = db_auto_dispatch_orders();
        if (dispatched > 0 && db_find_driver_by_id(driver.id, &driver) == 0 && driver.current_order_id > 0) {
            printf(ANSI_GREEN "  [AUTO-DISPATCH] Driver immediately matched and assigned to pending order ORD-%02d!\n" ANSI_RESET,
                   driver.current_order_id);
        } else {
            printf(ANSI_CYAN "  [STATUS] Driver is active and available in fleet for upcoming orders.\n" ANSI_RESET);
        }
    } else {
        printf(ANSI_RED "\n  [ERROR] Failed to save driver record to database!\n" ANSI_RESET);
    }
}

static void menu_view_drivers(void) {
    /* Auto-dispatch any pending orders to available drivers first */
    db_auto_dispatch_orders();

    Driver drivers[MAX_DRIVERS];
    int count = db_load_all_drivers(drivers, MAX_DRIVERS);

    printf("\n" ANSI_BOLD ANSI_CYAN "=========================================================================================\n");
    printf("                             FEASTOS DELIVERY DRIVER FLEET                              \n");
    printf("=========================================================================================" ANSI_RESET "\n");

    if (count == 0) {
        printf("  [Database empty. No drivers registered. Select Option 8 to seed sample fleet]\n");
        printf("=========================================================================================\n");
        return;
    }

    printf(" %-7s | %-18s | %-14s | %-18s | %-12s | %-8s\n",
           "ID", "Name", "Phone", "Equipment", "Status", "Active");
    printf("---------+--------------------+----------------+--------------------+--------------+---------\n");

    for (int i = 0; i < count; i++) {
        const char *st_color = (drivers[i].status == DRIVER_AVAILABLE) ? ANSI_GREEN :
                               ((drivers[i].status == DRIVER_BUSY) ? ANSI_YELLOW : ANSI_RED);

        char active_str[16];
        if (drivers[i].current_order_id > 0) {
            snprintf(active_str, sizeof(active_str), "ORD-%02d", drivers[i].current_order_id);
        } else {
            snprintf(active_str, sizeof(active_str), "None");
        }

        printf(" DRV-%d | %-18.18s | %-14.14s | %-18.18s | %s%-12.12s" ANSI_RESET " | %-8s\n",
               drivers[i].id, drivers[i].name, drivers[i].phone,
               equipment_to_string(drivers[i].equipment_capability),
               st_color, driver_status_to_string(drivers[i].status),
               active_str);
    }
    printf("=========================================================================================\n");
    printf("  Total Drivers in Fleet: %d\n", count);
}

/* -------------------------------------------------------------------------
 * Submenu: OS Scheduling Algorithms (Shreyash Rawat)
 * ------------------------------------------------------------------------- */
static void menu_os_scheduling(void) {
    Order orders[MAX_ORDERS];
    int count = db_load_all_orders(orders, MAX_ORDERS);

    if (count == 0) {
        printf(ANSI_RED "\n  [!] No orders found in database to schedule. Please seed or add orders first!\n" ANSI_RESET);
        return;
    }

    printf("\n" ANSI_BOLD ANSI_CYAN "=== OS Dispatcher: Automated Multi-Algorithm Execution (TCS-502) ===" ANSI_RESET "\n");
    printf("  Automatically applying algorithms on the active order set:\n");
    printf("    * 1. First-Come, First-Served (FCFS)\n");
    printf("    * 2. Priority Scheduling (Food Urgency & Perishability Score)\n\n");

    /* Automatically executes FCFS and Priority on its own */
    compare_all_algorithms(orders, count);
}

/* -------------------------------------------------------------------------
 * Submenu: Multi-Resource Allocation & Deadlock Engine (Shreyash Rawat)
 * ------------------------------------------------------------------------- */
static void menu_resource_deadlock(void) {
    char buf[32];
    int choice = -1;

    while (choice != 0) {
        printf("\n" ANSI_BOLD ANSI_CYAN "=== Multi-Resource Allocation & Deadlock Engine (Shreyash Rawat - TCS-502) ===" ANSI_RESET "\n");
        printf("  [1] View Multi-Resource Allocation State\n");
        printf("  [2] Run Banker's Safety Algorithm (Check Safe State & Safe Sequence)\n");
        printf("  [3] Simulate Resource Allocation Request (Safe vs Unsafe Rejection)\n");
        printf("  [4] Run Resource Allocation Graph (RAG) Deadlock Detection\n");
        printf("  [5] " ANSI_RED ANSI_BOLD "Inject Deadlock Scenario" ANSI_RESET " (Simulate Circular Wait on Kitchen & Bags)\n");
        printf("  [6] " ANSI_GREEN "Reset / Restore Resources" ANSI_RESET " (Break Deadlock & Sync with DB)\n");
        printf("  [0] Return to Main Menu\n");
        printf("  Enter choice (0-6): ");

        if (!fgets(buf, sizeof(buf), stdin)) break;
        choice = atoi(buf);

        if (choice == 1) {
            resource_manager_print_state(&g_res_mgr);
        } else if (choice == 2) {
            int safe_seq[MAX_ORDERS];
            bool is_safe = banker_check_safe_state(&g_res_mgr, safe_seq);
            if (is_safe) {
                printf(ANSI_GREEN "\n  [SAFE STATE CONFIRMED] System can fulfill all orders without deadlock!\n" ANSI_RESET);
                printf("  Safe Execution Sequence: < ");
                for (int i = 0; i < g_res_mgr.active_processes; i++) {
                    printf(ANSI_CYAN "ORD-%02d " ANSI_RESET, safe_seq[i]);
                }
                printf(">\n");
            } else {
                printf(ANSI_RED "\n  [ALERT: UNSAFE STATE DETECTED!] System cannot guarantee safe completion.\n" ANSI_RESET);
                printf("  Banker's Algorithm could not find any safe sequence. Deadlock inevitable!\n");
            }
        } else if (choice == 3) {
            printf("\n  Simulating resource request for active process 0 (ORD-%02d):\n", g_res_mgr.process_ids[0]);
            printf("  Requesting 1 Kitchen Slot, 1 Driver, 1 Equipment:\n");
            int req[NUM_RESOURCE_TYPES] = {1, 1, 1, 0};
            banker_request_resources(&g_res_mgr, 0, req);
        } else if (choice == 4) {
            int deadlocked[MAX_ORDERS];
            int count = 0;
            bool has_deadlock = rag_detect_deadlock(&g_res_mgr, deadlocked, &count);
            if (has_deadlock) {
                printf(ANSI_RED "\n  =================================================================\n");
                printf("   [DEADLOCK DETECTED VIA RAG] Circular Wait Cycle Identified!    \n");
                printf("  =================================================================\n" ANSI_RESET);
                printf("  %d processes are permanently deadlocked and cannot proceed:\n", count);
                for (int i = 0; i < count; i++) {
                    printf("    * Process " ANSI_YELLOW "ORD-%02d" ANSI_RESET " is stalled in wait-for cycle.\n", deadlocked[i]);
                }
                printf("\n  " ANSI_BOLD "Circular Dependency Graph:" ANSI_RESET "\n");
                printf("    (ORD-01 holds Kitchen Slot) ---> [Waiting for Insulated Bag held by ORD-02]\n");
                printf("    (ORD-02 holds Insulated Bag) ---> [Waiting for Kitchen Slot held by ORD-01]\n");
                printf("  " ANSI_YELLOW "Resolution:" ANSI_RESET " Select Option [6] to preempt resources and break deadlock.\n");
            } else {
                printf(ANSI_GREEN "\n  [RAG Cycle Check] No circular wait cycles detected in Resource Allocation Graph.\n" ANSI_RESET);
                printf("  The system is deadlock-free.\n");
            }
        } else if (choice == 5) {
            resource_manager_inject_deadlock(&g_res_mgr);
            printf(ANSI_RED "\n  =================================================================\n");
            printf("   [DEADLOCK SCENARIO INJECTED] Competition for Shared Equipment! \n");
            printf("  =================================================================\n" ANSI_RESET);
            printf("  Two orders are contending for limited kitchen and delivery equipment:\n");
            printf("    1. " ANSI_BOLD "ORD-01" ANSI_RESET " (Hot Pizza)  : Holds [Kitchen Slot=1], Needs [Insulated Bag=1]\n");
            printf("    2. " ANSI_BOLD "ORD-02" ANSI_RESET " (Hot Sizzler): Holds [Insulated Bag=1], Needs [Kitchen Slot=1]\n");
            printf("  Remaining Available Resources: Kitchen=[0], Insulated Bags=[0]\n\n");
            printf(ANSI_YELLOW "  >> Circular wait is now active: neither process will release its resource!\n" ANSI_RESET);
            printf("  -> Now select Option [4] to run RAG Deadlock Detection,\n");
            printf("     or Option [2] to verify Banker's Unsafe State Alert.\n");
        } else if (choice == 6) {
            sync_resources_with_db();
            printf(ANSI_GREEN "\n  [RECOVERY COMPLETE] Deadlock broken! Resources restored and re-synced with DB.\n" ANSI_RESET);
        } else if (choice == 0) {
            break;
        } else {
            printf(ANSI_RED "  Invalid choice!\n" ANSI_RESET);
        }
    }
}

/* -------------------------------------------------------------------------
 * Submenu: View All Databases (Ankit Kumar Singh)
 * Displays all 6 persistent databases in FeastOS:
 *   1. Customers Database        (data/customers.dat)
 *   2. Restaurants Database      (data/restaurants.dat)
 *   3. Food Orders Database      (data/orders.dat)
 *   4. Delivery Drivers Database (data/drivers.dat)
 *   5. Dispatches Database       (data/dispatches.dat)
 *   6. Equipment Inventory DB    (data/inventory.dat)
 * ------------------------------------------------------------------------- */
static void menu_view_all_databases(void) {
    /* Auto-dispatch any pending orders to available drivers first */
    db_auto_dispatch_orders();

    printf("\n" ANSI_BOLD ANSI_CYAN "======================================================================================================================\n");
    printf("                                      FEASTOS MULTI-DATABASE ARCHITECTURE VIEWER                                      \n");
    printf("======================================================================================================================" ANSI_RESET "\n");

    /* 1. Customers DB */
    Customer custs[MAX_CUSTOMERS];
    int c_count = db_load_all_customers(custs, MAX_CUSTOMERS);
    printf("\n  " ANSI_YELLOW "[DATABASE 1/6] CUSTOMER DATABASE (`data/customers.dat`) - Total Records: %d" ANSI_RESET "\n", c_count);
    printf("  ID       | Name                 | Phone           | Address                        | Delivery Tier      | Orders\n");
    printf(" ----------+----------------------+-----------------+--------------------------------+--------------------+--------\n");
    for (int i = 0; i < c_count; i++) {
        printf("  CUST-%d | %-20.20s | %-15.15s | %-30.30s | %-18.18s | %-6d\n",
               custs[i].id, custs[i].name, custs[i].phone, custs[i].address,
               delivery_tier_to_string(custs[i].tier), custs[i].total_orders);
    }

    /* 2. Restaurants DB */
    Restaurant rests[MAX_RESTAURANTS];
    int r_count = db_load_all_restaurants(rests, MAX_RESTAURANTS);
    printf("\n  " ANSI_YELLOW "[DATABASE 2/6] RESTAURANT & KITCHEN DATABASE (`data/restaurants.dat`) - Total Records: %d" ANSI_RESET "\n", r_count);
    printf("  ID       | Name                       | Cuisine                    | Kitchen Slots | Avg Prep | Rating\n");
    printf(" ----------+----------------------------+----------------------------+---------------+----------+--------\n");
    for (int i = 0; i < r_count; i++) {
        printf("  REST-%d | %-26.26s | %-26.26s | %d/%d Avail   | %2d mins  | %.1f \n",
               rests[i].id, rests[i].name, rests[i].cuisine,
               rests[i].available_slots, rests[i].total_kitchen_slots,
               rests[i].avg_prep_time_min, rests[i].rating);
    }

    /* 3. Food Orders DB */
    Order orders[MAX_ORDERS];
    int o_count = db_load_all_orders(orders, MAX_ORDERS);
    /* Sort orders by priority score descending */
    for (int i = 0; i < o_count - 1; i++) {
        for (int j = 0; j < o_count - i - 1; j++) {
            if (orders[j].priority_score < orders[j + 1].priority_score ||
               (orders[j].priority_score == orders[j + 1].priority_score && orders[j].id > orders[j + 1].id)) {
                Order tmp = orders[j];
                orders[j] = orders[j + 1];
                orders[j + 1] = tmp;
            }
        }
    }
    printf("\n  " ANSI_YELLOW "[DATABASE 3/6] FOOD ORDERS DATABASE (`data/orders.dat`) - Total Records: %d" ANSI_RESET "\n", o_count);
    printf("  ID     | Customer       | Item                 | Prep  | Dist  | Tier       | Prio | Status             | Driver   \n");
    printf(" --------+----------------+----------------------+-------+-------+------------+------+--------------------+-----------\n");
    for (int i = 0; i < o_count; i++) {
        char drv_label[32];
        if (orders[i].assigned_driver_id > 0) snprintf(drv_label, sizeof(drv_label), ANSI_GREEN "DRV-%d" ANSI_RESET, orders[i].assigned_driver_id);
        else snprintf(drv_label, sizeof(drv_label), ANSI_YELLOW "Waiting" ANSI_RESET);

        const char *status_color = (orders[i].status == STATUS_OUT_FOR_DELIVERY) ? ANSI_BLUE :
                                   ((orders[i].status == STATUS_DELIVERED) ? ANSI_GREEN : ANSI_YELLOW);

        printf("  ORD-%02d | %-14.14s | %-20.20s | %2d m | %4.1f | %-10.10s | " ANSI_MAGENTA "%3d" ANSI_RESET "  | %s%-18.18s" ANSI_RESET " | %s\n",
               orders[i].id, orders[i].customer_name, orders[i].food_item,
               orders[i].prep_time_min, orders[i].distance_km,
               delivery_tier_to_string(orders[i].delivery_tier),
               orders[i].priority_score,
               status_color, order_status_to_string(orders[i].status),
               drv_label);
    }

    /* 4. Delivery Driver Fleet DB */
    Driver drivers[MAX_DRIVERS];
    int d_count = db_load_all_drivers(drivers, MAX_DRIVERS);
    printf("\n  " ANSI_YELLOW "[DATABASE 4/6] DELIVERY DRIVER FLEET DATABASE (`data/drivers.dat`) - Total Records: %d" ANSI_RESET "\n", d_count);
    printf("  ID      | Name               | Phone          | Equipment          | Status       | Active Order\n");
    printf(" ---------+--------------------+----------------+--------------------+--------------+--------------\n");
    for (int i = 0; i < d_count; i++) {
        const char *st_color = (drivers[i].status == DRIVER_AVAILABLE) ? ANSI_GREEN :
                               ((drivers[i].status == DRIVER_BUSY) ? ANSI_YELLOW : ANSI_RED);
        char active_str[16];
        if (drivers[i].current_order_id > 0) snprintf(active_str, sizeof(active_str), "ORD-%02d", drivers[i].current_order_id);
        else snprintf(active_str, sizeof(active_str), "None");

        printf("  DRV-%d | %-18.18s | %-14.14s | %-18.18s | %s%-12.12s" ANSI_RESET " | %-12s\n",
               drivers[i].id, drivers[i].name, drivers[i].phone,
               equipment_to_string(drivers[i].equipment_capability),
               st_color, driver_status_to_string(drivers[i].status),
               active_str);
    }

    /* 5. Dispatches DB */
    DispatchRecord disps[MAX_DISPATCHES];
    int disp_count = db_load_all_dispatches(disps, MAX_DISPATCHES);
    printf("\n  " ANSI_YELLOW "[DATABASE 5/6] LOGISTICS & DISPATCH DATABASE (`data/dispatches.dat`) - Total Records: %d" ANSI_RESET "\n", disp_count);
    printf("  Disp ID | Order  | Driver | Equipment Allocated        | Status               | Dispatch Time\n");
    printf(" ---------+--------+--------+----------------------------+----------------------+--------------------\n");
    for (int i = 0; i < disp_count; i++) {
        char drv_str[16];
        if (disps[i].driver_id > 0) snprintf(drv_str, sizeof(drv_str), "DRV-%d", disps[i].driver_id);
        else snprintf(drv_str, sizeof(drv_str), "Pending");
        printf("  DSP-%03d | ORD-%02d | %-6s | %-26.26s | %-20.20s | %s\n",
               disps[i].dispatch_id, disps[i].order_id, drv_str,
               equipment_to_string(disps[i].equipment_used),
               order_status_to_string(disps[i].status),
               disps[i].dispatch_time);
    }

    /* 6. Equipment Inventory DB */
    EquipmentInventory invs[MAX_INVENTORY];
    int inv_count = db_load_all_inventory(invs, MAX_INVENTORY);
    printf("\n  " ANSI_YELLOW "[DATABASE 6/6] EQUIPMENT INVENTORY DATABASE (`data/inventory.dat`) - Total Records: %d" ANSI_RESET "\n", inv_count);
    printf("  Item ID | Serial No    | Equipment Type             | Status     | Assigned Order | Assigned Driver\n");
    printf(" ---------+--------------+----------------------------+------------+----------------+----------------\n");
    for (int i = 0; i < inv_count; i++) {
        char ord_str[16], drv_str[16];
        if (invs[i].assigned_to_order_id > 0) snprintf(ord_str, sizeof(ord_str), "ORD-%02d", invs[i].assigned_to_order_id);
        else snprintf(ord_str, sizeof(ord_str), "None");
        if (invs[i].assigned_to_driver_id > 0) snprintf(drv_str, sizeof(drv_str), "DRV-%d", invs[i].assigned_to_driver_id);
        else snprintf(drv_str, sizeof(drv_str), "None");

        printf("  INV-%d | %-12.12s | %-26.26s | %-10s | %-14s | %-14s\n",
               invs[i].item_id, invs[i].serial_no, equipment_to_string(invs[i].type),
               invs[i].is_assigned ? ANSI_RED "IN USE" ANSI_RESET : ANSI_GREEN "FREE" ANSI_RESET,
               ord_str, drv_str);
    }
    printf("======================================================================================================================\n");
}


/* -------------------------------------------------------------------------
 * Setup resource manager with current DB state
 * ------------------------------------------------------------------------- */
static void sync_resources_with_db(void) {
    Driver drivers[MAX_DRIVERS];
    int d_count = db_load_all_drivers(drivers, MAX_DRIVERS);
    int avail_drivers = 0;
    int bags = 0;
    int cold_boxes = 0;

    for (int i = 0; i < d_count; i++) {
        if (drivers[i].status == DRIVER_AVAILABLE) avail_drivers++;
        if (drivers[i].equipment_capability == EQUIP_INSULATED_BAG) bags++;
        if (drivers[i].equipment_capability == EQUIP_COLD_CONTAINER) cold_boxes++;
    }

    resource_manager_init(&g_res_mgr, 3 /* 3 kitchen slots */, avail_drivers > 0 ? avail_drivers : 4, bags + 2, cold_boxes + 2);

    Order orders[MAX_ORDERS];
    int o_count = db_load_all_orders(orders, MAX_ORDERS);
    int active_p = 0;

    for (int i = 0; i < o_count && active_p < 5; i++) {
        if (orders[i].status == STATUS_PENDING || orders[i].status == STATUS_PREPARING) {
            g_res_mgr.process_ids[active_p] = orders[i].id;
            /* Max claim */
            g_res_mgr.max_claim[active_p][RES_KITCHEN_SLOT] = 1;
            g_res_mgr.max_claim[active_p][RES_DRIVER] = 1;
            g_res_mgr.max_claim[active_p][RES_INSULATED_BAG] = (orders[i].equipment_needed == EQUIP_INSULATED_BAG) ? 1 : 0;
            g_res_mgr.max_claim[active_p][RES_COLD_CONTAINER] = (orders[i].equipment_needed == EQUIP_COLD_CONTAINER) ? 1 : 0;

            /* Current allocation */
            if (orders[i].status == STATUS_PREPARING) {
                g_res_mgr.allocation[active_p][RES_KITCHEN_SLOT] = 1;
            }

            /* Need = Max - Allocation */
            for (int r = 0; r < NUM_RESOURCE_TYPES; r++) {
                g_res_mgr.need[active_p][r] = g_res_mgr.max_claim[active_p][r] - g_res_mgr.allocation[active_p][r];
            }
            active_p++;
        }
    }
    g_res_mgr.active_processes = active_p;
}

/* -------------------------------------------------------------------------
 * Main CLI Event Loop
 * ------------------------------------------------------------------------- */
void run_cli_application(void) {
    db_storage_init();
    txn_manager_init();
    sync_resources_with_db();

    char line_buf[64];
    int choice = -1;

    while (choice != 0) {
        show_main_banner();
        printf("  " ANSI_BOLD "MAIN NAVIGATION MENU:" ANSI_RESET "\n");
        printf("   " ANSI_GREEN "[1]" ANSI_RESET " Enter New Food Order\n");
        printf("   " ANSI_GREEN "[2]" ANSI_RESET " View All Food Orders in Database\n");
        printf("   " ANSI_GREEN "[3]" ANSI_RESET " Register New Delivery Driver\n");
        printf("   " ANSI_GREEN "[4]" ANSI_RESET " View Delivery Driver Fleet\n");
        printf("   " ANSI_CYAN  "[5]" ANSI_RESET " OS Scheduling Simulator\n");
        printf("   " ANSI_CYAN  "[6]" ANSI_RESET " OS Multi-Resource Allocation & Deadlock Engine\n");
        printf("   " ANSI_YELLOW"[7]" ANSI_RESET " View All Databases\n");
        printf("   " ANSI_MAGENTA"[8]" ANSI_RESET " Seed Realistic Sample Dataset\n");
        printf("   " ANSI_RED   "[0]" ANSI_RESET " Exit FeastOS\n");
        printf("  ----------------------------------------------------------------------------------\n");
        printf("  Enter your selection [0-8]: ");

        if (!fgets(line_buf, sizeof(line_buf), stdin)) break;
        choice = atoi(line_buf);

        switch (choice) {
            case 1:
                menu_add_order();
                sync_resources_with_db();
                break;
            case 2:
                menu_view_orders();
                break;
            case 3:
                menu_add_driver();
                sync_resources_with_db();
                break;
            case 4:
                menu_view_drivers();
                break;
            case 5:
                menu_os_scheduling();
                break;
            case 6:
                sync_resources_with_db();
                menu_resource_deadlock();
                break;
            case 7:
                menu_view_all_databases();
                break;
            case 8:
            case 9:
                db_seed_sample_data();
                sync_resources_with_db();
                break;
            case 0:
                printf("\n  " ANSI_GREEN "Thank you for exploring FeastOS! Exiting cleanly..." ANSI_RESET "\n\n");
                break;
            default:
                printf(ANSI_RED "\n  [!] Invalid choice '%s'. Please enter a number between 0 and 8.\n" ANSI_RESET, line_buf);
                break;
        }

        if (choice != 0) {
            printf("\n  " ANSI_YELLOW "Press ENTER to return to Main Menu..." ANSI_RESET);
            fgets(line_buf, sizeof(line_buf), stdin);
        }
    }
}
