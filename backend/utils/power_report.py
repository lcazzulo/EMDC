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
    query = "select sample_ts from samples where rarr_flag = 0 order by sample_ts desc limit 10;"
    cur.execute(query)
    for (sample_ts,) in cur:
        if (last_val == 0):
            last_val = sample_ts
        else:
            diff = last_val - sample_ts
            last_val = sample_ts
            print(round(3600 / diff, 3))


if __name__ == '__main__':
    run_app()
