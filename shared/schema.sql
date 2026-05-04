-- Sabre ALPR Hub - SQLite Schema
-- Optimized for WAL mode

CREATE TABLE IF NOT EXISTS hits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,         -- ISO8601 UTC
    plate_text TEXT NOT NULL,
    plate_confidence REAL,

    -- YMMV (Year, Make, Model, Color)
    vehicle_year TEXT,
    vehicle_make TEXT,
    vehicle_model TEXT,
    vehicle_color TEXT,
    ymmv_confidence REAL,

    -- Telemetry
    gps_latitude REAL,
    gps_longitude REAL,
    gps_speed_kph REAL,              -- Derived from GPS Delta
    gps_direction_deg REAL,          -- Derived from GPS Delta

    -- File Paths
    plate_crop_path TEXT,            -- Path on NVMe (/mnt/sabre_storage/crops/...)
    context_image_path TEXT,         -- Path on NVMe (/mnt/sabre_storage/context/...)

    -- Security
    sha256_hash TEXT NOT NULL,       -- Hashing Protocol signature
    hub_uuid TEXT NOT NULL,

    -- Offload Status
    is_offloaded INTEGER DEFAULT 0   -- 0: No, 1: Yes
);

CREATE TABLE IF NOT EXISTS hotlist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plate_text TEXT UNIQUE NOT NULL,
    description TEXT,
    category TEXT,                   -- e.g., "Stolen", "Amber Alert"
    added_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_hits_timestamp ON hits(timestamp);
CREATE INDEX IF NOT EXISTS idx_hits_plate ON hits(plate_text);
CREATE INDEX IF NOT EXISTS idx_hits_offload ON hits(is_offloaded);
CREATE INDEX IF NOT EXISTS idx_hotlist_plate ON hotlist(plate_text);
