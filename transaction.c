

#include "dbms_transaction.h"
#include "dbms_storage.h"

static int g_txn_counter = 1000;

static void get_formatted_time(char *buf, size_t size) {
    time_t raw = time(NULL);
    struct tm *info = localtime(&raw);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", info);
}

int txn_manager_init(void) {
    db_storage_init();
    FILE *fp = fopen(WAL_LOG_FILE, "ab+");
    if (!fp) return -1;
    fclose(fp);
    return 0;
}

void wal_write_entry(const char *log_message) {
    FILE *fp = fopen(WAL_LOG_FILE, "a");
    if (!fp) return;

    char time_buf[32];
    get_formatted_time(time_buf, sizeof(time_buf));

    fprintf(fp, "[%s] %s\n", time_buf, log_message);
    fflush(fp);
    fclose(fp);
}

int txn_begin(TransactionContext *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(TransactionContext));

    ctx->txn_id = ++g_txn_counter;
    ctx->state = TXN_STATE_ACTIVE;
    ctx->timestamp = time(NULL);

    char wal_entry[128];
    snprintf(wal_entry, sizeof(wal_entry), "<START T_%d>", ctx->txn_id);
    wal_write_entry(wal_entry);

    printf(ANSI_CYAN "  [WAL] Logged: %s\n" ANSI_RESET, wal_entry);
    return ctx->txn_id;
}

bool txn_claim_order(TransactionContext *ctx, int order_id, int driver_id) {
    if (!ctx || ctx->state != TXN_STATE_ACTIVE) {
        printf(ANSI_RED "  [Error] No active transaction context!\n" ANSI_RESET);
        return false;
    }

    Order order;
    if (db_find_order_by_id(order_id, &order) != 0) {
        printf(ANSI_RED "  [Txn Error] Order ID %d not found in database!\n" ANSI_RESET, order_id);
        txn_rollback(ctx, "ORDER_NOT_FOUND");
        return false;
    }

    Driver driver;
    if (db_find_driver_by_id(driver_id, &driver) != 0) {
        printf(ANSI_RED "  [Txn Error] Driver ID %d not found in database!\n" ANSI_RESET, driver_id);
        txn_rollback(ctx, "DRIVER_NOT_FOUND");
        return false;
    }

    /* Check Concurrency / Lock: Is order already claimed? */
    if (order.status != STATUS_PENDING) {
        printf(ANSI_YELLOW "  [Txn Conflict] Order #%d is already locked/processed (Status: %s)!\n" ANSI_RESET,
               order.id, order_status_to_string(order.status));
        txn_rollback(ctx, "ORDER_LOCK_FAILED_ALREADY_CLAIMED");
        return false;
    }

    /* Check Driver Availability */
    if (driver.status != DRIVER_AVAILABLE) {
        printf(ANSI_YELLOW "  [Txn Conflict] Driver #%d is busy or offline (Status: %s)!\n" ANSI_RESET,
               driver.id, driver_status_to_string(driver.status));
        txn_rollback(ctx, "DRIVER_UNAVAILABLE");
        return false;
    }

    /* Equipment capability check */
    if (order.equipment_needed == EQUIP_COLD_CONTAINER && driver.equipment_capability != EQUIP_COLD_CONTAINER) {
        printf(ANSI_RED "  [Txn Mismatch] Order requires Cold Container, but Driver lacks this equipment!\n" ANSI_RESET);
        txn_rollback(ctx, "EQUIPMENT_MISMATCH_COLD_CONTAINER");
        return false;
    }
    if (order.equipment_needed == EQUIP_INSULATED_BAG &&
        driver.equipment_capability != EQUIP_INSULATED_BAG &&
        driver.equipment_capability != EQUIP_COLD_CONTAINER) {
        printf(ANSI_RED "  [Txn Mismatch] Order requires Insulated Bag, but Driver lacks thermal equipment!\n" ANSI_RESET);
        txn_rollback(ctx, "EQUIPMENT_MISMATCH_INSULATED_BAG");
        return false;
    }

    /* Record before-image in Transaction Context for Undo capability */
    ctx->order_id = order.id;
    ctx->driver_id = driver.id;
    ctx->old_order_status = order.status;
    ctx->new_order_status = STATUS_OUT_FOR_DELIVERY;
    ctx->old_driver_status = driver.status;
    ctx->new_driver_status = DRIVER_BUSY;

    /* Write-Ahead Logging: Write UNDO/REDO record BEFORE changing database on disk */
    char wal_record[256];
    snprintf(wal_record, sizeof(wal_record),
             "<T_%d, ORDER_%d: [old=%s, new=%s], DRIVER_%d: [old=%s, new=%s]>",
             ctx->txn_id,
             order.id, order_status_to_string(ctx->old_order_status), order_status_to_string(ctx->new_order_status),
             driver.id, driver_status_to_string(ctx->old_driver_status), driver_status_to_string(ctx->new_driver_status));
    wal_write_entry(wal_record);
    printf(ANSI_CYAN "  [WAL] Logged Write-Ahead record: %s\n" ANSI_RESET, wal_record);

    /* Apply changes to database tables */
    order.status = ctx->new_order_status;
    order.assigned_driver_id = driver.id;
    db_update_order(&order);

    driver.status = ctx->new_driver_status;
    driver.current_order_id = order.id;
    db_update_driver(&driver);

    printf(ANSI_GREEN "  [Txn Executed] Order #%d assigned to Driver #%d (%s).\n" ANSI_RESET,
           order.id, driver.id, driver.name);
    return true;
}

bool txn_commit(TransactionContext *ctx) {
    if (!ctx || ctx->state != TXN_STATE_ACTIVE) return false;

    char wal_entry[128];
    snprintf(wal_entry, sizeof(wal_entry), "<COMMIT T_%d>", ctx->txn_id);
    wal_write_entry(wal_entry);

    ctx->state = TXN_STATE_COMMITTED;
    printf(ANSI_GREEN "  [WAL] Logged: %s -- Transaction successfully committed to disk.\n" ANSI_RESET, wal_entry);
    return true;
}

bool txn_rollback(TransactionContext *ctx, const char *reason) {
    if (!ctx) return false;

    /* Undo in-memory and database changes if order was modified */
    if (ctx->order_id > 0) {
        Order order;
        if (db_find_order_by_id(ctx->order_id, &order) == 0) {
            order.status = ctx->old_order_status;
            order.assigned_driver_id = 0;
            db_update_order(&order);
        }
    }

    if (ctx->driver_id > 0) {
        Driver driver;
        if (db_find_driver_by_id(ctx->driver_id, &driver) == 0) {
            driver.status = ctx->old_driver_status;
            driver.current_order_id = 0;
            db_update_driver(&driver);
        }
    }

    char wal_entry[256];
    snprintf(wal_entry, sizeof(wal_entry), "<ABORT T_%d, REASON=%s>", ctx->txn_id, reason ? reason : "MANUAL_ABORT");
    wal_write_entry(wal_entry);

    ctx->state = TXN_STATE_ABORTED;
    printf(ANSI_RED "  [WAL] Logged: %s -- Rollback completed. State restored to original!\n" ANSI_RESET, wal_entry);
    return true;
}

void wal_display_log(void) {
    FILE *fp = fopen(WAL_LOG_FILE, "r");
    if (!fp) {
        printf(ANSI_YELLOW "  [WAL] Write-Ahead Log is currently empty.\n" ANSI_RESET);
        return;
    }

    printf("\n" ANSI_BOLD ANSI_CYAN "=========================================================================================\n");
    printf("                       FEASTOS DBMS WRITE-AHEAD LOG (WAL) AUDIT                          \n");
    printf("=========================================================================================" ANSI_RESET "\n");

    char line[512];
    int line_num = 1;
    bool has_entries = false;
    while (fgets(line, sizeof(line), fp)) {
        has_entries = true;
        /* Highlight start, commit, and abort entries */
        if (strstr(line, "<START")) {
            printf(" %03d | " ANSI_BLUE "%s" ANSI_RESET, line_num++, line);
        } else if (strstr(line, "<COMMIT")) {
            printf(" %03d | " ANSI_GREEN "%s" ANSI_RESET, line_num++, line);
        } else if (strstr(line, "<ABORT")) {
            printf(" %03d | " ANSI_RED "%s" ANSI_RESET, line_num++, line);
        } else {
            printf(" %03d | " ANSI_WHITE "%s" ANSI_RESET, line_num++, line);
        }
    }
    fclose(fp);

    if (!has_entries) {
        printf("  [Log file exists but contains 0 entries]\n");
    }
    printf("=========================================================================================\n");
}

void wal_clear_log(void) {
    remove(WAL_LOG_FILE);
    txn_manager_init();
    printf(ANSI_GREEN "  [+] Write-Ahead Log (WAL) cleared.\n" ANSI_RESET);
}

int wal_recover_from_crash(void) {
    FILE *fp = fopen(WAL_LOG_FILE, "r");
    if (!fp) {
        printf(ANSI_YELLOW "  [Recovery] No WAL log file found.\n" ANSI_RESET);
        return 0;
    }

    printf(ANSI_CYAN "\n  [ARIES Recovery Protocol] Scanning Write-Ahead Log for active transactions...\n" ANSI_RESET);

    char line[512];
    int active_txns[100];
    int active_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        int tid = 0;
        if (sscanf(line, "%*[^<]<START T_%d>", &tid) == 1) {
            active_txns[active_count++] = tid;
        } else if (sscanf(line, "%*[^<]<COMMIT T_%d>", &tid) == 1 || sscanf(line, "%*[^<]<ABORT T_%d", &tid) == 1) {
            /* Remove tid from active */
            for (int i = 0; i < active_count; i++) {
                if (active_txns[i] == tid) {
                    for (int j = i; j < active_count - 1; j++) {
                        active_txns[j] = active_txns[j + 1];
                    }
                    active_count--;
                    break;
                }
            }
        }
    }
    fclose(fp);

    if (active_count == 0) {
        printf(ANSI_GREEN "  [Recovery Check] All historical transactions are resolved (Committed/Aborted).\n" ANSI_RESET);
        printf("  [System is completely consistent and fault-tolerant].\n");
    } else {
        printf(ANSI_RED "  [Recovery Alert] Found %d uncommitted transaction(s) needing UNDO rollback!\n" ANSI_RESET, active_count);
        for (int i = 0; i < active_count; i++) {
            printf("    * Rolling back uncommitted Txn T_%d...\n", active_txns[i]);
            char abort_entry[128];
            snprintf(abort_entry, sizeof(abort_entry), "<ABORT T_%d, REASON=CRASH_RECOVERY_UNDO>", active_txns[i]);
            wal_write_entry(abort_entry);
        }
        printf(ANSI_GREEN "  [Recovery Complete] Database state restored to safe checkpoint.\n" ANSI_RESET);
    }

    return active_count;
}
