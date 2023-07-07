#!/usr/bin/python3

import binascii
import os
import re
import psycopg2
import requests
import requests.exceptions
import clickhouse_driver
import clickhouse_driver.errors
import secrets
import string
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
        self.referer = get_dimension(6)
        self.key_ext = "\t".join((self.region_area, self.region_city))

        self.metrics = api_item["metrics"]
        self.visits = int(self.metrics[0])
        self.bounce = int(self.metrics[1] * self.visits / 100.0)
        self.avg_time = self.metrics[2]


class Application(Service):
    ADV_ACTION_PATTERN = re.compile("[a-zA-Z0-9_-]{22}[.][.]/[a-zA-Z0-9_-]{22}[.][.]")
    ACTION_REQUEST_ID_CHARS = string.ascii_lowercase + string.ascii_uppercase + string.digits + "_-"

    def __init__(self):
        super().__init__()

        self.args_parser.add_argument("--days", type=float, help="How many days to query from Yandex Metrika.")
        self.args_parser.add_argument("--pg-host", help="PostgreSQL hostname.")
        self.args_parser.add_argument("--pg-db", help="PostgreSQL DB name.")
        self.args_parser.add_argument("--pg-user", help="PostgreSQL user name.")
        self.args_parser.add_argument("--pg-pass", help="PostgreSQL password.")
        self.args_parser.add_argument("--ch-host", help="Clickhouse hostname.")

        self.connection = None
        self.cursor = None

        self.ch_client = None

    def on_start(self):
        super().on_start()

        self.days = self.params["days"]

        self.post_click_orig_dir = os.path.join(self.out_dir, "YandexOrigPostClick")
        self.post_click_dir = os.path.join(self.out_dir, "YandexPostClick")
        self.adv_act_fname_dir = os.path.join(self.out_dir, "AdvertiserAction")
        self.geo_ip_dir = os.path.join(self.out_dir, "YandexOrigGeo")

    def on_run(self):
        try:
            if self.connection is None:
                ph_host = self.params["pg_host"]
                pg_db = self.params["pg_db"]
                pg_user = self.params["pg_user"]
                pg_pass = self.params["pg_pass"]
                self.connection = psycopg2.connect(
                    f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
                self.cursor = self.connection.cursor()

            if self.ch_client is None:
                self.ch_client = clickhouse_driver.Client(host=self.params["ch_host"])

            self.cursor.execute("SELECT ymref_id,token,metrika_id FROM YandexMetrikaRef WHERE status = 'A';")
            for ymref_id, token, metrica_id in tuple(self.cursor.fetchall()):
                self.cursor.execute(f"SELECT action_id FROM yandexmetrikaaction WHERE ymref_id='{ymref_id}';")
                action_to_ccg = {}
                for (action_id,) in tuple(self.cursor.fetchall()):
                    ccg_ids = []
                    self.cursor.execute(f"SELECT ccg_id FROM CCGAction WHERE action_id='{action_id}';")
                    for (ccg_id,) in tuple(self.cursor.fetchall()):
                        ccg_ids.append(ccg_id)
                    if ccg_ids:
                        action_to_ccg[action_id] = ",".join(str(ccg_id) for ccg_id in ccg_ids)
                self.on_metrica(ymref_id, token, metrica_id, action_to_ccg)

        except psycopg2.Error as e:
            self.print_(0, e)
            if self.connection is not None:
                self.connection.close()
                self.connection = None
        except clickhouse_driver.errors.Error as e:
            self.print_(0, e)
            if self.ch_client is not None:
                self.ch_client.disconnect()
                self.ch_client = None


    def on_metrica(self, ymref_id, token, metrica_id, action_to_ccg):
        try:
            self.on_requests(ymref_id, token, metrica_id, action_to_ccg)
            self.on_geo_ip(token, metrica_id)
        except requests.exceptions.RequestException as e:
            self.print_(0, e)

    def on_requests(self, ymref_id, token, metrica_id, action_to_ccg):
        self.print_(0, f"Requests metrica_id={metrica_id}")
        with Context(self, out_dir=self.post_click_orig_dir) as ctx_orig,\
                Context(self, out_dir=self.post_click_dir) as ctx,\
                Context(self, out_dir=self.adv_act_fname_dir) as ctx_adv_act:
            ym_result = self.__ym_api(
                metrica_id,
                token,
                dimensions="ym:s:dateTime,ym:s:<attribution>UTMSource,ym:s:<attribution>UTMContent,ym:s:<attribution>UTMTerm,ym:s:regionArea,ym:s:regionCity,ym:s:referer",
                metrics="ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds",
                sort="-ym:s:visits")
            if ym_result is None:
                return
            date_begin, date_end, data = ym_result

            old_rows = {}
            self.cursor.execute(SQL_SELECT_STATS, (ymref_id, date_begin, date_end))
            for time, utm_source, utm_content, utm_term, key_ext, visits, bounce, avg_time in self.cursor.fetchall():
                self.verify_running()
                old_rows[(time.strftime('%Y-%m-%d %H:%M:%S'), utm_source, utm_content, utm_term, key_ext)] = (visits, bounce, float(avg_time))

            log_fname_orig = f"YandexOrigPostClick.{ctx_orig.fname_seed}.csv"
            log_fname_prefix = f"YandexPostClick.{ctx.fname_seed}.24."
            adv_act_fname_prefix = f"AdvertiserAction.{ctx.fname_seed}.24."
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
                    self.process_new_record(action_to_ccg, ctx_orig, ctx, ctx_adv_act, log_fname_orig, log_fname_prefix, adv_act_fname_prefix, item)

    def process_new_record(self, action_to_ccg, ctx_orig, ctx, ctx_adv_act, log_fname_orig, log_fname_prefix, adv_act_fname_prefix, item):
        t = datetime.strptime(item.time, '%Y-%m-%d %H:%M:%S')

        log_orig = ctx_orig.files.get_line_writer(key=0, name=log_fname_orig)
        log_orig.write_line(
            f"{item.time}\t{item.utm_source}\t{item.utm_term}\t{item.visits}\t{item.bounce}\t{item.avg_time}")
        self.ch_client.execute(
            'INSERT INTO YandexOrigPostClick(time,utm_source,utm_term,visits,bounce,avg_time) VALUES',
            [[t, item.utm_source, item.utm_term, item.visits, item.bounce, item.avg_time]])

        try:
            user_id, request_id = item.utm_term.split(":")
        except ValueError:
            pass
        else:
            try:
                chunk_number = crc32(b64decode(user_id + "==", b"-_")) % 24
            except binascii.Error:
                pass
            else:
                user_id += ".."
                request_id += ".."
                log = ctx.files.get_line_writer(
                    key=chunk_number,
                    name=lambda: log_fname_prefix + str(chunk_number))
                if log.first:
                    log.write_line("YandexPostClick\t1.0")
                log.write_line(f"{item.time}\t{user_id}\t{request_id}")
                self.ch_client.execute(
                    'INSERT INTO YandexPostClick(time,user_id,request_id) VALUES',
                    [[t, user_id, request_id]])

        if action_to_ccg and self.ADV_ACTION_PATTERN.fullmatch(item.utm_term) is not None:
            user_id, request_id = item.utm_term.split("/")
            chunk_number = crc32(b64decode(user_id[:-2] + "==", b"-_")) % 24
            log = ctx_adv_act.files.get_line_writer(
                key=chunk_number,
                name=lambda: adv_act_fname_prefix + str(chunk_number))
            if log.first:
                log.write_line("AdvertiserAction\t3.6")
            t = datetime.strptime(item.time, '%Y-%m-%d %H:%M:%S')
            for action_id, ccg_ids in action_to_ccg.items():
                action_request_id = "".join(secrets.choice(self.ACTION_REQUEST_ID_CHARS) for _ in range(22)) + ".."
                log.write_line(f"{t.strftime('%Y-%m-%d_%H:%M:%S')}\t{user_id}\t@{request_id}\t@{action_id}\t-\t@{action_request_id}-@{ccg_ids}\t@{item.referer}\t-\t-\t0.0")

    def on_geo_ip(self, token, metrica_id):
        self.print_(0, f"GeoIP metrica_id={metrica_id}")
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

            with ctx.files.get_line_writer(f"YandexOrigGeo.{ctx.fname_seed}.csv") as f:
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
                    self.ch_client.execute(
                        'INSERT INTO YandexOrigGeo(ip_address,region_area,region_city) VALUES',
                        [[ip_address, region_area, region_city]])

    def __ym_api(self, metrica_id, token, dimensions, metrics, sort):
        today = datetime.today()
        date1 = (today + relativedelta(days=-self.days)).strftime('%Y-%m-%d')
        date2 = today.strftime('%Y-%m-%d')
        date_end = (today + relativedelta(days=1)).strftime('%Y-%m-%d')

        data = []

        for utm_source_filter in ['genius', 'pml', 'pharmatic']:
            offset = 1
            while True:
                self.verify_running()
                api_params = {
                    "ids": metrica_id,
                    "metrics": metrics,
                    "dimensions": dimensions,
                    "date1": date1,
                    "date2": date2,
                    "sort": sort,
                    "accuracy": "full",
                    "filters": f"ym:s:<attribution>UTMSource=='{utm_source_filter}'",
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
                except requests.exceptions.RequestException:
                    self.print_(0, "ymapi RequestException")
                    return None
                if response.status_code != 200:
                    self.print_(0, f"ymapi status code {response.status_code} != 200")
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

