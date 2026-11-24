-- Optional demo data. Run schema.sql first, then this file. Safe to rerun.
INSERT INTO restaurants(name,cuisine,address,phone,rating,eta_min,delivery_fee_paise,offer,tag,latitude,longitude,hero_image_url,status)
SELECT v.name,v.cuisine,v.address,v.phone,v.rating,v.eta_min,v.delivery_fee_paise,v.offer,v.tag,v.latitude,v.longitude,v.hero_image_url,'active'
FROM (VALUES
 ('Mountain Bites','North Indian · Thalis · Curry','Indiranagar, Bengaluru','08000000001',4.7::numeric,25,2500::bigint,'10% OFF up to ₹100','Local favourite',12.9716::double precision,77.6412::double precision,'https://images.unsplash.com/photo-1585937421612-70a008356fbe?auto=format&fit=crop&w=900&q=85'),
 ('Pizza Corner','Pizza · Italian · Pasta','Domlur, Bengaluru','08000000002',4.6::numeric,30,2000::bigint,'Free dip with pizza','Popular near you',12.9600::double precision,77.6380::double precision,'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=900&q=85'),
 ('Wok Express','Chinese · Noodles · Rice','Koramangala, Bengaluru','08000000003',4.5::numeric,20,1500::bigint,'₹50 OFF above ₹399','Quick bites',12.9352::double precision,77.6245::double precision,'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=900&q=85')
) AS v(name,cuisine,address,phone,rating,eta_min,delivery_fee_paise,offer,tag,latitude,longitude,hero_image_url)
WHERE NOT EXISTS (SELECT 1 FROM restaurants r WHERE r.name=v.name);

INSERT INTO menu_categories(restaurant_id,name)
SELECT r.restaurant_id,v.category
FROM (VALUES
 ('Mountain Bites','Main Course'),('Mountain Bites','Rice'),
 ('Pizza Corner','Pizza'),('Wok Express','Noodles'),('Wok Express','Rice')
) AS v(restaurant_name,category)
JOIN restaurants r ON r.name=v.restaurant_name
ON CONFLICT (restaurant_id,name) DO NOTHING;

INSERT INTO menu_items(category_id,name,description,price_paise,image_url,is_veg,perishability_score,preparation_time_min)
SELECT c.category_id,v.item_name,v.description,v.price_paise,v.image_url,v.is_veg,
       v.perishability_score,v.preparation_time_min
FROM (VALUES
 ('Mountain Bites','Main Course','Paneer Butter Masala','Paneer in a creamy tomato gravy',28900::bigint,'https://images.unsplash.com/photo-1631452180519-c014fe946bc7?auto=format&fit=crop&w=700&q=85',true,40,20),
 ('Mountain Bites','Rice','Veg Biryani','Fragrant basmati rice with herbs',24900::bigint,'https://images.unsplash.com/photo-1563379091339-03246963d96c?auto=format&fit=crop&w=700&q=85',true,30,20),
 ('Pizza Corner','Pizza','Margherita Pizza','Tomato, mozzarella and basil',29900::bigint,'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=700&q=85',true,30,15),
 ('Pizza Corner','Pizza','Classic Chicken Pizza','Chicken, cheese and herbs',37900::bigint,'https://images.unsplash.com/photo-1579751626657-72bc17010498?auto=format&fit=crop&w=700&q=85',false,35,20),
 ('Wok Express','Noodles','Veg Hakka Noodles','Wok-tossed noodles and vegetables',19900::bigint,'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=700&q=85',true,30,12),
 ('Wok Express','Rice','Chicken Fried Rice','Rice with chicken, egg and vegetables',25900::bigint,'https://images.unsplash.com/photo-1603133872878-684f208fb84b?auto=format&fit=crop&w=700&q=85',false,40,15)
) AS v(restaurant_name,category,item_name,description,price_paise,image_url,is_veg,perishability_score,preparation_time_min)
JOIN restaurants r ON r.name=v.restaurant_name
JOIN menu_categories c ON c.restaurant_id=r.restaurant_id AND c.name=v.category
WHERE NOT EXISTS (SELECT 1 FROM menu_items m WHERE m.category_id=c.category_id AND m.name=v.item_name);


-- Refresh perishability and preparation estimates on existing demo menu rows.
UPDATE menu_items AS m
SET perishability_score = v.perishability_score,
    preparation_time_min = v.preparation_time_min
FROM (
    VALUES
      ('Mountain Bites','Main Course','Paneer Butter Masala',40,20),
      ('Mountain Bites','Rice','Veg Biryani',30,20),
      ('Pizza Corner','Pizza','Margherita Pizza',30,15),
      ('Pizza Corner','Pizza','Classic Chicken Pizza',35,20),
      ('Wok Express','Noodles','Veg Hakka Noodles',30,12),
      ('Wok Express','Rice','Chicken Fried Rice',40,15)
) AS v(restaurant_name,category,item_name,perishability_score,preparation_time_min)
JOIN restaurants r ON r.name=v.restaurant_name
JOIN menu_categories c ON c.restaurant_id=r.restaurant_id AND c.name=v.category
WHERE m.category_id=c.category_id AND m.name=v.item_name;
