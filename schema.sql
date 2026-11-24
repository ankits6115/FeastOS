BEGIN;

CREATE TABLE IF NOT EXISTS restaurants (
    restaurant_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    cuisine VARCHAR(200) NOT NULL,
    address TEXT NOT NULL,
    phone VARCHAR(30),
    rating NUMERIC(2,1) NOT NULL DEFAULT 0 CHECK (rating >= 0 AND rating <= 5),
    eta_min INTEGER NOT NULL DEFAULT 30 CHECK (eta_min > 0),
    delivery_fee_paise BIGINT NOT NULL DEFAULT 0 CHECK (delivery_fee_paise >= 0),
    offer VARCHAR(200) NOT NULL DEFAULT '',
    tag VARCHAR(100) NOT NULL DEFAULT '',
    latitude DOUBLE PRECISION NOT NULL DEFAULT 12.9716 CHECK (latitude BETWEEN -90 AND 90),
    longitude DOUBLE PRECISION NOT NULL DEFAULT 77.5946 CHECK (longitude BETWEEN -180 AND 180),
    hero_image_url TEXT NOT NULL DEFAULT '',
    status VARCHAR(20) NOT NULL DEFAULT 'active'
        CHECK (status IN ('pending','active','suspended')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS menu_categories (
    category_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    restaurant_id BIGINT NOT NULL REFERENCES restaurants(restaurant_id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    UNIQUE (restaurant_id, name)
);

CREATE TABLE IF NOT EXISTS menu_items (
    item_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    category_id BIGINT NOT NULL REFERENCES menu_categories(category_id) ON DELETE CASCADE,
    name VARCHAR(150) NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    price_paise BIGINT NOT NULL CHECK (price_paise >= 0),
    image_url TEXT NOT NULL DEFAULT '',
    is_veg BOOLEAN NOT NULL DEFAULT TRUE,
    is_available BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_restaurants_status ON restaurants(status);
CREATE INDEX IF NOT EXISTS idx_menu_categories_restaurant ON menu_categories(restaurant_id);
CREATE INDEX IF NOT EXISTS idx_menu_items_category_available ON menu_items(category_id, is_available);
ALTER TABLE menu_items ADD COLUMN IF NOT EXISTS perishability_score INTEGER NOT NULL DEFAULT 35;
ALTER TABLE menu_items ADD COLUMN IF NOT EXISTS preparation_time_min INTEGER NOT NULL DEFAULT 20;


-- Users for email login and signup
CREATE TABLE IF NOT EXISTS users (
    user_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    full_name VARCHAR(150) NOT NULL DEFAULT '',
    password_hash VARCHAR(255) NOT NULL DEFAULT '',
    phone VARCHAR(30) NOT NULL DEFAULT '',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(lower(email));

-- Orders and Razorpay payment records. Orders remain pending until the
-- backend verifies the Razorpay checkout signature.
CREATE TABLE IF NOT EXISTS orders (
    order_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    user_id BIGINT REFERENCES users(user_id),
    user_email VARCHAR(255) NOT NULL DEFAULT '',
    status VARCHAR(30) NOT NULL DEFAULT 'placed'
        CHECK (status IN ('placed','confirmed','cancelled','placed_demo','confirmed_demo','cancelled_demo')),
    payment_method VARCHAR(20) NOT NULL DEFAULT 'RAZORPAY'
        CHECK (payment_method IN ('RAZORPAY','DEMO','COD')),
    payment_status VARCHAR(30) NOT NULL DEFAULT 'pending'
        CHECK (payment_status IN ('pending','paid','failed','simulated_success','simulated_failed')),
    subtotal_paise BIGINT NOT NULL DEFAULT 0 CHECK (subtotal_paise >= 0),
    delivery_fee_paise BIGINT NOT NULL DEFAULT 2900 CHECK (delivery_fee_paise >= 0),
    platform_fee_paise BIGINT NOT NULL DEFAULT 500 CHECK (platform_fee_paise >= 0),
    total_paise BIGINT NOT NULL DEFAULT 0 CHECK (total_paise >= 0),
    restaurant_id BIGINT REFERENCES restaurants(restaurant_id),
    delivery_address TEXT NOT NULL DEFAULT '',
    delivery_latitude DOUBLE PRECISION CHECK (delivery_latitude BETWEEN -90 AND 90),
    delivery_longitude DOUBLE PRECISION CHECK (delivery_longitude BETWEEN -180 AND 180),
    distance_km DOUBLE PRECISION CHECK (distance_km IS NULL OR distance_km >= 0),
    razorpay_order_id VARCHAR(80) UNIQUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Upgrade existing installations without deleting prior order history.
ALTER TABLE orders ADD COLUMN IF NOT EXISTS user_id BIGINT REFERENCES users(user_id);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS user_email VARCHAR(255) NOT NULL DEFAULT '';
ALTER TABLE orders ADD COLUMN IF NOT EXISTS restaurant_id BIGINT REFERENCES restaurants(restaurant_id);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS delivery_address TEXT NOT NULL DEFAULT '';
ALTER TABLE orders ADD COLUMN IF NOT EXISTS delivery_latitude DOUBLE PRECISION;
ALTER TABLE orders ADD COLUMN IF NOT EXISTS delivery_longitude DOUBLE PRECISION;
ALTER TABLE orders ADD COLUMN IF NOT EXISTS distance_km DOUBLE PRECISION;
ALTER TABLE orders ADD COLUMN IF NOT EXISTS razorpay_order_id VARCHAR(80);
ALTER TABLE orders DROP CONSTRAINT IF EXISTS orders_status_check;
ALTER TABLE orders DROP CONSTRAINT IF EXISTS orders_payment_method_check;
ALTER TABLE orders DROP CONSTRAINT IF EXISTS orders_payment_status_check;
ALTER TABLE orders ADD CONSTRAINT orders_status_check CHECK (status IN ('placed','confirmed','cancelled','placed_demo','confirmed_demo','cancelled_demo'));
ALTER TABLE orders ADD CONSTRAINT orders_payment_method_check CHECK (payment_method IN ('RAZORPAY','DEMO','COD'));
ALTER TABLE orders ADD CONSTRAINT orders_payment_status_check CHECK (payment_status IN ('pending','paid','failed','simulated_success','simulated_failed'));
CREATE UNIQUE INDEX IF NOT EXISTS idx_orders_razorpay_order_id ON orders(razorpay_order_id) WHERE razorpay_order_id IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_orders_restaurant_id ON orders(restaurant_id);

CREATE TABLE IF NOT EXISTS order_items (
    order_item_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    order_id BIGINT NOT NULL REFERENCES orders(order_id) ON DELETE CASCADE,
    restaurant_id BIGINT NOT NULL REFERENCES restaurants(restaurant_id),
    menu_item_id BIGINT REFERENCES menu_items(item_id) ON DELETE SET NULL,
    item_name_snapshot VARCHAR(150) NOT NULL,
    unit_price_paise BIGINT NOT NULL CHECK (unit_price_paise >= 0),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    line_total_paise BIGINT NOT NULL CHECK (line_total_paise >= 0)
);

CREATE TABLE IF NOT EXISTS payment_transactions (
    transaction_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    order_id BIGINT NOT NULL REFERENCES orders(order_id) ON DELETE RESTRICT,
    transaction_ref VARCHAR(80) NOT NULL UNIQUE,
    transaction_type VARCHAR(30) NOT NULL DEFAULT 'RAZORPAY_PAYMENT',
    provider VARCHAR(40) NOT NULL DEFAULT 'RAZORPAY',
    status VARCHAR(30) NOT NULL DEFAULT 'created',
    amount_paise BIGINT NOT NULL CHECK (amount_paise >= 0),
    currency CHAR(3) NOT NULL DEFAULT 'INR',
    note TEXT NOT NULL DEFAULT 'Awaiting Razorpay payment verification.',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
ALTER TABLE payment_transactions DROP CONSTRAINT IF EXISTS payment_transactions_transaction_type_check;
ALTER TABLE payment_transactions DROP CONSTRAINT IF EXISTS payment_transactions_status_check;
ALTER TABLE payment_transactions ADD CONSTRAINT payment_transactions_transaction_type_check CHECK (transaction_type IN ('RAZORPAY_PAYMENT','DEMO_PAYMENT'));
ALTER TABLE payment_transactions ADD CONSTRAINT payment_transactions_status_check CHECK (status IN ('created','captured','failed','SIMULATED_SUCCESS','SIMULATED_FAILED'));

CREATE INDEX IF NOT EXISTS idx_orders_created_at ON orders(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_order_items_order ON order_items(order_id);
CREATE INDEX IF NOT EXISTS idx_payment_transactions_order ON payment_transactions(order_id);

-- Operational data that can be managed directly through pgAdmin/psql.
CREATE TABLE IF NOT EXISTS delivery_agents (
    agent_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    full_name VARCHAR(150) NOT NULL,
    phone VARCHAR(30) UNIQUE,
    status VARCHAR(24) NOT NULL DEFAULT 'offline'
        CHECK (status IN ('offline','available','on_delivery','inactive')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS vehicles (
    vehicle_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    vehicle_type VARCHAR(30) NOT NULL DEFAULT 'two_wheeler'
        CHECK (vehicle_type IN ('two_wheeler','bicycle','three_wheeler','other')),
    registration_number VARCHAR(40) UNIQUE NOT NULL,
    status VARCHAR(24) NOT NULL DEFAULT 'available'
        CHECK (status IN ('available','in_use','maintenance','inactive')),
    notes TEXT NOT NULL DEFAULT '',
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS agent_vehicle_assignments (
    assignment_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    agent_id BIGINT NOT NULL REFERENCES delivery_agents(agent_id),
    vehicle_id BIGINT NOT NULL REFERENCES vehicles(vehicle_id),
    assigned_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    unassigned_at TIMESTAMPTZ,
    CHECK (unassigned_at IS NULL OR unassigned_at >= assigned_at)
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_active_vehicle_assignment
    ON agent_vehicle_assignments(vehicle_id) WHERE unassigned_at IS NULL;
CREATE UNIQUE INDEX IF NOT EXISTS uq_active_agent_assignment
    ON agent_vehicle_assignments(agent_id) WHERE unassigned_at IS NULL;

CREATE TABLE IF NOT EXISTS restaurant_equipment (
    equipment_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    restaurant_id BIGINT NOT NULL REFERENCES restaurants(restaurant_id) ON DELETE CASCADE,
    equipment_name VARCHAR(120) NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 1 CHECK (quantity >= 0),
    status VARCHAR(24) NOT NULL DEFAULT 'available'
        CHECK (status IN ('available','maintenance','inactive')),
    notes TEXT NOT NULL DEFAULT '',
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_restaurant_equipment_restaurant
    ON restaurant_equipment(restaurant_id);

CREATE TABLE IF NOT EXISTS equipment_maintenance (
    maintenance_id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    equipment_id BIGINT NOT NULL REFERENCES restaurant_equipment(equipment_id) ON DELETE CASCADE,
    description TEXT NOT NULL,
    maintenance_date DATE NOT NULL DEFAULT CURRENT_DATE,
    completed BOOLEAN NOT NULL DEFAULT FALSE,
    notes TEXT NOT NULL DEFAULT ''
);

COMMIT;
