
#include "dbms_storage.h"
#include <sys/stat.h>

int db_storage_init(void) {
    /* Ensure data directory exists */
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        #ifdef _WIN32
        _mkdir("data");
        #else
        mkdir("data", 0755);
        #endif
    }

    /* Verify access or touch data files */
    FILE *fp = fopen(DRIVERS_DB_FILE, "ab+");
    if (fp) fclose(fp);

    fp = fopen(ORDERS_DB_FILE, "ab+");
    if (fp) fclose(fp);

    return 0;
}

/* -------------------------------------------------------------------------
 * Driver Storage CRUD
 * ------------------------------------------------------------------------- */
int db_load_all_drivers(Driver drivers[], int max_count) {
    FILE *fp = fopen(DRIVERS_DB_FILE, "rb");
    if (!fp) return 0;

    int count = 0;
    while (count < max_count && fread(&drivers[count], sizeof(Driver), 1, fp) == 1) {
        count++;
    }
    fclose(fp);
    return count;
}

int db_save_driver(const Driver *driver) {
    if (!driver) return -1;

    Driver existing[MAX_DRIVERS];
    int count = db_load_all_drivers(existing, MAX_DRIVERS);

    /* Check if already exists for update */
    int found_idx = -1;
    for (int i = 0; i < count; i++) {
        if (existing[i].id == driver->id) {
            found_idx = i;
            break;
        }
    }

    if (found_idx != -1) {
        existing[found_idx] = *driver;
    } else {
        if (count >= MAX_DRIVERS) return -1;
        existing[count++] = *driver;
    }

    FILE *fp = fopen(DRIVERS_DB_FILE, "wb");
    if (!fp) return -1;

    fwrite(existing, sizeof(Driver), count, fp);
    fclose(fp);
    return 0;
}

int db_find_driver_by_id(int driver_id, Driver *driver_out) {
    Driver drivers[MAX_DRIVERS];
    int count = db_load_all_drivers(drivers, MAX_DRIVERS);

    for (int i = 0; i < count; i++) {
        if (drivers[i].id == driver_id) {
            if (driver_out) *driver_out = drivers[i];
            return 0;
        }
    }
    return -1;
}

int db_update_driver(const Driver *driver) {
    return db_save_driver(driver);
}

int db_get_next_driver_id(void) {
    Driver drivers[MAX_DRIVERS];
    int count = db_load_all_drivers(drivers, MAX_DRIVERS);
    int max_id = 100;
    for (int i = 0; i < count; i++) {
        if (drivers[i].id > max_id) max_id = drivers[i].id;
    }
    return max_id + 1;
}

/* -------------------------------------------------------------------------
 * Order Storage CRUD
 * ------------------------------------------------------------------------- */
int db_load_all_orders(Order orders[], int max_count) {
    FILE *fp = fopen(ORDERS_DB_FILE, "rb");
    if (!fp) return 0;

    int count = 0;
    while (count < max_count && fread(&orders[count], sizeof(Order), 1, fp) == 1) {
        count++;
    }
    fclose(fp);
    return count;
}

int db_save_order(const Order *order) {
    if (!order) return -1;

    Order existing[MAX_ORDERS];
    int count = db_load_all_orders(existing, MAX_ORDERS);

    int found_idx = -1;
    for (int i = 0; i < count; i++) {
        if (existing[i].id == order->id) {
            found_idx = i;
            break;
        }
    }

    if (found_idx != -1) {
        existing[found_idx] = *order;
    } else {
        if (count >= MAX_ORDERS) return -1;
        existing[count++] = *order;
    }

    FILE *fp = fopen(ORDERS_DB_FILE, "wb");
    if (!fp) return -1;

    fwrite(existing, sizeof(Order), count, fp);
    fclose(fp);
    return 0;
}

int db_find_order_by_id(int order_id, Order *order_out) {
    Order orders[MAX_ORDERS];
    int count = db_load_all_orders(orders, MAX_ORDERS);

    for (int i = 0; i < count; i++) {
        if (orders[i].id == order_id) {
            if (order_out) *order_out = orders[i];
            return 0;
        }
    }
    return -1;
}

int db_update_order(const Order *order) {
    return db_save_order(order);
}

int db_get_next_order_id(void) {
    Order orders[MAX_ORDERS];
    int count = db_load_all_orders(orders, MAX_ORDERS);
    int max_id = 0;
    for (int i = 0; i < count; i++) {
        if (orders[i].id > max_id) max_id = orders[i].id;
    }
    return max_id + 1;
}

/* -------------------------------------------------------------------------
 * Database Utilities: Clear and Seed Sample Data
 * ------------------------------------------------------------------------- */
void db_clear_all_data(void) {
    remove(ORDERS_DB_FILE);
    remove(DRIVERS_DB_FILE);
    db_storage_init();
    printf(ANSI_GREEN "  [+] Database reset: Orders and Drivers cleared.\n" ANSI_RESET);
}

void db_seed_sample_data(void) {
    db_clear_all_data();

    /* 1. Seed Sample Delivery Drivers */
    Driver d1 = {
        .id = 101,
        .name = "Rahul Sharma",
        .phone = "+91-9876543210",
        .vehicle = "Hero Splendor (Bike)",
        .equipment_capability = EQUIP_INSULATED_BAG,
        .status = DRIVER_AVAILABLE,
        .current_order_id = 0,
        .total_deliveries = 14
    };

    Driver d2 = {
        .id = 102,
        .name = "Priya Verma",
        .phone = "+91-9812345678",
        .vehicle = "Ather 450X (EV)",
        .equipment_capability = EQUIP_COLD_CONTAINER,
        .status = DRIVER_AVAILABLE,
        .current_order_id = 0,
        .total_deliveries = 28
    };

    Driver d3 = {
        .id = 103,
        .name = "Amit Patel",
        .phone = "+91-9723456789",
        .vehicle = "Honda Activa (Scooter)",
        .equipment_capability = EQUIP_INSULATED_BAG,
        .status = DRIVER_AVAILABLE,
        .current_order_id = 0,
        .total_deliveries = 9
    };

    Driver d4 = {
        .id = 104,
        .name = "Sneha Roy",
        .phone = "+91-9654321098",
        .vehicle = "TVS Jupiter",
        .equipment_capability = EQUIP_NONE,
        .status = DRIVER_AVAILABLE,
        .current_order_id = 0,
        .total_deliveries = 21
    };

    db_save_driver(&d1);
    db_save_driver(&d2);
    db_save_driver(&d3);
    db_save_driver(&d4);

    /* 2. Seed Sample Food Orders with realistic perishability & distances */
    Order o1 = {
        .id = 1,
        .customer_name = "Aarav Mehta",
        .restaurant_name = "Milano Gelato Parlour",
        .food_item = "Belgian Chocolate Gelato (500ml)",
        .food_type = FOOD_ICE_CREAM_FROZEN,
        .prep_time_min = 6,
        .distance_km = 4.5f,
        .delivery_tier = TIER_VIP,
        .is_express = 1,
        .priority_score = 92,
        .equipment_needed = EQUIP_COLD_CONTAINER,
        .status = STATUS_PENDING,
        .assigned_driver_id = 0,
        .arrival_time = 0,
        .burst_time = 15,
        .remaining_time = 15
    };

    Order o2 = {
        .id = 2,
        .customer_name = "Neha Kapoor",
        .restaurant_name = "Toscano Wood-fire Pizzeria",
        .food_item = "Margherita & Truffle Pasta",
        .food_type = FOOD_HOT_MEAL,
        .prep_time_min = 20,
        .distance_km = 6.2f,
        .delivery_tier = TIER_STANDARD,
        .is_express = 0,
        .priority_score = 64,
        .equipment_needed = EQUIP_INSULATED_BAG,
        .status = STATUS_PENDING,
        .assigned_driver_id = 0,
        .arrival_time = 2,
        .burst_time = 32,
        .remaining_time = 32
    };

    Order o3 = {
        .id = 3,
        .customer_name = "Rohan Das",
        .restaurant_name = "Sizzler Town",
        .food_item = "Sizzling Paneer Platter",
        .food_type = FOOD_HOT_MEAL,
        .prep_time_min = 15,
        .distance_km = 2.0f,
        .delivery_tier = TIER_EXPRESS,
        .is_express = 1,
        .priority_score = 85,
        .equipment_needed = EQUIP_INSULATED_BAG,
        .status = STATUS_PENDING,
        .assigned_driver_id = 0,
        .arrival_time = 3,
        .burst_time = 19,
        .remaining_time = 19
    };

    Order o4 = {
        .id = 4,
        .customer_name = "Simran Kaur",
        .restaurant_name = "The French Baker",
        .food_item = "Almond Butter Croissants (Box of 4)",
        .food_type = FOOD_BAKERY_AMBIENT,
        .prep_time_min = 10,
        .distance_km = 3.5f,
        .delivery_tier = TIER_STANDARD,
        .is_express = 0,
        .priority_score = 42,
        .equipment_needed = EQUIP_NONE,
        .status = STATUS_PENDING,
        .assigned_driver_id = 0,
        .arrival_time = 5,
        .burst_time = 17,
        .remaining_time = 17
    };

    Order o5 = {
        .id = 5,
        .customer_name = "Vikram Aditya",
        .restaurant_name = "Organic Superfoods",
        .food_item = "Cold-Pressed Detox Juices",
        .food_type = FOOD_PACKAGED_DRY,
        .prep_time_min = 5,
        .distance_km = 7.8f,
        .delivery_tier = TIER_STANDARD,
        .is_express = 0,
        .priority_score = 30,
        .equipment_needed = EQUIP_NONE,
        .status = STATUS_PENDING,
        .assigned_driver_id = 0,
        .arrival_time = 8,
        .burst_time = 21,
        .remaining_time = 21
    };

    db_save_order(&o1);
    db_save_order(&o2);
    db_save_order(&o3);
    db_save_order(&o4);
    db_save_order(&o5);

    printf(ANSI_GREEN "  [+] Database seeded successfully with 4 drivers and 5 sample food orders!\n" ANSI_RESET);
}
