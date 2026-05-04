-- Sabre ALPR Hub - SQLite Schema
-- Optimized for NVMe, High-Vibration, and Forensic Audit
PRAGMA journal_mode=WAL;
PRAGMA synchronous=NORMAL;
PRAGMA cache_size=-64000; -- 64MB Cache

CREATE TABLE IF NOT EXISTS shifts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    officer_id TEXT NOT NULL,
    vehicle_id TEXT NOT NULL,
    disclaimer_accepted_at TEXT NOT NULL, -- ISO8601 UTC
    shift_start TEXT NOT NULL,            -- ISO8601 UTC
    shift_end TEXT                        -- ISO8601 UTC (NULL until end)
);

CREATE TABLE IF NOT EXISTS hits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    shift_id INTEGER,                -- Linked to shifts.id
    timestamp_iso8601 TEXT NOT NULL,
    plate_text TEXT NOT NULL,
    plate_confidence REAL,
    ymmv_json TEXT,
    gps_lat REAL,
    gps_long REAL,
    speed_kph REAL,
    direction_deg REAL,
    image_path_ir TEXT,
    image_path_color TEXT,
    sha256_hash TEXT NOT NULL,
    hub_uuid TEXT NOT NULL,
    is_offloaded INTEGER DEFAULT 0,
    FOREIGN KEY(shift_id) REFERENCES shifts(id)
);

CREATE TABLE IF NOT EXISTS hotlist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plate_text TEXT UNIQUE NOT NULL,
    description TEXT,
    category TEXT,
    added_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_hits_timestamp ON hits(timestamp_iso8601);
CREATE INDEX IF NOT EXISTS idx_hits_plate ON hits(plate_text);
CREATE INDEX IF NOT EXISTS idx_hits_shift ON hits(shift_id);
CREATE INDEX IF NOT EXISTS idx_hotlist_plate ON hotlist(plate_text);
