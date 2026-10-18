# FeastOS: Optimized Food Delivery Application

**Courses:** Operating Systems (TCS-502) & Database Management Systems (TCS-503)  
**Team:** Team 205 (Foodsters)  
**Language:** C

---

## 👥 Team Members & Task Division

| Team Member | Student ID | Email | Primary Responsibilities & Code Modules |
| :--- | :--- | :--- | :--- |
| **Rawat, Shreyash** *(Team Lead)* | **24021627** | `24021627@geu.ac.in` | **OS Scheduling & Multi-Resource Kernel**<br>• `src/os/scheduling.c` & `include/os_scheduling.h`<br>• `src/os/resource_manager.c` & `include/os_resource.h`<br>• FCFS, Round Robin, and Priority Scheduling algorithms<br>• Banker's Safety Algorithm & RAG cycle detection |
| **Kumar Singh, Ankit** | **240211994** | `240211994@geu.ac.in` | **DBMS Storage, Transactions & WAL Engine**<br>• `src/dbms/storage.c` & `include/dbms_storage.h`<br>• `src/dbms/transaction.c` & `include/dbms_transaction.h`<br>• Persistent database storage for orders and drivers<br>• ACID Transactions, Write-Ahead Logging (`wal.log`), Rollback & Crash Recovery |
| **Chauhan, Tanvi** | **240222004** | `240222004@geu.ac.in` | **Priority Engine, CLI & Dashboard Interface**<br>• `src/cli/priority_calc.c` & `include/cli_priority.h`<br>• `src/cli/menu.c` & `include/cli_menu.h`<br>• Multi-factor Priority Calculation formula (Food type, prep, distance, tier)<br>• Menu-driven interactive CLI application |

---

## 🧠 Core Computer Science Concept Mapping

As highlighted in the project proposal, FeastOS explicitly translates food delivery logistics into foundational OS and DBMS abstractions:

```
+-----------------------------+------------------------------------------------+
| Food Delivery Domain        | OS / DBMS Abstraction                         |
+-----------------------------+------------------------------------------------+
| Food Order                  | Process (Requires CPU & I/O execution time)    |
| Delivery Dispatcher         | OS Kernel CPU Scheduler                        |
| Kitchen Slots               | Limited Concurrency Slots (Core CPU capacity)  |
| Delivery Drivers            | Hardware Device Controllers / Resources        |
| Insulated Bags & Cryo Boxes | Specialized Hardware Device Resources          |
| Order Claim & Assignment    | Atomic ACID Transaction                        |
| Order State Lifecycle       | Write-Ahead Logging (WAL) and Rollback         |
| Competing Driver Requests   | Concurrency Control & Deadlock Prevention      |
+-----------------------------+------------------------------------------------+
```

---

## 📂 Project Structure

```
feastos2/
├── Makefile                     # Build system with clang/gcc
├── README.md                    # Project documentation & team report mapping
├── include/
│   ├── common.h                 # Shared constants, enums, data structs, ANSI colors
│   ├── os_scheduling.h          # Member 1: FCFS, RR, Priority scheduling signatures & metrics
│   ├── os_resource.h            # Member 1: Multi-resource, Banker's Algorithm, RAG headers
│   ├── dbms_storage.h           # Member 2: CRUD storage prototypes for drivers & orders
│   ├── dbms_transaction.h       # Member 2: ACID transactions, Write-Ahead Logging (WAL) & Undo
│   ├── cli_priority.h           # Member 3: Dynamic priority heuristic & equipment mapping
│   └── cli_menu.h               # Member 3: Menu-driven command-line interface declarations
├── src/
│   ├── main.c                   # Application entry point
│   ├── os/
│   │   ├── scheduling.c         # Explicit FCFS, Round Robin, Priority algorithms & Gantt charts
│   │   └── resource_manager.c   # Multi-resource allocation, Banker's safe state, RAG deadlock
│   ├── dbms/
│   │   ├── storage.c            # File persistence for data/orders.dat & data/drivers.dat
│   │   └── transaction.c        # ACID transactions, Write-Ahead Log (data/wal.log), Rollback
│   └── cli/
│       ├── priority_calc.c      # Formula calculating priority from food, prep time, distance, tier
│       └── menu.c               # Menu-driven CLI navigation and visual format
└── data/                        # Persistent storage directory
    ├── orders.dat               # Binary database table of food orders
    ├── drivers.dat              # Binary database table of delivery driver fleet
    └── wal.log                  # Persistent Write-Ahead Log
```

---

## ⚡ Priority Calculation Formula

The priority calculation engine (`src/cli/priority_calc.c`) computes a normalized score (1 to 100) where **higher score = higher priority** in the OS dispatcher:

$$\text{Priority Score} = P_{\text{food}} + P_{\text{prep}} + P_{\text{dist}} + P_{\text{tier}}$$

1. **Food Type & Perishability ($P_{\text{food}}$):**
   - **Ice Cream / Gelato / Frozen Desserts:** $+45\text{ pts}$ *(Extreme perishability: melts within 15–20 minutes; requires Cold Storage Container)*
   - **Hot Meals / Soups / Sizzlers:** $+35\text{ pts}$ *(Thermal loss risk; requires Thermal Insulated Bag)*
   - **Bakery / Ambient Fresh Pastries:** $+20\text{ pts}$ *(Moderate shelf-life)*
   - **Packaged Dry Goods / Groceries / Beverages:** $+10\text{ pts}$ *(Standard shelf-life; no specialized equipment needed)*
2. **Kitchen Preparation Time ($P_{\text{prep}}$):**
   $$P_{\text{prep}} = \min\left(25, \frac{\text{prep\_time\_min} \times 25}{35}\right)$$
   *(Orders requiring longer prep time are scheduled earlier in the kitchen to minimize head-of-line delays).*
3. **Delivery Distance ($P_{\text{dist}}$):**
   $$P_{\text{dist}} = \min(20, \lfloor \text{distance\_km} \times 1.8 \rfloor)$$
4. **Customer Urgency Tier ($P_{\text{tier}}$):**
   - VIP / Express: $+20\text{ pts}$
   - Standard: $+0\text{ pts}$

---

## 🔄 OS Scheduling Algorithms (`src/os/scheduling.c`)

1. **First-Come, First-Served (FCFS):**
   - Non-preemptive scheduling strictly ordered by arrival timestamp.
2. **Round Robin (RR):**
   - Preemptive time-sliced execution with interactive choice of time quantum $Q$ (e.g. 2, 4, or 5).
   - Generates multi-segment Gantt charts with context switching.
3. **Priority Scheduling:**
   - Dispatches highest-priority orders first (perishable ice creams and VIP hot meals prioritized).
4. **Comparative Analysis Benchmark:**
   - Evaluates:
     - **Average Waiting Time (AWT)**
     - **Average Turnaround Time (ATAT)**
     - **Average Response Time (ART)**
     - **Resource / Dispatcher Utilization %**
     - Side-by-side formatted comparison table and clean execution timelines.

---

## 💾 DBMS Persistent Storage & ACID Transactions

- **Data Persistence:**
  - `data/orders.dat` stores all order records (ID, customer, restaurant, food item, prep time, distance, calculated priority, required equipment, status, assigned driver).
  - `data/drivers.dat` stores all driver records (ID, name, phone, vehicle, equipment capability, status, active order).
- **Write-Ahead Logging (WAL) & ACID Guarantees:**
  - Before modifying database records, an entry is written to `data/wal.log`:
    `<T_id, ORDER_x: [old_status, new_status], DRIVER_y: [old_status, new_status]>`
  - On success: `<COMMIT T_id>`
  - On failure/disconnect: `<ABORT T_id, REASON=...>` and state is restored via Undo rollback.
  - ARIES crash recovery scans the WAL on startup to undo incomplete transactions.

---

## 🚀 Building & Running

### 1. Compile
```bash
make
```

### 2. Run Application
```bash
./feastos
```
*(or run `make run`)*

### 3. Clean
```bash
make clean      # Removes object files and binary
make distclean  # Cleans binary, orders.dat, drivers.dat, and wal.log
```

---

## 📋 Interactive CLI Menu Overview

When you start `./feastos`, the menu displays:

```text
================================================================================
  FeastOS: Optimized Food delivery Application
  Courses: DBMS (TCS-503) & OS (TCS-502) | Team 205 (Foodsters)               
================================================================================
MAIN NAVIGATION MENU:
 [1] Enter New Food Order (Calculates Priority based on Food, Prep, Dist)
 [2] View All Food Orders in Database (data/orders.dat)
 [3] Register New Delivery Driver
 [4] View Delivery Driver Fleet (data/drivers.dat)
 [5] OS Scheduling Simulator (Auto-Runs FCFS, Round Robin & Priority on its own)
 [6] OS Multi-Resource Allocation & Deadlock Engine (Banker's & RAG)
 [7] DBMS Transactions & Write-Ahead Logging (WAL Commit/Rollback)
 [8] Interactive Priority & Equipment Calculator (Formula Tester)
 [9] Seed Realistic Sample Dataset (Instant Swiggy/Zomato orders & fleet)
 [0] Exit FeastOS
```
