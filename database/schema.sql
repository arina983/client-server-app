-- Database Schema for Mobile Telemetry Project
-- Author: Arina
-- Description: Relational structure for storing GPS telemetry and neighbor cell radio metrics.

-- 1. Main table for location and serving cell data
CREATE TABLE IF NOT EXISTS device_data (
    id SERIAL PRIMARY KEY,
    latitude DOUBLE PRECISION,
    longitude DOUBLE PRECISION,
    altitude DOUBLE PRECISION,
    accuracy DOUBLE PRECISION,
    timestamp BIGINT,
    traffic_rx BIGINT,
    traffic_tx BIGINT,
    rsrp INTEGER,
    rsrq INTEGER,
    rssi INTEGER,
    sinr DOUBLE PRECISION
);

-- 2. Detail table for neighbor cells (1:N relationship)
CREATE TABLE IF NOT EXISTS cells_history (
    id SERIAL PRIMARY KEY,
    device_data_id INTEGER,
    pci INTEGER,
    rsrp INTEGER,
    rsrq INTEGER,
    rssi INTEGER,
    sinr DOUBLE PRECISION,
    
    -- Integrity: delete logs if the parent location record is removed
    CONSTRAINT fk_device_data
        FOREIGN KEY(device_data_id) 
        REFERENCES device_data(id) 
        ON DELETE CASCADE
);

-- Index for faster analytical queries
CREATE INDEX idx_cells_history_device_id ON cells_history(device_data_id);