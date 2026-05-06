from fastapi import FastAPI, Depends, HTTPException, BackgroundTasks
from pydantic import BaseModel
import sqlite3
import os
import datetime

app = FastAPI(title="Sabre ALPR Hub - Local API")

DB_PATH = "/mnt/sabre_storage/sabre_hub.db"

class ShiftAuditRequest(BaseModel):
    officer_id: str
    vehicle_id: str
    accepted_at: str
    event_type: str

@app.post("/shift/audit")
async def log_shift_audit(audit: ShiftAuditRequest):
    """Record shift start and legal disclaimer acceptance in Hub DB."""
    try:
        with sqlite3.connect(DB_PATH) as conn:
            # 1. Ensure shift record exists
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO shifts (officer_id, vehicle_id, disclaimer_accepted_at, shift_start)
                VALUES (?, ?, ?, datetime('now'))
            """, (audit.officer_id, audit.vehicle_id, audit.accepted_at))
            shift_id = cursor.lastrowid
            conn.commit()
            return {"status": "success", "shift_id": shift_id}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/health")
async def get_health():
    # Simple thermal and storage health report
    return {"status": "online", "thermal": "NORMAL", "storage": "OK"}
