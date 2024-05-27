import pika

def callback(ch, method, properties, body):
    #pass
    print(" [x] %r:%r" % (method.routing_key, body))

def run_app():
    connection = pika.BlockingConnection(pika.ConnectionParameters(host='altair.luca-cazzulo.me'))
    channel = connection.channel()


    #result = channel.queue_declare(queue='queue-1', exclusive=False)
    #queue_name = result.method.queue
    queue_name = 'queue-1'
    #channel.queue_bind(exchange='EMDC', queue=queue_name, routing_key="A.B.C")

    print(' [*] Waiting for logs. To exit press CTRL+C')




    channel.basic_consume(queue=queue_name, on_message_callback=callback, auto_ack=True)

    channel.start_consuming()

if __name__ == '__main__':
    run_app()
