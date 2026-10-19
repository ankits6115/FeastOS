/* =========================================================================
 * Project: FeastOS - Optimized Food Delivery Application
 * Module:  Multi-Factor Priority Calculation Engine
 * Author:  Chauhan, Tanvi (240222004) - TCS-502 / TCS-503 Team Foodsters
 * ========================================================================= */

#include "cli_priority.h"

EquipmentType determine_required_equipment(FoodCategory food_type) {
    switch (food_type) {
        case FOOD_ICE_CREAM_FROZEN:
            return EQUIP_COLD_CONTAINER;
        case FOOD_HOT_MEAL:
            return EQUIP_INSULATED_BAG;
        case FOOD_BAKERY_AMBIENT:
        case FOOD_PACKAGED_DRY:
        default:
            return EQUIP_NONE;
    }
}

int calculate_order_priority(
    FoodCategory food_type,
    int prep_time_min,
    float distance_km,
    DeliveryTier tier,
    PriorityBreakdown *breakdown
) {
    int perishability = 0;
    const char *perish_desc = "";

    switch (food_type) {
        case FOOD_ICE_CREAM_FROZEN:
            perishability = 45;
            perish_desc = "Extreme Perishability (Melts < 20 min; cryogenic box required)";
            break;
        case FOOD_HOT_MEAL:
            perishability = 35;
            perish_desc = "High Perishability (Thermal decay; thermal bag required)";
            break;
        case FOOD_BAKERY_AMBIENT:
            perishability = 20;
            perish_desc = "Moderate Urgency (Ambient freshness stability)";
            break;
        case FOOD_PACKAGED_DRY:
        default:
            perishability = 10;
            perish_desc = "Standard Urgency (Long shelf-life / ambient goods)";
            break;
    }

    /* Kitchen Prep Factor (Max 25 pts) */
    int prep_pts = (prep_time_min * 25) / 35;
    if (prep_pts > 25) prep_pts = 25;
    if (prep_pts < 2)  prep_pts = 2;

    /* Distance & Transit Urgency (Max 20 pts) */
    int dist_pts = (int)(distance_km * 1.8f);
    if (dist_pts > 20) dist_pts = 20;
    if (dist_pts < 1)  dist_pts = 1;

    /* Delivery Tier Bonus: VIP (+30 pts), Express (+15 pts), Standard (+0 pts) */
    int tier_pts = 0;
    const char *tier_desc = "Standard";
    switch (tier) {
        case TIER_VIP:
            tier_pts = 30;
            tier_desc = "VIP Priority (+30 pts)";
            break;
        case TIER_EXPRESS:
            tier_pts = 15;
            tier_desc = "Express Fast-Track (+15 pts)";
            break;
        case TIER_STANDARD:
        default:
            tier_pts = 0;
            tier_desc = "Standard Fair-Share (+0 pts)";
            break;
    }

    int total = perishability + prep_pts + dist_pts + tier_pts;
    if (total > 100) total = 100;
    if (total < 10)  total = 10;

    if (breakdown) {
        breakdown->food_perishability_score = perishability;
        breakdown->prep_time_score = prep_pts;
        breakdown->distance_penalty = dist_pts;
        breakdown->tier_bonus = tier_pts;
        breakdown->calculated_priority = total;
        snprintf(breakdown->reason, sizeof(breakdown->reason), "%s", perish_desc);
        snprintf(breakdown->tier_label, sizeof(breakdown->tier_label), "%s", tier_desc);
    }

    return total;
}

void print_priority_breakdown(const PriorityBreakdown *breakdown) {
    if (!breakdown) return;

    printf("\n  " ANSI_CYAN ANSI_BOLD "--- PRIORITY HEURISTIC BREAKDOWN ---" ANSI_RESET "\n");
    printf("    1. Food Perishability Score : " ANSI_YELLOW "+%d pts" ANSI_RESET " (%s)\n",
           breakdown->food_perishability_score, breakdown->reason);
    printf("    2. Kitchen Prep Time Factor : " ANSI_YELLOW "+%d pts" ANSI_RESET "\n",
           breakdown->prep_time_score);
    printf("    3. Transit Distance Urgency : " ANSI_YELLOW "+%d pts" ANSI_RESET "\n",
           breakdown->distance_penalty);
    printf("    4. Delivery Urgency Tier    : " ANSI_YELLOW "+%d pts" ANSI_RESET " (%s)\n",
           breakdown->tier_bonus, breakdown->tier_label);
    printf("  -----------------------------------------------\n");
    printf("  " ANSI_BOLD "  FINAL CALCULATED PRIORITY : " ANSI_GREEN "%d / 100" ANSI_RESET " (Higher = Dispatched First)\n",
           breakdown->calculated_priority);
}

void prompt_priority_parameters(
    FoodCategory *food_type,
    int *prep_time_min,
    float *distance_km,
    DeliveryTier *tier,
    EquipmentType *equipment,
    PriorityBreakdown *breakdown
) {
    char input_buf[64];

    printf("\n  " ANSI_BOLD ANSI_CYAN "=== Interactive Priority & Equipment Calculator ===" ANSI_RESET "\n");
    printf("  Select Food Category:\n");
    printf("    [1] Ice Cream / Gelato / Frozen Dessert (High Perishability)\n");
    printf("    [2] Hot Meal / Soups / Sizzler (Thermal Decay Risk)\n");
    printf("    [3] Bakery / Ambient / Pastry (Moderate Shelf-Life)\n");
    printf("    [4] Packaged Groceries / Beverages / Dry Goods (Standard)\n");
    printf("  Enter choice (1-4): ");
    
    int cat_choice = 2;
    if (fgets(input_buf, sizeof(input_buf), stdin)) {
        int parsed = atoi(input_buf);
        if (parsed >= 1 && parsed <= 4) cat_choice = parsed;
    }
    *food_type = (FoodCategory)cat_choice;

    printf("  Enter Kitchen Preparation Time in minutes (e.g., 5 to 45 min): ");
    int prep = 15;
    if (fgets(input_buf, sizeof(input_buf), stdin)) {
        int parsed = atoi(input_buf);
        if (parsed > 0) prep = parsed;
    }
    *prep_time_min = prep;

    printf("  Enter Distance between Restaurant and Delivery Destination in km (e.g., 1.5 to 18.0 km): ");
    float dist = 4.0f;
    if (fgets(input_buf, sizeof(input_buf), stdin)) {
        float parsed = (float)atof(input_buf);
        if (parsed > 0.1f) dist = parsed;
    }
    *distance_km = dist;

    printf("  Select Delivery Tier:\n");
    printf("    [1] VIP Delivery      (+30 pts: Premium Gold customer, top dispatch priority)\n");
    printf("    [2] Express Delivery  (+15 pts: Speed guarantee < 20 min, dedicated single driver)\n");
    printf("    [3] Standard Delivery (+0  pts: Fair-share baseline, standard prep queue)\n");
    printf("  Enter choice (1-3): ");
    int tier_choice = 3;
    if (fgets(input_buf, sizeof(input_buf), stdin)) {
        int parsed = atoi(input_buf);
        if (parsed == 1) tier_choice = 1;
        else if (parsed == 2) tier_choice = 2;
        else tier_choice = 3;
    }

    if (tier_choice == 1) *tier = TIER_VIP;
    else if (tier_choice == 2) *tier = TIER_EXPRESS;
    else *tier = TIER_STANDARD;

    *equipment = determine_required_equipment(*food_type);

    calculate_order_priority(*food_type, *prep_time_min, *distance_km, *tier, breakdown);
    print_priority_breakdown(breakdown);

    printf("  " ANSI_BOLD "  Auto-Assigned Equipment     : " ANSI_MAGENTA "%s\n" ANSI_RESET,
           equipment_to_string(*equipment));
}
