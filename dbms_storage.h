#ifndef DBMS_STORAGE_H
#define DBMS_STORAGE_H

#include "common.h"

/* =========================================================================
 * Module: DBMS Storage & Persistent Data Layer
 * Author: Ankit Kumar Singh (240211994) - TCS-503 DBMS
 * Description:
 *   Handles persistent disk storage for FeastOS data entities:
 *     - Food Orders table/records (`data/orders.dat`)
 *     - Delivery Drivers table/records (`data/drivers.dat`)
 *   Provides CRUD (Create, Read, Update, Delete) primitives, indexing by ID,
 *   and seeding demo data for instant evaluation.
 * ========================================================================= */

#define ORDERS_DB_FILE       "data/orders.dat"
#define DRIVERS_DB_FILE      "data/drivers.dat"
#define CUSTOMERS_DB_FILE    "data/customers.dat"
#define RESTAURANTS_DB_FILE  "data/restaurants.dat"
#define DISPATCHES_DB_FILE   "data/dispatches.dat"
#define INVENTORY_DB_FILE    "data/inventory.dat"

/* Database initialization */
int db_storage_init(void);

/* 1. Customer Database CRUD (`data/customers.dat`) */
int db_save_customer(const Customer *customer);
int db_load_all_customers(Customer customers[], int max_count);
int db_find_customer_by_id(int customer_id, Customer *customer_out);
int db_get_next_customer_id(void);

/* 2. Restaurant & Kitchen Database CRUD (`data/restaurants.dat`) */
int db_save_restaurant(const Restaurant *restaurant);
int db_load_all_restaurants(Restaurant restaurants[], int max_count);
int db_find_restaurant_by_id(int restaurant_id, Restaurant *restaurant_out);
int db_get_next_restaurant_id(void);

/* 3. Driver Storage CRUD (`data/drivers.dat`) */
int db_save_driver(const Driver *driver);
int db_load_all_drivers(Driver drivers[], int max_count);
int db_find_driver_by_id(int driver_id, Driver *driver_out);
int db_update_driver(const Driver *driver);
int db_get_next_driver_id(void);

/* 4. Order Storage CRUD (`data/orders.dat`) */
int db_save_order(const Order *order);
int db_load_all_orders(Order orders[], int max_count);
int db_find_order_by_id(int order_id, Order *order_out);
int db_update_order(const Order *order);
int db_get_next_order_id(void);

/* 5. Dispatch & Logistics Database CRUD (`data/dispatches.dat`) */
int db_save_dispatch(const DispatchRecord *rec);
int db_load_all_dispatches(DispatchRecord dispatches[], int max_count);
int db_find_dispatch_by_order_id(int order_id, DispatchRecord *rec_out);
int db_get_next_dispatch_id(void);

/* 6. Equipment & Inventory Database CRUD (`data/inventory.dat`) */
int db_save_inventory(const EquipmentInventory *item);
int db_load_all_inventory(EquipmentInventory items[], int max_count);
int db_find_inventory_by_id(int item_id, EquipmentInventory *item_out);
int db_get_next_inventory_id(void);

/* Database Utilities */
int  db_auto_dispatch_orders(void);
void db_seed_sample_data(void);
void db_clear_all_data(void);

#endif /* DBMS_STORAGE_H */
