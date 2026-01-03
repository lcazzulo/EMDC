import json
import os
import time
import pika
import paho.mqtt.client as mqtt

# -----------------------------
# CONFIGURATION
# -----------------------------
MQTT_BROKER = "localhost"         # MQTT broker host (Home Assistant host)
MQTT_PORT = 1883                  # MQTT broker port
MQTT_USER = "mqtt_user"           # MQTT user
MQTT_PASSWORD = "emdc"   # MQTT password
STATE_FILE = "energy_state.json"  # persistent state file
WH_PER_PULSE = 1                  # Wh per pulse from your meter

RABBITMQ_HOST = "192.168.10.6"
RABBITMQ_USER = "emdc"
RABBITMQ_PASSWORD = "emdc"
RABBITMQ_QUEUE = "queue_mqtt_writer"
RABBITMQ_EXCHANGE = "EMDC"
# -----------------------------

# Load previous cumulative state
if os.path.exists(STATE_FILE):
    with open(STATE_FILE, "r") as f:
        state = json.load(f)
        active_energy_wh = state.get("active_energy_wh", 0)
        reactive_energy_wh = state.get("reactive_energy_wh", 0)
else:
    active_energy_wh = 0
    reactive_energy_wh = 0

# Initialize MQTT client
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

def save_state():
    """Persist cumulative energy values to JSON."""
    with open(STATE_FILE, "w") as f:
        json.dump({
            "active_energy_wh": active_energy_wh,
            "reactive_energy_wh": reactive_energy_wh
        }, f)

def publish_energy():
    """Publish the current cumulative values to MQTT with retain=True."""
    mqtt_client.publish("home/energy/active", active_energy_wh / 1000, retain=True)
    mqtt_client.publish("home/energy/reactive", reactive_energy_wh / 1000, retain=True)

def callback(ch, method, properties, body):
    """Handle messages from RabbitMQ."""
    global active_energy_wh, reactive_energy_wh

    try:
        y = json.loads(body)
        if y["rarr"] == 0:
            active_energy_wh += WH_PER_PULSE
        else:
            reactive_energy_wh += WH_PER_PULSE

        save_state()
        publish_energy()
        print(f"Updated: active={active_energy_wh} Wh, reactive={reactive_energy_wh} Wh")

    except Exception as e:
        print(f"Error processing message: {e}")

def run_consumer():
    """Main loop: connect to RabbitMQ and consume messages."""
    while True:
        try:
            credentials = pika.PlainCredentials(RABBITMQ_USER, RABBITMQ_PASSWORD)  # username, password
            connection = pika.BlockingConnection(
                  pika.ConnectionParameters(
                    host=RABBITMQ_HOST,
                    port=5672,
                    credentials=credentials
                   )
            )
            channel = connection.channel()

            # Ensure the queue exists
            channel.queue_declare(queue=RABBITMQ_QUEUE, durable=True)
            channel.queue_bind(exchange=RABBITMQ_EXCHANGE, queue=RABBITMQ_QUEUE, routing_key="EMDC.EVENTS.ACTIVE")
            channel.queue_bind(exchange=RABBITMQ_EXCHANGE, queue=RABBITMQ_QUEUE, routing_key="EMDC.EVENTS.REACTIVE")

            print(" [*] Waiting for messages. To exit press CTRL+C")
            channel.basic_consume(queue=RABBITMQ_QUEUE, on_message_callback=callback, auto_ack=True)
            channel.start_consuming()

        except pika.exceptions.AMQPConnectionError:
            print("Connection to RabbitMQ failed. Retrying in 10 seconds...")
            time.sleep(10)
        except KeyboardInterrupt:
            print("Exiting consumer...")
            try:
                mqtt_client.loop_stop()
                mqtt_client.disconnect()
            except:
                pass
            break

if __name__ == "__main__":
    run_consumer()
