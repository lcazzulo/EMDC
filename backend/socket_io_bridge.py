import time
import pika
import json
import socketio

sio = socketio.Client()
connected = False

@sio.event
def connect():
    global connected
    print("I'm connected!")
    connected = True
    sio.emit('join', '')

@sio.event
def connect_error(data):
    print("The connection failed!")


@sio.event
def disconnect():
    global connected
    print("I'm disconnected!")
    connected = False


def callback(ch, method, properties, body):
    global connected
    print(" [x] %r:%r" % (method.routing_key, body))
    y = json.loads(body)
    if connected == True:
        sio.emit('EVENTS', y)


def run_app():
    global connected
    while not connected:
        try:
            sio.connect('http://localhost:5000')
            connected = True
        except:
            print("Error during connection to server")
            time.sleep(5)

    while(True):
        try:

            connection = pika.BlockingConnection(pika.ConnectionParameters(host='altair.luca-cazzulo.me'))
            channel = connection.channel()
            queue_name = 'queue_socket_io'

            print(' [*] Waiting for logs. To exit press CTRL+C')




            channel.basic_consume(queue=queue_name, on_message_callback=callback, auto_ack=True)
            try:
                channel.start_consuming()
            except KeyboardInterrupt:
                channel.stop_consuming()
                connection.close()
                break
        except pika.exceptions.ConnectionClosedByBroker:
            time.sleep(10)
            continue
        # Do not recover on channel errors
        except pika.exceptions.AMQPChannelError as err:
            print("Caught a channel error: {}, stopping...".format(err))
            break
        # Recover on all other connection errors
        except pika.exceptions.AMQPConnectionError:
            print("Connection was closed, retrying...")
            time.sleep(10)
            continue


if __name__ == '__main__':
    run_app()
