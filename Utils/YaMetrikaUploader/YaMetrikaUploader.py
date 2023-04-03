#!/usr/bin/python3

import os
import psycopg2
import requests
from datetime import datetime
from dateutil.relativedelta import relativedelta
from base64 import b64decode
from zlib import crc32
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context


SQL_SELECT_STATS = """
SELECT time, utm_source, utm_content, utm_term, key_ext, visits, bounce, avg_time
FROM YandexMetrikaStats
WHERE ymref_id=%s AND time >= %s::timestamp AND time < %s::timestamp;
"""


SQL_UPDATE_STAT = """
UPDATE YandexMetrikaStats
SET visits=%s, bounce=%s, avg_time=%s
WHERE ymref_id=%s AND time=%s::timestamp AND utm_source=%s AND utm_content=%s AND utm_term=%s AND key_ext=%s;
"""


SQL_INSERT_STAT = """
INSERT INTO YandexMetrikaStats(ymref_id, time, utm_source, utm_content, utm_term, key_ext, visits, bounce, avg_time)
VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
"""


class ClickItem:
    def __init__(self, api_item):
        dimensions = api_item["dimensions"]

        def get_dimension(index):
            r = dimensions[index]["name"]
            if r is None:
                r = ""
            return r

        self.time = get_dimension(0)
        self.utm_source = get_dimension(1)
        self.utm_content = get_dimension(2)
        self.utm_term = get_dimension(3)
        self.region_area = get_dimension(4)
        self.region_city = get_dimension(5)
        self.key_ext = "\t".join((self.region_area, self.region_city))

        self.metrics = api_item["metrics"]
        self.visits = int(self.metrics[0])
        self.bounce = int(self.metrics[1] * self.visits / 100.0)
        self.avg_time = self.metrics[2]


class Application(Service):
    def __init__(self):
        super().__init__()

        self.args_parser.add_argument("--days", type=float, help="How many days to query from Yandex Metrika.")
        self.args_parser.add_argument("--pg-host", help="PostgreSQL hostname.")
        self.args_parser.add_argument("--pg-db", help="PostgreSQL DB name.")
        self.args_parser.add_argument("--pg-user", help="PostgreSQL user name.")
        self.args_parser.add_argument("--pg-pass", help="PostgreSQL password.")

    def on_start(self):
        super().on_start()

        ph_host = self.params["pg_host"]
        pg_db = self.params["pg_db"]
        pg_user = self.params["pg_user"]
        pg_pass = self.params["pg_pass"]
        self.connection = psycopg2.connect(
            f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
        self.cursor = self.connection.cursor()

        self.days = self.params["days"]

        self.post_click_orig_dir = os.path.join(self.out_dir, "YandexOrigPostClick")
        self.post_click_dir = os.path.join(self.out_dir, "YandexPostClick")
        self.geo_ip_dir = os.path.join(self.out_dir, "YandexOrigGeo")

    def on_run(self):
        self.cursor.execute("SELECT ymref_id, token, metrika_id FROM YandexMetrikaRef WHERE status = 'A';")
        for ymref_id, token, metrica_id in tuple(self.cursor.fetchall()):
            self.verify_running()
            self.on_metrica(ymref_id, token, metrica_id)

    def on_metrica(self, ymref_id, token, metrica_id):
        self.on_requests(ymref_id, token, metrica_id)
        self.on_geo_ip(token, metrica_id)

    def on_requests(self, ymref_id, token, metrica_id):
        with Context(self) as ctx:
            ym_result = self.__ym_api(
                metrica_id,
                token,
                dimensions="ym:s:dateTime,ym:s:<attribution>UTMSource,ym:s:<attribution>UTMContent,ym:s:<attribution>UTMTerm,ym:s:regionArea,ym:s:regionCity",
                metrics="ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds",
                sort="-ym:s:visits")
            if ym_result is None:
                return
            date_begin, date_end, data = ym_result

            old_rows = {}
            self.cursor.execute(SQL_SELECT_STATS, (ymref_id, date_begin, date_end))
            for time, utm_source, utm_content, utm_term, key_ext, visits, bounce, avg_time in self.cursor.fetchall():
                self.verify_running()
                old_rows[(str(time), utm_source, utm_content, utm_term, key_ext)] = (visits, bounce, float(avg_time))

            log_fname_orig = f"YandexOrigPostClick.{ctx.fname_seed}.csv"
            log_fname_prefix = f"YandexPostClick.{ctx.fname_seed}.24."
            metrica_printed = False
            for api_item in data:
                self.verify_running()
                item = ClickItem(api_item)
                already_processed = False
                row = old_rows.get((item.time, item.utm_source, item.utm_content, item.utm_term, item.key_ext))
                if row is None:
                    self.cursor.execute(
                        SQL_INSERT_STAT,
                        (ymref_id, item.time, item.utm_source, item.utm_content, item.utm_term, item.key_ext,
                         item.visits, item.bounce, item.avg_time))
                    self.cursor.execute("COMMIT")
                elif item.visits > row[0]:
                    self.cursor.execute(
                        SQL_UPDATE_STAT,
                        (item.visits, item.bounce, item.avg_time, ymref_id, item.time, item.utm_source, item.utm_content,
                         item.utm_term, item.key_ext))
                    self.cursor.execute("COMMIT")
                else:
                    already_processed = True

                if not already_processed:
                    if not metrica_printed:
                        metrica_printed = True
                        self.print_(1, "Metrica ID", metrica_id)
                    self.process_new_record(ctx, log_fname_orig, log_fname_prefix, item)

    def process_new_record(self, ctx, log_fname_orig, log_fname_prefix, item):
        log = ctx.files.get_line_writer(key=-1, name=log_fname_orig)
        log.write_line(
            f"{item.time}\t{item.utm_source}\t{item.utm_term}\t{item.visits}\t{item.bounce}\t{item.avg_time}")

        try:
            user_id, request_id = item.utm_term.split(":")
        except ValueError:
            pass
        else:
            chunk_number = crc32(b64decode(user_id + "==", b"-_")) % 24
            user_id += ".."
            request_id += ".."
            log = ctx.files.get_line_writer(
                key=chunk_number,
                name=lambda: log_fname_prefix + str(chunk_number))
            log.write_line(f"YandexPostClick\t1.0\n{item.time}\t{user_id}\t{request_id}")

    def on_geo_ip(self, token, metrica_id):
        with Context(self, out_dir=self.geo_ip_dir) as ctx:
            ym_result = self.__ym_api(
                metrica_id,
                token,
                dimensions="ym:s:ipAddress,ym:s:regionArea,ym:s:regionCity",
                metrics="ym:s:visits",
                sort="-ym:s:visits")
            if ym_result is None:
                return
            date_begin, date_end, data = ym_result

            with ctx.files.get_line_writer(self, f"YandexOrigGeo.{ctx.fname_seed}.csv") as f:
                for api_item in data:

                    dimensions = api_item["dimensions"]

                    def get_dimension(index):
                        r = dimensions[index]["name"]
                        if r is None:
                            r = ""
                        return r

                    ip_address = get_dimension(0)
                    region_area = get_dimension(1)
                    region_city = get_dimension(2)

                    f.write_line(f"{ip_address},{region_area}/{region_city}")

    def __ym_api(self, metrica_id, token, dimensions, metrics, sort):
        today = datetime.today()
        date1 = (today + relativedelta(days=-self.days)).strftime('%Y-%m-%d')
        date2 = today.strftime('%Y-%m-%d')
        date_end = (today + relativedelta(days=1)).strftime('%Y-%m-%d')
    
        data = []
        offset = 1

        while True:
            api_params = {
                "ids": metrica_id,
                "metrics": metrics,
                "dimensions": dimensions,
                "date1": date1,
                "date2": date2,
                "sort": sort,
                "accuracy": "full",
                "offset": offset,
                "limit": 100000
            }
            header_params = {
                'GET': '/stat/v1/data HTTP/1.1',
                'Host': 'api-metrika.yandex.net',
                'Authorization': 'OAuth ' + token,
                'Content-Type': 'application/x-yametrika+json'
            }
            try:
                response = requests.get(
                    "https://api-metrika.yandex.net/stat/v1/data",
                    params=api_params,
                    headers=header_params)
            except:
                return None
            if response.status_code != 200:
                return None
            response_data = response.json()['data']
            if not response_data:
                break
            data.extend(response_data)
            offset += 100000
        return date1, date_end, data


if __name__ == "__main__":
    service = Application()
    service.run()

