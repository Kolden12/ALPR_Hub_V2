-- Sabre ALPR Hub - SQLite Schema
-- Optimized for WAL mode
PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS hits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
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
    is_offloaded INTEGER DEFAULT 0
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
CREATE INDEX IF NOT EXISTS idx_hits_offload ON hits(is_offloaded);
CREATE INDEX IF NOT EXISTS idx_hotlist_plate ON hotlist(plate_text);
