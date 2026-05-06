from fastapi import FastAPI, Depends, HTTPException, status
from pydantic import BaseModel
from typing import List, Optional
import datetime
import jwt
import asyncio
from sqlalchemy.orm import Session
from . import models
from .db import SessionLocal

app = FastAPI(title="Sabre Command Center API")

# Background Heartbeat Monitor
async def heartbeat_monitor():
    while True:
        db = SessionLocal()
        try:
            threshold = datetime.datetime.utcnow() - datetime.timedelta(seconds=300)
            offline_vehicles = db.query(models.Vehicle).filter(
                models.Vehicle.last_heartbeat < threshold,
                models.Vehicle.status == "ACTIVE"
            ).all()

            for v in offline_vehicles:
                v.status = "OFFLINE"
                alert = models.Alert(vehicle_id=v.id, type="HEARTBEAT_TIMEOUT", severity="CRITICAL")
                db.add(alert)
            db.commit()
        finally:
            db.close()
        await asyncio.sleep(60)

@app.on_event("startup")
async def startup_event():
    asyncio.create_task(heartbeat_monitor())

@app.post("/telemetry/heartbeat")
async def receive_heartbeat(vehicle_id: str, lat: float, lon: float, db: Session = Depends(SessionLocal)):
    vehicle = db.query(models.Vehicle).filter(models.Vehicle.id == vehicle_id).first()
    if vehicle:
        vehicle.last_heartbeat = datetime.datetime.utcnow()
        vehicle.last_known_lat = lat
        vehicle.last_known_lon = lon
        vehicle.status = "ACTIVE"
        db.commit()
    return {"status": "received"}
