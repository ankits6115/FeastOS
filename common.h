#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

/* =========================================================================
 * Project: FeastOS - Optimized Food Delivery Application
 * Courses: Operating Systems (TCS-502) & Database Management Systems (TCS-503)
 * Team:    Team 205 - Foodsters
 * Members:
 *   1. Rawat, Shreyash   (24021627) - Team Lead & OS Kernel Engine
 *   2. Kumar Singh, Ankit(240211994)- DBMS Transactions & Storage Engine
 *   3. Chauhan, Tanvi    (240222004)- Priority Engine & CLI Interface
 * ========================================================================= */

#define MAX_ORDERS      100
#define MAX_DRIVERS     50
#define MAX_CUSTOMERS   50
#define MAX_RESTAURANTS 20
#define MAX_DISPATCHES  100
#define MAX_INVENTORY   50
#define MAX_NAME_LEN    64
#define MAX_PHONE_LEN   20
#define MAX_VEHICLE_LEN 32
#define MAX_ADDR_LEN    64

/* Terminal color styling macros */
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

/* Food Category (Affects perishability, urgency, and equipment) */
typedef enum {
    FOOD_ICE_CREAM_FROZEN = 1, /* Melts quickly: high perishability urgency */
    FOOD_HOT_MEAL         = 2, /* Must stay hot: medium-high urgency */
    FOOD_BAKERY_AMBIENT   = 3, /* Ambient stability: moderate urgency */
    FOOD_PACKAGED_DRY     = 4  /* High shelf-life: standard urgency */
} FoodCategory;

/* Specialized Equipment Required */
typedef enum {
    EQUIP_NONE           = 0,
    EQUIP_INSULATED_BAG  = 1, /* Needed for hot meals/soups */
    EQUIP_COLD_CONTAINER = 2  /* Needed for ice creams/frozen items */
} EquipmentType;

/* Order Lifecycle Status */
typedef enum {
    STATUS_PENDING          = 0,
    STATUS_PREPARING        = 1,
    STATUS_READY_FOR_PICKUP = 2,
    STATUS_OUT_FOR_DELIVERY = 3,
    STATUS_DELIVERED        = 4,
    STATUS_CANCELLED        = 5
} OrderStatus;

/* Delivery Driver Availability Status */
typedef enum {
    DRIVER_OFFLINE   = 0,
    DRIVER_AVAILABLE = 1,
    DRIVER_BUSY      = 2
} DriverStatus;

/* Customer Delivery Urgency Tier */
typedef enum {
    TIER_STANDARD = 1, /* Base fair-share priority (+0 pts) */
    TIER_EXPRESS  = 2, /* Fast-track single-order dispatch (+15 pts) */
    TIER_VIP      = 3  /* Premium subscription, real-time preemption (+30 pts) */
} DeliveryTier;

/* Priority Calculation Breakdown */
typedef struct {
    int food_perishability_score;
    int prep_time_score;
    int distance_penalty;
    int tier_bonus;
    int calculated_priority;
    char reason[128];
    char tier_label[32];
} PriorityBreakdown;

/* -------------------------------------------------------------------------
 * Database Entity 1: Customers Database (`data/customers.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int id;                         /* Customer ID, e.g. 201 */
    char name[MAX_NAME_LEN];        /* Customer Full Name */
    char phone[MAX_PHONE_LEN];      /* Phone Number */
    char address[MAX_ADDR_LEN];     /* Delivery Address / Zone */
    DeliveryTier tier;              /* VIP, Express, Standard */
    int total_orders;               /* Historical order count */
    float loyalty_points;           /* Reward points */
} Customer;

/* -------------------------------------------------------------------------
 * Database Entity 2: Restaurants & Kitchens Database (`data/restaurants.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int id;                         /* Restaurant ID, e.g. 301 */
    char name[MAX_NAME_LEN];        /* Restaurant Name */
    char cuisine[MAX_NAME_LEN];     /* Cuisine Category */
    char location[MAX_ADDR_LEN];    /* Sector / Location */
    int total_kitchen_slots;        /* Hardware cooking stations */
    int available_slots;            /* Current free stations */
    int avg_prep_time_min;          /* Benchmark preparation time */
    float rating;                   /* Customer rating (e.g. 4.8) */
} Restaurant;

/* -------------------------------------------------------------------------
 * Database Entity 3: Food Orders Database (`data/orders.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int id;
    char customer_name[MAX_NAME_LEN];
    char restaurant_name[MAX_NAME_LEN];
    char food_item[MAX_NAME_LEN];
    FoodCategory food_type;
    int prep_time_min;            /* Kitchen burst time */
    float distance_km;            /* Transit distance */
    DeliveryTier delivery_tier;   /* Standard, Express, or VIP */
    int is_express;               /* 1 if Express or VIP, 0 if Standard */
    int priority_score;           /* 1 to 100 (Higher = more urgent) */
    EquipmentType equipment_needed;
    OrderStatus status;
    int assigned_driver_id;       /* 0 if not assigned */
    
    /* OS Scheduling Metrics */
    int arrival_time;             /* Process arrival time */
    int burst_time;               /* Simulated CPU burst = prep + travel */
    int remaining_time;           /* Remaining time */
    int start_time;               /* First time dispatched */
    int completion_time;          /* Process finish time */
    int waiting_time;             /* Waiting time in queue */
    int turnaround_time;          /* Completion - Arrival */
    int response_time;            /* Start - Arrival */
} Order;

/* -------------------------------------------------------------------------
 * Database Entity 4: Delivery Fleet Database (`data/drivers.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];
    char vehicle[MAX_VEHICLE_LEN];
    EquipmentType equipment_capability;
    DriverStatus status;
    int current_order_id;
    int total_deliveries;
} Driver;

/* -------------------------------------------------------------------------
 * Database Entity 5: Dispatch & Logistics Database (`data/dispatches.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int dispatch_id;                /* e.g. 501 */
    int order_id;                   /* Foreign Key -> Order */
    int driver_id;                  /* Foreign Key -> Driver */
    char customer_name[MAX_NAME_LEN];
    char restaurant_name[MAX_NAME_LEN];
    EquipmentType equipment_used;   /* Equipment allocated */
    OrderStatus status;             /* Pending, Out for Delivery, Delivered */
    char dispatch_time[32];         /* Timestamp */
    char delivery_eta[32];          /* ETA */
} DispatchRecord;

/* -------------------------------------------------------------------------
 * Database Entity 6: Equipment & Inventory Database (`data/inventory.dat`)
 * ------------------------------------------------------------------------- */
typedef struct {
    int item_id;                    /* e.g. 701 */
    EquipmentType type;             /* Kitchen Slot, Insulated Bag, Cold Container */
    char serial_no[32];             /* e.g. "TB-BAG-01" */
    char description[64];           /* Description */
    bool is_assigned;               /* Assigned or available */
    int assigned_to_driver_id;      /* Driver ID holding equipment */
    int assigned_to_order_id;       /* Order ID using equipment */
} EquipmentInventory;

/* Helper conversion functions */
static inline const char *food_category_to_string(FoodCategory cat) {
    switch (cat) {
        case FOOD_ICE_CREAM_FROZEN: return "Ice Cream / Frozen";
        case FOOD_HOT_MEAL:         return "Hot Meal / Sizzler";
        case FOOD_BAKERY_AMBIENT:   return "Bakery / Ambient";
        case FOOD_PACKAGED_DRY:     return "Packaged / Dry";
        default:                    return "Unknown Food";
    }
}

static inline const char *equipment_to_string(EquipmentType eq) {
    switch (eq) {
        case EQUIP_NONE:           return "Standard Bag";
        case EQUIP_INSULATED_BAG:  return "Thermal Insulated Bag";
        case EQUIP_COLD_CONTAINER: return "Cold Storage Container";
        default:                   return "None";
    }
}

static inline const char *order_status_to_string(OrderStatus st) {
    switch (st) {
        case STATUS_PENDING:          return "Pending (In Queue)";
        case STATUS_PREPARING:        return "Kitchen Preparing";
        case STATUS_READY_FOR_PICKUP: return "Ready for Pickup";
        case STATUS_OUT_FOR_DELIVERY: return "Out for Delivery";
        case STATUS_DELIVERED:        return "Delivered";
        case STATUS_CANCELLED:        return "Cancelled";
        default:                      return "Unknown";
    }
}

static inline const char *driver_status_to_string(DriverStatus st) {
    switch (st) {
        case DRIVER_OFFLINE:   return "Offline";
        case DRIVER_AVAILABLE: return "Available";
        case DRIVER_BUSY:      return "Busy (On Delivery)";
        default:               return "Unknown";
    }
}

static inline const char *delivery_tier_to_string(DeliveryTier tier) {
    switch (tier) {
        case TIER_VIP:      return "VIP Priority";
        case TIER_EXPRESS:  return "Express Fast-Track";
        case TIER_STANDARD:
        default:            return "Standard";
    }
}

#endif /* COMMON_H */
