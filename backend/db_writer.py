import time
import pika
import mariadb
import json

db_conn = mariadb.connect(
        host="127.0.0.1",
        port=3306,
        user="emdc",
        password="emdc")
cur = db_conn.cursor()

def callback(ch, method, properties, body):
    print(" [x] %r:%r" % (method.routing_key, body))
    y = json.loads(body)
    cur.execute("INSERT INTO EMDC.samples (user_id, sample_ts, dc_id, rarr_flag, insert_ts) VALUES (?, ?, ?, ?, ?)",
        (0, y["ts"], y["dc_id"], y["rarr"], round(time.time() * 1000)));
    db_conn.commit()

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
