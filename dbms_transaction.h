#ifndef DBMS_TRANSACTION_H
#define DBMS_TRANSACTION_H

#include "common.h"

/* =========================================================================
 * Module: DBMS Transaction Management, Write-Ahead Logging (WAL) & Rollback
 * Author: Ankit Kumar Singh (240211994) - TCS-503 DBMS
 * Description:
 *   Guarantees ACID transactional properties for FeastOS:
 *     - Atomicity: An order claim either fully assigns kitchen + driver or rolls back.
 *     - Consistency: Prevents double assignments or orphaned driver states.
 *     - Isolation: Locking mechanism on order records.
 *     - Durability: Write-Ahead Logging (WAL) stored on disk before state mutation.
 *   Provides crash simulation and ARIES-style WAL recovery (Undo/Redo).
 * ========================================================================= */

#define WAL_LOG_FILE "data/wal.log"

typedef enum {
    TXN_STATE_ACTIVE    = 0,
    TXN_STATE_COMMITTED = 1,
    TXN_STATE_ABORTED   = 2
} TxnState;

typedef struct {
    int txn_id;
    TxnState state;
    int order_id;
    int driver_id;
    OrderStatus old_order_status;
    OrderStatus new_order_status;
    DriverStatus old_driver_status;
    DriverStatus new_driver_status;
    time_t timestamp;
} TransactionContext;

/* Transaction Operations */
int txn_manager_init(void);
int txn_begin(TransactionContext *ctx);
bool txn_claim_order(TransactionContext *ctx, int order_id, int driver_id);
bool txn_commit(TransactionContext *ctx);
bool txn_rollback(TransactionContext *ctx, const char *reason);

/* Write-Ahead Logging (WAL) Inspection & Crash Recovery */
void wal_write_entry(const char *log_message);
void wal_display_log(void);
void wal_clear_log(void);
int  wal_recover_from_crash(void);

#endif /* DBMS_TRANSACTION_H */
