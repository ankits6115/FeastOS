#ifndef CLI_PRIORITY_H
#define CLI_PRIORITY_H

#include "common.h"

/* =========================================================================
 * Module: Priority Calculation Engine
 * Author: Tanvi Chauhan (240222004) - TCS-502 / TCS-503 Team Foodsters
 * Description:
 *   Computes priority scores (1 to 100) dynamically using multi-factor OS
 *   priority formula:
 *     - Food Type & Perishability (Ice cream vs Hot meal vs Dry items)
 *     - Kitchen Preparation Time (Minutes)
 *     - Delivery Distance between Restaurant & Destination (Kilometers)
 *     - Customer Delivery Urgency Tier (VIP/Express vs Standard)
 *   Also determines the specialized equipment required (Insulated Bag / Cold Box).
 * ========================================================================= */

/* Calculates priority based on multi-factor heuristic formula */
int calculate_order_priority(
    FoodCategory food_type,
    int prep_time_min,
    float distance_km,
    DeliveryTier tier,
    PriorityBreakdown *breakdown
);

/* Determines necessary hardware equipment based on food category */
EquipmentType determine_required_equipment(FoodCategory food_type);

/* Interactive CLI helper: Prompts user step-by-step with menu selections */
void prompt_priority_parameters(
    FoodCategory *food_type,
    int *prep_time_min,
    float *distance_km,
    DeliveryTier *tier,
    EquipmentType *equipment,
    PriorityBreakdown *breakdown
);

/* Formatted breakdown display for terminal */
void print_priority_breakdown(const PriorityBreakdown *breakdown);

#endif /* CLI_PRIORITY_H */
