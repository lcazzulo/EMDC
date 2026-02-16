import json
import os
import time
import paho.mqtt.client as mqtt

# -----------------------------
# CONFIGURATION
# -----------------------------
MQTT_BROKER = "mosquitto"
MQTT_PORT = 1883
MQTT_USER = "mqtt_user"
MQTT_PASSWORD = "emdc"

# Subscribe to EMDC events produced by datapublish
SUB_TOPIC = "emdc/events/energy/#"

STATE_FILE = "state/energy_state.json"
WH_PER_PULSE = 1
# -----------------------------

os.makedirs(os.path.dirname(STATE_FILE), exist_ok=True)

# Load previous cumulative state
if os.path.exists(STATE_FILE):
    with open(STATE_FILE, "r") as f:
        state = json.load(f)
        active_energy_wh = state.get("active_energy_wh", 0)
        reactive_energy_wh = state.get("reactive_energy_wh", 0)
        prev_ts_active = state.get("prev_ts_active", 0)
        prev_ts_reactive = state.get("prev_ts_reactive", 0)
else:
    active_energy_wh = 0
    reactive_energy_wh = 0
    prev_ts_active = 0
    prev_ts_reactive = 0


def save_state():
    """Persist cumulative energy values to JSON."""
    with open(STATE_FILE, "w") as f:
        json.dump({
            "active_energy_wh": active_energy_wh,
            "reactive_energy_wh": reactive_energy_wh,
            "prev_ts_active": prev_ts_active,
            "prev_ts_reactive": prev_ts_reactive
        }, f)


def publish_energy(client: mqtt.Client):
    """Publish the current cumulative values to MQTT with retain=True."""
    client.publish("home/energy/active_energy", active_energy_wh / 1000, retain=True)
    client.publish("home/energy/reactive_energy", reactive_energy_wh / 1000, retain=True)


def publish_power(client: mqtt.Client, instant_power_active_kw, instant_power_reactive_kw):
    """Publish instantaneous power if available."""
    if instant_power_active_kw is not None:
        client.publish("home/energy/active_power", instant_power_active_kw, retain=True)
    if instant_power_reactive_kw is not None:
        client.publish("home/energy/reactive_power", instant_power_reactive_kw, retain=True)


def classify_event(topic: str, payload_obj: dict) -> str | None:
    """
    Returns 'active' or 'reactive' or None.
    Priority:
      1) topic suffix (emdc/events/energy/active|reactive)
      2) payload field 'rarr' (0=active, else reactive)
    """
    t = topic.lower()
    if t.endswith("/active"):
        return "active"
    if t.endswith("/reactive"):
        return "reactive"

    rarr = payload_obj.get("rarr", None)
    if rarr is None:
        return None
    return "active" if int(rarr) == 0 else "reactive"


def get_ts_ms(payload_obj: dict) -> int:
    """
    Your old AMQP message used y["ts"].
    Keep that, but allow fallback keys if your C JSON differs.
    """
    for k in ("ts", "timestamp", "time", "t"):
        if k in payload_obj:
            return int(payload_obj[k])
    raise KeyError("No timestamp field found (expected 'ts' in ms)")


# MQTT callbacks
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[MQTT] Connected. Subscribing to: {SUB_TOPIC}")
        client.subscribe(SUB_TOPIC, qos=1)
    else:
        print(f"[MQTT] Connect failed rc={rc}")


def on_disconnect(client, userdata, rc, properties=None):
    # rc != 0 means unexpected disconnect; loop_forever will reconnect.
    print(f"[MQTT] Disconnected rc={rc}")


def on_message(client, userdata, msg):
    global active_energy_wh, reactive_energy_wh, prev_ts_active, prev_ts_reactive

    try:
        payload = msg.payload.decode("utf-8", errors="replace")
        y = json.loads(payload)

        event_kind = classify_event(msg.topic, y)
        if event_kind is None:
            print(f"Skipping unknown event type. topic={msg.topic}")
            return

        ts = get_ts_ms(y)

        instant_power_active_kw = None
        instant_power_reactive_kw = None

        if event_kind == "active":
            active_energy_wh += WH_PER_PULSE
            if prev_ts_active > 0:
                delta_t_sec = (ts - prev_ts_active) / 1000
                if delta_t_sec > 0:
                    instant_power_active_kw = (WH_PER_PULSE / 1000) / (delta_t_sec / 3600)
            prev_ts_active = ts
        else:
            reactive_energy_wh += WH_PER_PULSE
            if prev_ts_reactive > 0:
                delta_t_sec = (ts - prev_ts_reactive) / 1000
                if delta_t_sec > 0:
                    instant_power_reactive_kw = (WH_PER_PULSE / 1000) / (delta_t_sec / 3600)
            prev_ts_reactive = ts

        save_state()
        publish_energy(client)
        publish_power(client, instant_power_active_kw, instant_power_reactive_kw)

        print(f"Updated: active={active_energy_wh/1000:.3f} kWh, reactive={reactive_energy_wh/1000:.3f} kWh", end="")
        instant_parts = []
        if instant_power_active_kw is not None:
            instant_parts.append(f"active={instant_power_active_kw:.3f} kW")
        if instant_power_reactive_kw is not None:
            instant_parts.append(f"reactive={instant_power_reactive_kw:.3f} kW")
        if instant_parts:
            print(", instant power: " + ", ".join(instant_parts))
        else:
            print("")

    except Exception as e:
        print(f"Error processing message: {e}")


def main():
    client = mqtt.Client()

    client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect

    # Nice reconnect behavior (exponential-ish backoff)
    client.reconnect_delay_set(min_delay=1, max_delay=30)

    # Connect and block forever; reconnects automatically
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    print("[*] Waiting for MQTT messages. CTRL+C to exit.")
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("Exiting...")
        try:
            client.disconnect()
        except:
            pass


if __name__ == "__main__":
    main()
