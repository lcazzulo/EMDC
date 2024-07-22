from flask import Flask, request
from flask_socketio import SocketIO, emit, join_room
import json

app = Flask(__name__)
socketio = SocketIO(app, logger=False, engineio_logger=False, cors_allowed_origins="*")
last_ts_active = -1
last_ts_reactive = -1

@socketio.on('join')
def on_join(data):
    print('join room')
    join_room('EMDC')

@socketio.on('EVENTS')
def handle_events(data):
    global last_ts_active
    global last_ts_reactive
    power_active = -1
    power_reactive = -1
    curr_ts = data.get("ts")
    print('EVENTS received ' + str(curr_ts))
    if data.get("rarr") == 0:
        if last_ts_active != -1:
            power_active = 3600 / (curr_ts - last_ts_active)
        last_ts_active = curr_ts
    else:
        if last_ts_reactive != -1:
             power_reactive = 3600 / (curr_ts - last_ts_reactive)
        last_ts_reactive = curr_ts
    if power_active != -1:
        print('POWER ACTIVE   = ' + str (power_active))
        pa = {"ts": last_ts_active, "power": power_active}
        emit('POWER.ACTIVE', json.dumps(pa), to='EMDC')
    if power_reactive != -1:
        print('POWER REACTIVE = ' + str (power_reactive))
        pr = {"ts": last_ts_reactive, "power": power_reactive}
        emit('POWER.REACTIVE', json.dumps(pr), to='EMDC')
    emit('EVENTS', data, to='EMDC')

if __name__ == '__main__':
    socketio.run(app,host='0.0.0.0')
