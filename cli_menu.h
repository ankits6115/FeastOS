#ifndef CLI_MENU_H
#define CLI_MENU_H

#include "common.h"

/* =========================================================================
 * Module: Menu-Driven Command Line Application
 * Author: Tanvi Chauhan (240222004) - TCS-502 / TCS-503 Team Foodsters
 * Description:
 *   Interactive user interface providing a comprehensive menu for FeastOS:
 *     - Food Order Entry & Management (with live priority calculation)
 *     - Delivery Driver Registration & Fleet Tracking
 *     - OS Scheduling Simulator (FCFS, Priority & Comparative Gantt)
 *     - Multi-Resource Allocation & Deadlock Engine (Banker's & RAG)
 *     - DBMS Transactions, Write-Ahead Logging & Rollback Tester
 *     - Project Proposal & Team Contribution Dashboard
 * ========================================================================= */

void run_cli_application(void);
void show_main_banner(void);
void show_team_proposal_info(void);

#endif /* CLI_MENU_H */
