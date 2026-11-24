-- FeastOS direct SQL cookbook
-- Run this in pgAdmin Query Tool or psql against the feastos database.
-- These are examples: run only the INSERT statements you actually want.

-- 1) Inspect restaurants and their coordinates (the map uses latitude/longitude).
SELECT restaurant_id, name, address, latitude, longitude, status
FROM restaurants
ORDER BY restaurant_id;

-- 2) Add a restaurant. Replace the sample address and coordinates with its real location.
-- Coordinates are decimal degrees: latitude first, longitude second.
/* Copy and run this block after replacing the sample details.
INSERT INTO restaurants
    (name, cuisine, address, phone, rating, eta_min, delivery_fee_paise,
     offer, tag, latitude, longitude, hero_image_url, status)
VALUES
    ('SQL Demo Kitchen', 'North Indian · Snacks', 'Replace with restaurant address',
     NULL, 4.4, 25, 2000, 'Demo offer', 'New',
     30.3165, 78.0322, '', 'active')
RETURNING restaurant_id;
*/

-- 3) Add a menu category to an existing restaurant.
-- Replace 1 with the restaurant_id returned above.
/* Replace 1 with the actual restaurant_id, then run:
INSERT INTO menu_categories (restaurant_id, name)
VALUES (1, 'Main Course')
ON CONFLICT (restaurant_id, name) DO NOTHING;
*/

-- 4) Add a menu item. Price is in paise: ₹199.00 = 19900.
/* Replace 1 with the actual restaurant_id, then run:
INSERT INTO menu_items (category_id, name, description, price_paise, image_url, is_veg, is_available)
SELECT c.category_id, 'Paneer Wrap', 'Paneer and vegetables in a soft wrap',
       19900, '', TRUE, TRUE
FROM menu_categories c
WHERE c.restaurant_id = 1 AND c.name = 'Main Course'
RETURNING item_id;
*/

-- 5) Change a menu price or temporarily hide an item.
/* Replace item_id with the item you intend to change, then run one or both:
UPDATE menu_items SET price_paise = 20900 WHERE item_id = 1;
UPDATE menu_items SET is_available = FALSE WHERE item_id = 1;
*/

-- 6) Add restaurant kitchen resources/equipment.
/* Replace restaurant_id 1 with the target restaurant, then run:
INSERT INTO restaurant_equipment (restaurant_id, equipment_name, quantity, status, notes)
VALUES
    (1, 'Gas stove', 2, 'available', 'Two burners/stoves available'),
    (1, 'Refrigerator', 1, 'available', 'Main kitchen refrigerator'),
    (1, 'Deep fryer', 1, 'maintenance', 'Do not allocate while under maintenance');
*/

-- 7) Add delivery agents and two-wheelers.
/* Use fictional contact data and placeholder registrations for a demo only:
INSERT INTO delivery_agents (full_name, phone, status)
VALUES ('Demo Agent One', '9000000001', 'available'),
       ('Demo Agent Two', '9000000002', 'offline');

INSERT INTO vehicles (vehicle_type, registration_number, status, notes)
VALUES ('two_wheeler', 'DEMO-BIKE-001', 'available', 'Demo record; not a real registration'),
       ('two_wheeler', 'DEMO-BIKE-002', 'available', 'Demo record; not a real registration');
*/

-- 8) Assign an agent to a vehicle. Replace IDs with actual IDs from the tables.
-- Only one active assignment per agent and per vehicle is allowed by the unique indexes.
/* Replace both IDs with actual IDs after checking the tables, then run:
INSERT INTO agent_vehicle_assignments (agent_id, vehicle_id)
VALUES (1, 1);
*/

-- 9) Inspect kitchen equipment, delivery agents, vehicles and active assignments.
SELECT r.name AS restaurant, e.equipment_name, e.quantity, e.status
FROM restaurant_equipment e JOIN restaurants r USING (restaurant_id)
ORDER BY r.name, e.equipment_name;

SELECT agent_id, full_name, phone, status FROM delivery_agents ORDER BY agent_id;
SELECT vehicle_id, vehicle_type, registration_number, status FROM vehicles ORDER BY vehicle_id;
SELECT a.assignment_id, d.full_name, v.registration_number, a.assigned_at
FROM agent_vehicle_assignments a
JOIN delivery_agents d USING (agent_id)
JOIN vehicles v USING (vehicle_id)
WHERE a.unassigned_at IS NULL;

-- 10) Prefer disabling a restaurant over deleting it, so historical records remain meaningful.
/* Replace 1 with the restaurant_id you intend to suspend, then run:
UPDATE restaurants SET status = 'suspended' WHERE restaurant_id = 1;
*/
