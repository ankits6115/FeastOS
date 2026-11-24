
BEGIN;

WITH menu_data (
    restaurant_name, category_name, item_name,
    description, price_paise, is_veg
) AS (
    VALUES
    -- McDonald's
    ('McDonald''s', 'Popular Items', 'McAloo Tikki Burger', 'Crispy potato patty burger', 6900::bigint, TRUE),
    ('McDonald''s', 'Popular Items', 'McChicken Burger', 'Chicken burger with lettuce and sauce', 15900::bigint, FALSE),
    ('McDonald''s', 'Popular Items', 'French Fries', 'Classic salted fries', 9900::bigint, TRUE),

    -- Burger King
    ('Burger King - Astley Hall', 'Popular Items', 'Veg Whopper', 'Veg patty with fresh vegetables and sauces', 17900::bigint, TRUE),
    ('Burger King - Astley Hall', 'Popular Items', 'Chicken Whopper', 'Grilled chicken burger with sauces', 19900::bigint, FALSE),
    ('Burger King - Astley Hall', 'Popular Items', 'Crispy Fries', 'Golden crispy potato fries', 9900::bigint, TRUE),

    -- Domino's
    ('Domino''s Pizza - Pacific Mall', 'Popular Items', 'Margherita Pizza', 'Classic pizza with cheese and tomato', 19900::bigint, TRUE),
    ('Domino''s Pizza - Pacific Mall', 'Popular Items', 'Farmhouse Pizza', 'Pizza with vegetables and cheese', 34900::bigint, TRUE),
    ('Domino''s Pizza - Pacific Mall', 'Popular Items', 'Chicken Dominator Pizza', 'Chicken pizza with assorted toppings', 49900::bigint, FALSE),

    -- KFC
    ('KFC - Pacific Mall', 'Popular Items', 'Hot and Crispy Chicken', 'Crispy seasoned fried chicken', 19900::bigint, FALSE),
    ('KFC - Pacific Mall', 'Popular Items', 'Chicken Zinger Burger', 'Crispy chicken burger', 18900::bigint, FALSE),
    ('KFC - Pacific Mall', 'Popular Items', 'Peri Peri Fries', 'Fries with peri peri seasoning', 10900::bigint, TRUE),

    -- Subway
    ('Subway - Clock Tower', 'Popular Items', 'Veggie Delite Sub', 'Vegetable sub with sauces', 14900::bigint, TRUE),
    ('Subway - Clock Tower', 'Popular Items', 'Paneer Tikka Sub', 'Paneer tikka with fresh vegetables', 22900::bigint, TRUE),
    ('Subway - Clock Tower', 'Popular Items', 'Chicken Teriyaki Sub', 'Chicken sub with teriyaki-style sauce', 27900::bigint, FALSE),

    -- Starbucks
    ('Starbucks - Mall of Dehradun', 'Popular Items', 'Caffe Latte', 'Espresso with steamed milk', 26000::bigint, TRUE),
    ('Starbucks - Mall of Dehradun', 'Popular Items', 'Cappuccino', 'Espresso with steamed milk foam', 28000::bigint, TRUE),
    ('Starbucks - Mall of Dehradun', 'Popular Items', 'Chocolate Brownie', 'Chocolate brownie dessert', 22000::bigint, TRUE),

    -- Barbeque Nation
    ('Barbeque Nation - Mall of Dehradun', 'Popular Items', 'Paneer Tikka', 'Grilled marinated paneer', 27900::bigint, TRUE),
    ('Barbeque Nation - Mall of Dehradun', 'Popular Items', 'Chicken Tikka', 'Grilled marinated chicken', 34900::bigint, FALSE),
    ('Barbeque Nation - Mall of Dehradun', 'Popular Items', 'Veg Seekh Kebab', 'Spiced vegetable kebabs', 24900::bigint, TRUE),

    -- Pizza Hut
    ('Pizza Hut - Mall of Dehradun', 'Popular Items', 'Classic Margherita', 'Cheese and tomato pizza', 19900::bigint, TRUE),
    ('Pizza Hut - Mall of Dehradun', 'Popular Items', 'Country Feast Pizza', 'Vegetable-loaded pizza', 32900::bigint, TRUE),
    ('Pizza Hut - Mall of Dehradun', 'Popular Items', 'Chicken Sausage Pizza', 'Pizza topped with chicken sausage', 34900::bigint, FALSE),

    -- Taco Bell
    ('Taco Bell - Pacific Mall', 'Popular Items', 'Crunchy Taco', 'Crunchy taco with seasoned filling', 9900::bigint, FALSE),
    ('Taco Bell - Pacific Mall', 'Popular Items', 'Bean Burrito', 'Beans, rice and sauce in a tortilla', 14900::bigint, TRUE),
    ('Taco Bell - Pacific Mall', 'Popular Items', 'Cheesy Fries', 'Fries topped with cheese sauce', 12900::bigint, TRUE),

    -- Wow! Momo
    ('Wow! Momo - Pacific Mall', 'Popular Items', 'Steamed Veg Momos', 'Steamed vegetable dumplings', 12900::bigint, TRUE),
    ('Wow! Momo - Pacific Mall', 'Popular Items', 'Chicken Momos', 'Steamed chicken dumplings', 15900::bigint, FALSE),
    ('Wow! Momo - Pacific Mall', 'Popular Items', 'Kurkure Momos', 'Crispy coated momos', 18900::bigint, TRUE),

    -- Cafe Delhi Heights
    ('Cafe Delhi Heights - Pacific Mall', 'Popular Items', 'Veggie Burger', 'Vegetable patty burger', 29900::bigint, TRUE),
    ('Cafe Delhi Heights - Pacific Mall', 'Popular Items', 'Chicken Tikka Sandwich', 'Chicken tikka sandwich', 34900::bigint, FALSE),
    ('Cafe Delhi Heights - Pacific Mall', 'Popular Items', 'Penne Alfredo', 'Penne pasta in creamy white sauce', 37900::bigint, TRUE),

    -- Chili's
    ('Chili''s - Pacific Mall', 'Popular Items', 'Loaded Nachos', 'Nachos with cheese and toppings', 32900::bigint, TRUE),
    ('Chili''s - Pacific Mall', 'Popular Items', 'Chicken Wings', 'Seasoned chicken wings with sauce', 39900::bigint, FALSE),
    ('Chili''s - Pacific Mall', 'Popular Items', 'Classic Chicken Burger', 'Chicken burger with lettuce and sauce', 37900::bigint, FALSE),

    -- Kalsang
    ('Kalsang', 'Popular Items', 'Veg Thukpa', 'Tibetan noodle soup with vegetables', 19900::bigint, TRUE),
    ('Kalsang', 'Popular Items', 'Chicken Momos', 'Chicken-filled dumplings', 21900::bigint, FALSE),
    ('Kalsang', 'Popular Items', 'Veg Chowmein', 'Stir-fried noodles with vegetables', 18900::bigint, TRUE),

    -- Black Pepper Restaurant
    ('Black Pepper Restaurant', 'Popular Items', 'Paneer Butter Masala', 'Paneer in creamy tomato gravy', 29900::bigint, TRUE),
    ('Black Pepper Restaurant', 'Popular Items', 'Chicken Curry', 'Chicken cooked in Indian spices', 34900::bigint, FALSE),
    ('Black Pepper Restaurant', 'Popular Items', 'Dal Tadka', 'Yellow lentils tempered with spices', 19900::bigint, TRUE)
),
target_restaurants AS (
    SELECT DISTINCT
        r.restaurant_id,
        r.name AS restaurant_name,
        d.category_name
    FROM menu_data d
    JOIN restaurants r
      ON LOWER(r.name) = LOWER(d.restaurant_name)
),
upserted_categories AS (
    INSERT INTO menu_categories (restaurant_id, name)
    SELECT DISTINCT restaurant_id, category_name
    FROM target_restaurants
    ON CONFLICT (restaurant_id, name)
    DO UPDATE SET name = EXCLUDED.name
    RETURNING category_id, restaurant_id, name
)
INSERT INTO menu_items (
    category_id, name, description,
    price_paise, image_url, is_veg
)
SELECT
    c.category_id,
    d.item_name,
    d.description,
    d.price_paise,
    '',
    d.is_veg
FROM menu_data d
JOIN restaurants r
  ON LOWER(r.name) = LOWER(d.restaurant_name)
JOIN upserted_categories c
  ON c.restaurant_id = r.restaurant_id
 AND c.name = d.category_name
WHERE NOT EXISTS (
    SELECT 1
    FROM menu_items existing
    WHERE existing.category_id = c.category_id
      AND LOWER(existing.name) = LOWER(d.item_name)
);

COMMIT;
BEGIN;

UPDATE menu_items SET image_url = CASE item_id
  WHEN 1 THEN 'https://images.unsplash.com/photo-1528735602780-2552fd46c7af?auto=format&fit=crop&w=700&q=85'
  WHEN 2 THEN 'https://images.unsplash.com/photo-1517256064527-09c73fc73e38?auto=format&fit=crop&w=700&q=85'
  WHEN 3 THEN 'https://images.unsplash.com/photo-1565299624946-b28f40a0ae38?auto=format&fit=crop&w=700&q=85'
  WHEN 4 THEN 'https://images.unsplash.com/photo-1574071318508-1cdbab80d002?auto=format&fit=crop&w=700&q=85'
  WHEN 5 THEN 'https://images.unsplash.com/photo-1603133872878-684f208fb84b?auto=format&fit=crop&w=700&q=85'
  WHEN 6 THEN 'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=700&q=85'
  WHEN 7 THEN 'https://images.unsplash.com/photo-1589302168068-964664d93dc0?auto=format&fit=crop&w=700&q=85'
  WHEN 8 THEN 'https://images.unsplash.com/photo-1631452180519-c014fe946bc7?auto=format&fit=crop&w=700&q=85'
  WHEN 9 THEN 'https://images.unsplash.com/photo-1550547660-d9450f859349?auto=format&fit=crop&w=700&q=85'
  WHEN 10 THEN 'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=700&q=85'
  WHEN 11 THEN 'https://images.unsplash.com/photo-1573080496219-bb080dd4f877?auto=format&fit=crop&w=700&q=85'
  WHEN 12 THEN 'https://images.unsplash.com/photo-1550547660-d9450f859349?auto=format&fit=crop&w=700&q=85'
  WHEN 13 THEN 'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=700&q=85'
  WHEN 14 THEN 'https://images.unsplash.com/photo-1573080496219-bb080dd4f877?auto=format&fit=crop&w=700&q=85'
  WHEN 15 THEN 'https://images.unsplash.com/photo-1574071318508-1cdbab80d002?auto=format&fit=crop&w=700&q=85'
  WHEN 16 THEN 'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=700&q=85'
  WHEN 17 THEN 'https://images.unsplash.com/photo-1565299624946-b28f40a0ae38?auto=format&fit=crop&w=700&q=85'
  WHEN 18 THEN 'https://images.unsplash.com/photo-1567620832903-9fc6debc209f?auto=format&fit=crop&w=700&q=85'
  WHEN 19 THEN 'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=700&q=85'
  WHEN 20 THEN 'https://images.unsplash.com/photo-1585109649139-366815a0d713?auto=format&fit=crop&w=700&q=85'
  WHEN 21 THEN 'https://images.unsplash.com/photo-1509722747041-616f39b57569?auto=format&fit=crop&w=700&q=85'
  WHEN 22 THEN 'https://images.unsplash.com/photo-1509722747041-616f39b57569?auto=format&fit=crop&w=700&q=85'
  WHEN 23 THEN 'https://images.unsplash.com/photo-1509722747041-616f39b57569?auto=format&fit=crop&w=700&q=85'
  WHEN 24 THEN 'https://images.unsplash.com/photo-1534778101976-62847782c213?auto=format&fit=crop&w=700&q=85'
  WHEN 25 THEN 'https://images.unsplash.com/photo-1572442388796-11668a67e53d?auto=format&fit=crop&w=700&q=85'
  WHEN 26 THEN 'https://images.unsplash.com/photo-1606313564200-e75d5e30476c?auto=format&fit=crop&w=700&q=85'
  WHEN 27 THEN 'https://images.unsplash.com/photo-1567188040759-fb8a883dc6d8?auto=format&fit=crop&w=700&q=85'
  WHEN 28 THEN 'https://images.unsplash.com/photo-1532550907401-a500c9a57435?auto=format&fit=crop&w=700&q=85'
  WHEN 29 THEN 'https://images.unsplash.com/photo-1603360946369-dc9bb6258143?auto=format&fit=crop&w=700&q=85'
  WHEN 30 THEN 'https://images.unsplash.com/photo-1574071318508-1cdbab80d002?auto=format&fit=crop&w=700&q=85'
  WHEN 31 THEN 'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=700&q=85'
  WHEN 32 THEN 'https://images.unsplash.com/photo-1565299624946-b28f40a0ae38?auto=format&fit=crop&w=700&q=85'
  WHEN 33 THEN 'https://images.unsplash.com/photo-1551504734-5ee1c4a1479b?auto=format&fit=crop&w=700&q=85'
  WHEN 34 THEN 'https://images.unsplash.com/photo-1626700051175-6818013e1d4f?auto=format&fit=crop&w=700&q=85'
  WHEN 35 THEN 'https://images.unsplash.com/photo-1585109649139-366815a0d713?auto=format&fit=crop&w=700&q=85'
  WHEN 36 THEN 'https://images.unsplash.com/photo-1534422298391-e4f8c172dddb?auto=format&fit=crop&w=700&q=85'
  WHEN 37 THEN 'https://images.unsplash.com/photo-1534422298391-e4f8c172dddb?auto=format&fit=crop&w=700&q=85'
  WHEN 38 THEN 'https://images.unsplash.com/photo-1534422298391-e4f8c172dddb?auto=format&fit=crop&w=700&q=85'
  WHEN 39 THEN 'https://images.unsplash.com/photo-1550547660-d9450f859349?auto=format&fit=crop&w=700&q=85'
  WHEN 40 THEN 'https://images.unsplash.com/photo-1509722747041-616f39b57569?auto=format&fit=crop&w=700&q=85'
  WHEN 41 THEN 'https://images.unsplash.com/photo-1473093295043-cdd812d0e601?auto=format&fit=crop&w=700&q=85'
  WHEN 42 THEN 'https://images.unsplash.com/photo-1513456852971-30c0b8199d4d?auto=format&fit=crop&w=700&q=85'
  WHEN 43 THEN 'https://images.unsplash.com/photo-1567620832903-9fc6debc209f?auto=format&fit=crop&w=700&q=85'
  WHEN 44 THEN 'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=700&q=85'
  WHEN 45 THEN 'https://images.unsplash.com/photo-1547928576-a4a33237cbc3?auto=format&fit=crop&w=700&q=85'
  WHEN 46 THEN 'https://images.unsplash.com/photo-1534422298391-e4f8c172dddb?auto=format&fit=crop&w=700&q=85'
  WHEN 47 THEN 'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=700&q=85'
  WHEN 48 THEN 'https://images.unsplash.com/photo-1631452180519-c014fe946bc7?auto=format&fit=crop&w=700&q=85'
  WHEN 49 THEN 'https://images.unsplash.com/photo-1588166524941-3bf61a9c41db?auto=format&fit=crop&w=700&q=85'
  WHEN 50 THEN 'https://images.unsplash.com/photo-1546833999-b9f581a1996d?auto=format&fit=crop&w=700&q=85'
  ELSE image_url
END;

COMMIT;

