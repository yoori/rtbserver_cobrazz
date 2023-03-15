import os
import argparse
import json
import asyncio
import psycopg2
import signal
from time import sleep
import requests
from datetime import datetime
from dateutil.relativedelta import relativedelta


def get_sleep_subperiods(t):
    v = t / 0.1
    for i in range(int(v)):
        yield 0.1
    yield 0.1 * (v - int(v))


SQL_SELECT_STATS = """
SELECT time, utm_source, utm_content, utm_term, visits, bounce, avg_time
FROM YandexMetrikaStats
WHERE ymref_id=%s AND time >= %s::timestamp AND time < %s::timestamp;
"""


SQL_UPDATE_STAT = """
UPDATE YandexMetrikaStats
SET visits=%s, bounce=%s, avg_time=%s
WHERE ymref_id=%s AND time=%s::timestamp AND utm_source=%s AND utm_content=%s AND utm_term=%s;
"""


SQL_INSERT_STAT = "INSERT INTO YandexMetrikaStats VALUES (%s, %s, %s, %s, %s, %s, %s, %s)"


class Application:
    def __init__(self):
        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

        parser = argparse.ArgumentParser()
        parser.add_argument("-period", type=float, help="Period between checking files.")
        parser.add_argument("-days", type=float, help="How many days to query from Yandex Metrika.")
        parser.add_argument("-pg-host", help="PostgreSQL hostname.")
        parser.add_argument("-pg-db", help="PostgreSQL DB name.")
        parser.add_argument("-pg-user", help="PostgreSQL user name.")
        parser.add_argument("-pg-pass", help="PostgreSQL password.")
        parser.add_argument("-verbosity", type=int, help="Level of console information.")
        parser.add_argument("-print-line", type=int, help="Print line index despite verbosity.")
        parser.add_argument("--pid-file", help="File with process ID.")
        parser.add_argument("-config", default=None, help="Path to JSON config.")

        args = parser.parse_args()

        if args.config is None:
            config = {}
        else:
            with open(args.config, "r") as f:
                config = json.load(f)

        def get_param(name, *default):
            v = getattr(args, name)
            if v is not None:
                return v
            v = config.get(name)
            if v is not None:
                return v
            assert(0 <= len(default) <= 1)
            if len(default) == 0:
                raise RuntimeError(f"param '{name}' not found")
            return default[0]

        self.period = get_param("period")
        self.verbosity = get_param("verbosity", 1)
        ph_host = get_param("pg_host")
        pg_db = get_param("pg_db")
        pg_user = get_param("pg_user")
        pg_pass = get_param("pg_pass")
        self.connection = psycopg2.connect(
            f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
        self.cursor = self.connection.cursor()
        self.print_line = get_param("print_line", 0)
        self.line_index = 0
        self.pid_file = get_param("pid_file", None)
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))
        self.days = get_param("days")

    def __stop(self, signum, frame):
        print("Stop signal")
        self.running = False

    def run(self):
        try:
            self.on_run()
        finally:
            if self.pid_file is not None:
                os.remove(self.pid_file)

    def on_run(self):
        while True:
            self.on_period()
            for t in get_sleep_subperiods(self.period):
                if not self.running:
                    return
                sleep(t)

    def on_period(self):
        self.cursor.execute("SELECT ymref_id, token, metrika_id FROM YandexMetrikaRef WHERE status = 'A';")
        for ymref_id, token, metrica_id in tuple(self.cursor.fetchmany()):
            if not self.running:
                return
            self.on_metrica(ymref_id, token, metrica_id)

    def on_metrica(self, ymref_id, token, metrica_id):
        date1 = (datetime.today() + relativedelta(days=-self.days)).strftime('%Y-%m-%d')
        date2 = datetime.today().strftime('%Y-%m-%d')
        date_end = (datetime.today() + relativedelta(days=1)).strftime('%Y-%m-%d')

        self.cursor.execute(SQL_SELECT_STATS, (ymref_id, date1, date_end))
        old_rows = {}
        for time, utm_source, utm_content, utm_term, visits, bounce, avg_time in self.cursor.fetchall():
            if not self.running:
                return
            old_rows[(str(time), utm_source, utm_content, utm_term)] = (visits, bounce, float(avg_time))

        api_param = {
            "ids": metrica_id,
            "metrics": "ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds",
            "dimensions": "ym:s:dateTime,ym:s:<attribution>UTMSource,ym:s:<attribution>UTMContent,ym:s:<attribution>UTMTerm",
            "date1": date1,
            "date2": date2,
            "sort": "-ym:s:visits",
            "accuracy": "full",
            "limit": 100000
        }
        header_params = {
            'GET': '/stat/v1/data HTTP/1.1',
            'Host': 'api-metrika.yandex.net',
            'Authorization': 'OAuth ' + token,
            'Content-Type': 'application/x-yametrika+json'
        }
        response = requests.get(
            "https://api-metrika.yandex.net/stat/v1/data",
            params=api_param,
            headers=header_params
        )
        result = response.json()
        result = result['data']
        metrica_printed = False
        for item in result:
            if not self.running:
                return
            dimensions = item["dimensions"]
            time = dimensions[0]["name"]
            utm_source = dimensions[1]["name"]
            if utm_source is None:
                utm_source = ""
            utm_content = dimensions[2]["name"]
            if utm_content is None:
                utm_content = ""
            utm_term = dimensions[3]["name"]
            if utm_term is None:
                utm_term = ""

            metrics = item["metrics"]
            visits = int(metrics[0])
            bounce = int(metrics[1] * visits / 100.0)
            avg_time = metrics[2]

            already_processed = False
            row = old_rows.get((time, utm_source, utm_content, utm_term))

            if row is None:
                self.cursor.execute(
                    SQL_INSERT_STAT,
                    (ymref_id, time, utm_source, utm_term, visits, bounce, avg_time, utm_content))
                self.cursor.execute("COMMIT")
            elif visits > row[0]:
                self.cursor.execute(
                    SQL_UPDATE_STAT,
                    (visits, bounce, avg_time, ymref_id, time, utm_source, utm_content, utm_term))
                self.cursor.execute("COMMIT")
            else:
                already_processed = True

            if not already_processed:
                if self.verbosity >= 1 and not metrica_printed:
                    metrica_printed = True
                    print("Metrica ID", metrica_id)
                self.process_new_record(time, utm_source, utm_content, utm_term, visits, bounce, avg_time)

    def process_new_record(self, time, utm_source, utm_content, utm_term, visits, bounce, avg_time):
        print(",".join(str(i) for i in (time, utm_source, utm_content, utm_term, visits, bounce, avg_time)))


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

