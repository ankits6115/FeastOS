# FeastOS: Optimized Food delivery Application

**Courses:** Operating Systems (TCS-502) & Database Management Systems (TCS-503)  
**Team:** Team 205 (Foodsters)  
**Language:** Pure C (C11 standard)

---

## 👥 Team Members

| Name | Student ID | Role & Core Modules |
| :--- | :--- | :--- |
| **Shreyash Rawat** *(Team Lead)* | **24021627** | **OS Architecture:** CPU Scheduling (FCFS & Priority), Banker's Algorithm, RAG Deadlock Engine |
| **Ankit Kumar Singh** | **240211994** | **DBMS Architecture:** 6 Relational Databases, ACID Transactions, Write-Ahead Logging (WAL), Rollback & Recovery |
| **Tanvi Chauhan** | **240222004** | **Priority & CLI:** Multi-Factor Priority Heuristic, Equipment Mapping, Interactive CLI Menu |

---

## 💡 About FeastOS

FeastOS is an optimized food delivery system simulator that bridges real-world delivery logistics with foundational Computer Science concepts:

* **Orders as OS Processes:** Every food order is modeled as a process with arrival time, burst time, priority score, and execution state.
* **Dispatcher as CPU Scheduler:** Automatically applies **FCFS** and **Priority Scheduling** to evaluate Average Waiting Time ($AWT$), Turnaround Time ($ATAT$), and Response Time ($ART$).
* **Kitchen & Fleet as Hardware Resources:** Kitchen slots, drivers, and specialized equipment (Insulated Bags, Cold Containers) are managed using **Banker's Algorithm** (safety check) and **Resource Allocation Graphs (RAG)** (deadlock detection).
* **Delivery Lifecycle as ACID Transactions:** Order assignments run as atomic transactions backed by **Write-Ahead Logging (`data/wal.log`)** with automatic rollback on errors and ARIES crash recovery.
* **Multi-Database Storage:** Storage is normalized across 6 persistent relational database files under `data/`.

---

## 🚀 Quick Start

### Build and Run
```bash
# 1. Compile the project
make

# 2. Launch FeastOS
./feastos
```

### Clean
```bash
make clean
```

---

## 📋 Main Navigation Menu

```text
================================================================================
  FeastOS: Optimized Food delivery Application
  Courses: DBMS (TCS-503) & OS (TCS-502) | Team 205 (Foodsters)
================================================================================
  MAIN NAVIGATION MENU:
   [1] Enter New Food Order
   [2] View All Food Orders in Database
   [3] Register New Delivery Driver
   [4] View Delivery Driver Fleet
   [5] OS Scheduling Simulator
   [6] OS Multi-Resource Allocation & Deadlock Engine
   [7] View All Databases
   [8] Seed Realistic Sample Dataset
   [0] Exit FeastOS
```

---

## 💾 Multi-Database Architecture (`data/`)

FeastOS maintains persistent binary records across 6 normalized databases:

1. **`data/customers.dat`**: Customer accounts and delivery tiers (VIP, Express, Standard).
2. **`data/restaurants.dat`**: Partner kitchens, cuisines, capacity, and average prep times.
3. **`data/orders.dat`**: Food orders sorted by priority score descending ($92 \rightarrow 85 \rightarrow 64 \dots$).
4. **`data/drivers.dat`**: Delivery driver fleet, thermal equipment capabilities, and active orders.
5. **`data/dispatches.dat`**: Logistics tracking linking Orders $\leftrightarrow$ Drivers $\leftrightarrow$ Equipment.
6. **`data/inventory.dat`**: Serialized hardware inventory (Thermal Insulated Bags & Cold Containers).
7. **`data/wal.log`**: Write-Ahead Log storing `<START>`, before/after images, `<COMMIT>`, and `<ABORT>` entries.

---

## ⚡ Dynamic Priority Score Formula

$$\text{Priority Score} = P_{\text{food}} + P_{\text{prep}} + P_{\text{dist}} + P_{\text{tier}}$$

* **Food Perishability ($P_{\text{food}}$):** Ice Cream/Gelato (+45), Hot Meals/Pizzas (+35), Bakery (+20), Groceries (+10)
* **Preparation Time ($P_{\text{prep}}$):** Up to +25 points (longer prep dishes scheduled early to avoid kitchen bottlenecks)
* **Delivery Distance ($P_{\text{dist}}$):** Up to +20 points (prioritizes farther trips to meet delivery deadlines)
* **Customer Tier ($P_{\text{tier}}$):** VIP (+20), Express (+10), Standard (+0)
