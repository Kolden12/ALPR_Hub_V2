from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer
from pydantic import BaseModel
from typing import List, Optional
import datetime
import jwt

app = FastAPI(title="Sabre Command Center API")

# OAuth2 / JWT Configuration
SECRET_KEY = "SABRE_FLEET_MANAGEMENT_SECRET"
ALGORITHM = "HS256"

class Token(BaseModel):
    access_token: str
    token_type: str

class VehicleProfile(BaseModel):
    vehicle_id: str
    agency_id: str
    camera_ips: List[str]
    geofence_lat: float
    geofence_lon: float
    geofence_radius: int
    config_version: int

@app.post("/auth/provision", response_model=Token)
async def provision_vehicle(vehicle_id: str, ott: str):
    # OTT Validation logic (24h expiration)
    if ott == "VALID_OTT_TOKEN": # Placeholder for DB check
        token_data = {"sub": vehicle_id, "exp": datetime.datetime.utcnow() + datetime.timedelta(days=365)}
        token = jwt.encode(token_data, SECRET_KEY, algorithm=ALGORITHM)
        return {"access_token": token, "token_type": "bearer"}
    raise HTTPException(status_code=401, detail="Invalid OTT")

@app.get("/fleet/profile/{vehicle_id}", response_model=VehicleProfile)
async def get_vehicle_profile(vehicle_id: str, token: str = Depends(OAuth2PasswordBearer(tokenUrl="token"))):
    # Return Golden Config for specific vehicle
    return {
        "vehicle_id": vehicle_id,
        "agency_id": "SAPD-01",
        "camera_ips": ["192.168.1.101", "192.168.1.102"],
        "geofence_lat": 29.4241,
        "geofence_lon": -98.4936,
        "geofence_radius": 100,
        "config_version": 5
    }

@app.post("/telemetry/heartbeat")
async def receive_heartbeat(vehicle_id: str, lat: float, lon: float, temp: float, batt: float):
    # Log to Redis for real-time dashboard
    return {"status": "received", "update_available": False}
