FROM python:3.11-slim

WORKDIR /app

ENV PYTHONUNBUFFERED=1

# Install dependencies
RUN pip install --no-cache-dir paho-mqtt
RUN mkdir -p /app/state

# Copy application code
COPY mqtt_writer_from_mqtt.py .

# Default command
CMD ["python", "mqtt_writer_from_mqtt.py"]
