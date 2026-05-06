from sqlalchemy import Column, Integer, String, DateTime, ForeignKey, Float
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship
import datetime

Base = declarative_base()

class Agency(Base):
    __tablename__ = "agencies"
    id = Column(String, primary_key=True)
    name = Column(String, nullable=False)
    logo_url = Column(String)
    created_at = Column(DateTime, default=datetime.datetime.utcnow)

    vehicles = relationship("Vehicle", back_populates="agency")

class Vehicle(Base):
    __tablename__ = "vehicles"
    id = Column(String, primary_key=True)
    agency_id = Column(String, ForeignKey("agencies.id"))
    config_version = Column(Integer, default=1)
    status = Column(String, default="ACTIVE") # ACTIVE, DECOMMISSIONED, OFFLINE
    last_known_lat = Column(Float)
    last_known_lon = Column(Float)
    last_heartbeat = Column(DateTime, default=datetime.datetime.utcnow)

    agency = relationship("Agency", back_populates="vehicles")
    shifts = relationship("ShiftAudit", back_populates="vehicle")
    alerts = relationship("Alert", back_populates="vehicle")
    maintenance_logs = relationship("MaintenanceLog", back_populates="vehicle")

class ShiftAudit(Base):
    __tablename__ = "shift_audits"
    id = Column(Integer, primary_key=True, autoincrement=True)
    vehicle_id = Column(String, ForeignKey("vehicles.id"))
    officer_id = Column(String, nullable=False)
    disclaimer_accepted_at = Column(DateTime, nullable=False)
    shift_start = Column(DateTime, default=datetime.datetime.utcnow)
    shift_end = Column(DateTime)

    vehicle = relationship("Vehicle", back_populates="shifts")

class MaintenanceLog(Base):
    __tablename__ = "maintenance_logs"
    id = Column(Integer, primary_key=True, autoincrement=True)
    vehicle_id = Column(String, ForeignKey("vehicles.id"))
    tech_id = Column(String, nullable=False)
    action = Column(String, nullable=False) # e.g., "Jumper Clear", "Tier 2 Wipe"
    timestamp = Column(DateTime, default=datetime.datetime.utcnow)

    vehicle = relationship("Vehicle", back_populates="maintenance_logs")

class Alert(Base):
    __tablename__ = "alerts"
    id = Column(Integer, primary_key=True, autoincrement=True)
    vehicle_id = Column(String, ForeignKey("vehicles.id"))
    type = Column(String, nullable=False) # HEARTBEAT_TIMEOUT, THERMAL_CRITICAL
    severity = Column(String, default="CRITICAL")
    timestamp = Column(DateTime, default=datetime.datetime.utcnow)
    resolved = Column(Integer, default=0)

    vehicle = relationship("Vehicle", back_populates="alerts")
