import time
import mariadb



def run_app():
    last_val = 0
    db_conn = mariadb.connect(
        host="127.0.0.1",
        port=3306,
        user="emdc",
        password="emdc",
        database="EMDC")
    cur = db_conn.cursor()
    query = "select FROM_UNIXTIME(SAMPLE_TS/1000, '%Y-%m-%d %H:%i:%s') as TIMESTAMP, sample_ts from samples where rarr_flag = 0 order by sample_ts desc limit 41;"
    cur.execute(query)
    for (timestamp, sample_ts) in cur:
        if (last_val == 0):
            last_val = sample_ts
        else:
            diff = last_val - sample_ts
            last_val = sample_ts
            print(timestamp + " " + str(round(3600 / diff, 3)) + " kW")


if __name__ == '__main__':
    run_app()
