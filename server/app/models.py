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
    status = Column(String, default="ACTIVE") # ACTIVE, DECOMMISSIONED
    last_known_lat = Column(Float)
    last_known_lon = Column(Float)

    agency = relationship("Agency", back_populates="vehicles")
    shifts = relationship("ShiftAudit", back_populates="vehicle")

class ShiftAudit(Base):
    __tablename__ = "shift_audits"
    id = Column(Integer, primary_key=True, autoincrement=True)
    vehicle_id = Column(String, ForeignKey("vehicles.id"))
    officer_id = Column(String, nullable=False)
    disclaimer_accepted_at = Column(DateTime, nullable=False)
    start_time = Column(DateTime, default=datetime.datetime.utcnow)
    end_time = Column(DateTime)

    vehicle = relationship("Vehicle", back_populates="shifts")
