# EMDC Energy Monitoring System

This project collects energy pulses from a smart meter, publishes cumulative and instantaneous energy data to Home Assistant via MQTT, and stores state persistently.  

## Current Setup

### Components

1. **Edge device (Raspberry Pi / meter interface)**  
   - Publishes energy pulses to RabbitMQ (`EMDC` exchange).  
   - Each pulse represents **1 Wh**.  

2. **RabbitMQ Broker** (running on `sirius.luca-cazzulo.me`)  
   - Docker container with `3.13-management` image.  
   - Exchange: `EMDC` (topic exchange).  
   - Queues:  
     - `queue_db_writer` (for MariaDB storage, optional)  
     - `queue_mqtt_writer` (for MQTT publishing)  

3. **MQTT Broker (Mosquitto)** (running on `carina.luca-cazzulo.me`)  
   - Docker container: `eclipse-mosquitto:2.0`.  
   - MQTT credentials: `mqtt_user / emdc`.  
   - Port `1883` exposed for clients (Home Assistant, debugging, etc.).  

4. **Python Consumer (`emdc-mqtt-writer`)**  
   - Docker container running `mqtt_writer.py`.  
   - Consumes `queue_mqtt_writer` from RabbitMQ.  
   - Publishes energy data to MQTT topics:  

     | Topic | Value | Unit |
     |-------|-------|------|
     | `home/energy/active_energy` | cumulative active energy | kWh |
     | `home/energy/reactive_energy` | cumulative reactive energy | kWh |
     | `home/energy/active_power` | instantaneous active power | kW |
     | `home/energy/reactive_power` | instantaneous reactive power | kW |

   - Maintains **persistent state** in `state/energy_state.json` to survive restarts.  
   - Automatically calculates instantaneous power from pulse timing.  

5. **Home Assistant**  
   - Configured with MQTT integration pointing to Mosquitto broker.  
   - Sensors added for all four topics.  
   - Energy panel/dashboard displays cumulative and instantaneous energy.  


## Installation / Usage

### Docker Setup

1. **MQTT Broker** (on `carina`)  

```bash
cd ~/EMDC/backend
docker-compose up -d


├── EMDC/
│ ├── backend/
│ │ ├── mqtt_writer.py
│ │ ├── Dockerfile
│ │ ├── docker-compose.yml
│ │ ├── state/ # Persistent JSON state (ignored by Git)
│ │ └── .gitignore
└── mosquitto/
├── config/
├── data/
└── log/
```
