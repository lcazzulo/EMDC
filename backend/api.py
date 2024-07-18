from flask import Flask, request
from flask_socketio import SocketIO, emit, join_room


app = Flask(__name__)
socketio = SocketIO(app, logger=False, engineio_logger=False, cors_allowed_origins="*")

@socketio.on('join')
def on_join(data):
    print('join room')
    join_room('EMDC')

@socketio.on('EVENTS')
def handle_events(data):
    print('EVENTS received')
    emit('EVENTS', data, to='EMDC')

if __name__ == '__main__':
    socketio.run(app,host='0.0.0.0')
