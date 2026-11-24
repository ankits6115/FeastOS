BEGIN;

WITH new_restaurants (
    name, cuisine, address, latitude, longitude, hero_image_url
) AS (
    VALUES
    ('McDonald''s', 'Burgers, Fast Food',
     'Rajpur Road area, Dehradun, Uttarakhand',
     30.3279079, 78.0460471,
     'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=900&q=80'),

    ('Burger King - Astley Hall', 'Burgers, Fast Food',
     'No. 9, Astley Hall, Rajpur Road, Dehradun',
     30.3284000, 78.0465000,
     'https://images.unsplash.com/photo-1550547660-d9450f859349?auto=format&fit=crop&w=900&q=80'),

    ('Domino''s Pizza - Pacific Mall', 'Pizza',
     'Pacific Mall, Jakhan, Rajpur Road, Dehradun',
     30.3661602, 78.0706392,
     'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=900&q=80'),

    ('KFC - Pacific Mall', 'Fried Chicken, Fast Food',
     'Pacific Mall, Jakhan, Rajpur Road, Dehradun',
     30.3662799, 78.0705356,
     'https://images.unsplash.com/photo-1626082927389-6cd097cdc6ec?auto=format&fit=crop&w=900&q=80'),

    ('Subway - Clock Tower', 'Sandwiches, Fast Food',
     'Rajpur Road, near Bharat Petrol Pump, Dehradun',
     30.3254223, 78.0433537,
     'https://images.unsplash.com/photo-1528735602780-2552fd46c7af?auto=format&fit=crop&w=900&q=80'),

    ('Starbucks - Mall of Dehradun', 'Coffee, Cafe',
     'Mall of Dehradun, Haridwar Road',
     30.2668300, 78.0819300,
     'https://images.unsplash.com/photo-1509042239860-f550ce710b93?auto=format&fit=crop&w=900&q=80'),

    ('Barbeque Nation - Mall of Dehradun', 'Barbecue, Buffet',
     'Mall of Dehradun, Miyanwala, Dehradun',
     30.2666800, 78.0823800,
     'https://images.unsplash.com/photo-1555939594-58d7cb561ad1?auto=format&fit=crop&w=900&q=80'),

    ('Pizza Hut - Mall of Dehradun', 'Pizza',
     'Mall of Dehradun, Haridwar Road',
     30.2668300, 78.0819300,
     'https://images.unsplash.com/photo-1579751626657-72bc17010498?auto=format&fit=crop&w=900&q=80'),

    ('Taco Bell - Pacific Mall', 'Mexican Style, Fast Food',
     'Pacific Mall, Jakhan, Dehradun',
     30.3665000, 78.0707000,
     'https://images.unsplash.com/photo-1551504734-5ee1c4a1479b?auto=format&fit=crop&w=900&q=80'),

    ('Wow! Momo - Pacific Mall', 'Momos, Fast Food',
     'Pacific Mall, Jakhan, Dehradun',
     30.3665000, 78.0707000,
     'https://images.unsplash.com/photo-1563245372-f21724e3856d?auto=format&fit=crop&w=900&q=80'),

    ('Cafe Delhi Heights - Pacific Mall', 'Multi-Cuisine',
     'Pacific Mall, Jakhan, Dehradun',
     30.3665000, 78.0707000,
     'https://images.unsplash.com/photo-1517248135467-4c7edcad34c4?auto=format&fit=crop&w=900&q=80'),

    ('Chili''s - Pacific Mall', 'American, Tex-Mex',
     'Pacific Mall, Jakhan, Dehradun',
     30.3665000, 78.0707000,
     'https://images.unsplash.com/photo-1544025162-d76694265947?auto=format&fit=crop&w=900&q=80'),

    ('Kalsang', 'Tibetan, Chinese, Asian',
     'Dehradun city',
     30.3468500, 78.0625600,
     'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=900&q=80'),

    ('Black Pepper Restaurant', 'Indian, Chinese, Multi-Cuisine',
     '3, Rajpur Road, Astley Hall, Dehradun',
     30.3284000, 78.0465000,
     'https://images.unsplash.com/photo-1546833999-b9f581a1996d?auto=format&fit=crop&w=900&q=80')
)

INSERT INTO restaurants (
    name, cuisine, address, latitude, longitude,
    rating, eta_min, delivery_fee_paise,
    offer, tag, hero_image_url, status
)
SELECT
    n.name,
    n.cuisine,
    n.address,
    n.latitude,
    n.longitude,
    4.0,
    30,
    0,
    '',
    'Popular',
    n.hero_image_url,
    'active'
FROM new_restaurants n
WHERE NOT EXISTS (
    SELECT 1
    FROM restaurants r
    WHERE LOWER(r.name) = LOWER(n.name)
);

UPDATE restaurants
SET hero_image_url = CASE name
    WHEN 'McDonald''s' THEN
        'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=900&q=80'
    WHEN 'Burger King - Astley Hall' THEN
        'https://images.unsplash.com/photo-1550547660-d9450f859349?auto=format&fit=crop&w=900&q=80'
    WHEN 'Domino''s Pizza - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=900&q=80'
    WHEN 'KFC - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1626082927389-6cd097cdc6ec?auto=format&fit=crop&w=900&q=80'
    WHEN 'Subway - Clock Tower' THEN
        'https://images.unsplash.com/photo-1528735602780-2552fd46c7af?auto=format&fit=crop&w=900&q=80'
    WHEN 'Starbucks - Mall of Dehradun' THEN
        'https://images.unsplash.com/photo-1509042239860-f550ce710b93?auto=format&fit=crop&w=900&q=80'
    WHEN 'Barbeque Nation - Mall of Dehradun' THEN
        'https://images.unsplash.com/photo-1555939594-58d7cb561ad1?auto=format&fit=crop&w=900&q=80'
    WHEN 'Pizza Hut - Mall of Dehradun' THEN
        'https://images.unsplash.com/photo-1579751626657-72bc17010498?auto=format&fit=crop&w=900&q=80'
    WHEN 'Taco Bell - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1551504734-5ee1c4a1479b?auto=format&fit=crop&w=900&q=80'
    WHEN 'Wow! Momo - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1563245372-f21724e3856d?auto=format&fit=crop&w=900&q=80'
    WHEN 'Cafe Delhi Heights - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1517248135467-4c7edcad34c4?auto=format&fit=crop&w=900&q=80'
    WHEN 'Chili''s - Pacific Mall' THEN
        'https://images.unsplash.com/photo-1544025162-d76694265947?auto=format&fit=crop&w=900&q=80'
    WHEN 'Kalsang' THEN
        'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=900&q=80'
    WHEN 'Black Pepper Restaurant' THEN
        'https://images.unsplash.com/photo-1546833999-b9f581a1996d?auto=format&fit=crop&w=900&q=80'
    ELSE hero_image_url
END
WHERE LOWER(name) IN (
    'mcdonald''s',
    'burger king - astley hall',
    'domino''s pizza - pacific mall',
    'kfc - pacific mall',
    'subway - clock tower',
    'starbucks - mall of dehradun',
    'barbeque nation - mall of dehradun',
    'pizza hut - mall of dehradun',
    'taco bell - pacific mall',
    'wow! momo - pacific mall',
    'cafe delhi heights - pacific mall',
    'chili''s - pacific mall',
    'kalsang',
    'black pepper restaurant'
)
AND (hero_image_url IS NULL OR hero_image_url = '');

COMMIT;