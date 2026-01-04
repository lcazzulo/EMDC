FROM python:3.11-slim

WORKDIR /app

ENV PYTHONUNBUFFERED=1

# Install dependencies
RUN pip install --no-cache-dir pika paho-mqtt

# Copy application code
COPY mqtt_writer.py .

# Default command
CMD ["python", "mqtt_writer.py"]
