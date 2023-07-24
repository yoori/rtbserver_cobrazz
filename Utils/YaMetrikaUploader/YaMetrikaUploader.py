#!/usr/bin/python3

import binascii
import os
import re
import psycopg2
import requests
import requests.exceptions
import clickhouse_driver
import clickhouse_driver.errors
from collections import namedtuple
import secrets
import string
from datetime import datetime
from dateutil.relativedelta import relativedelta
from base64 import b64decode
from zlib import crc32
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context


class YaMetrikaRow:
    def __init__(self, item):
        self.item = item
        self.dimensions = item["dimensions"]
        self.metrics = item["metrics"]

    def get_dimension(self, index):
        r = self.dimensions[index]["name"]
        if r is None:
            r = ""
        return r


RequestKeyNonAggregated = namedtuple("RequestKeyNonAggregated", "time utm_source utm_content utm_term key_ext referer")
RequestKeyAggregated = namedtuple("RequestKeyAggregated", "time utm_source utm_content utm_term key_ext")


class RequestData:
    def __init__(self, visits, bounce, avg_time):
        self.visits = visits
        self.bounce = bounce
        self.avg_time = avg_time


SQL_SELECT_SNAPSHOT = """
SELECT time, utm_content, utm_term, key_ext, referer, visits, bounce, avg_time
FROM YaMetrikaUploaderSnapshot
WHERE ymref_id=%s AND utm_source=%s AND time >= %s::timestamp AND time < %s::timestamp;
"""


SQL_UPDATE_SNAPSHOT = """
UPDATE YaMetrikaUploaderSnapshot
SET visits=%s, bounce=%s, avg_time=%s
WHERE ymref_id=%s AND time=%s::timestamp AND utm_source=%s AND utm_content=%s AND utm_term=%s AND key_ext=%s AND referer=%s;
"""


SQL_INSERT_SNAPSHOT = """
INSERT INTO YaMetrikaUploaderSnapshot(ymref_id, time, utm_source, utm_content, utm_term, key_ext, referer, visits, bounce, avg_time)
VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
"""


SQL_SELECT_STAT = """
SELECT time, utm_content, utm_term, key_ext, visits, bounce, avg_time
FROM YandexMetrikaStats
WHERE ymref_id=%s AND utm_source=%s AND time >= %s::timestamp AND time < %s::timestamp;
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


class Application(Service):
    ADV_ACTION_PATTERN = re.compile("[a-zA-Z0-9_-]{22}[.][.]/[a-zA-Z0-9_-]{22}[.][.]")
    ACTION_REQUEST_ID_CHARS = string.ascii_lowercase + string.ascii_uppercase + string.digits + "_-"
    UTM_SOURCE_FILTERS = ['genius', 'pml', 'pharmatic']

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
        today = datetime.today()
        self.date1 = (today + relativedelta(days=-self.days)).strftime('%Y-%m-%d')
        self.date2 = today.strftime('%Y-%m-%d')
        self.date_begin = self.date1
        self.date_end = (today + relativedelta(days=1)).strftime('%Y-%m-%d')
        try:
            self.on_requests(ymref_id, token, metrica_id, action_to_ccg)
            self.on_geo_ip(token, metrica_id)
        except requests.exceptions.RequestException as e:
            self.print_(0, e)
        except RuntimeError as e:
            self.print_(0, e)

    def on_requests(self, ymref_id, token, metrica_id, action_to_ccg):
        self.print_(0, f"Requests metrica_id={metrica_id}")
        with Context(self, out_dir=self.post_click_orig_dir) as ctx_post_click_orig,\
                Context(self, out_dir=self.post_click_dir) as ctx_post_click,\
                Context(self, out_dir=self.adv_act_fname_dir) as ctx_adv_act:

            adv_act_fname_prefix = f"AdvertiserAction.{ctx_adv_act.fname_seed}.24."
            log_fname_orig = f"YandexOrigPostClick.{ctx_post_click_orig.fname_seed}.csv"
            log_fname_prefix = f"YandexPostClick.{ctx_post_click.fname_seed}.24."

            for utm_source in self.UTM_SOURCE_FILTERS:
                old_records = {}
                self.cursor.execute(SQL_SELECT_SNAPSHOT, (ymref_id, utm_source, self.date_begin, self.date_end))
                for time, utm_content, utm_term, key_ext, referer, visits, bounce, avg_time in self.cursor.fetchall():
                    self.verify_running()
                    key = RequestKeyNonAggregated(time, utm_source, utm_content, utm_term, key_ext, referer)
                    data = RequestData(visits, bounce, avg_time)
                    old_records[key] = data

                aggr_records = {}

                for ym_row in self.__ym_api(
                        metrica_id,
                        token,
                        dimensions="ym:s:dateTime,ym:s:<attribution>UTMContent,ym:s:<attribution>UTMTerm,ym:s:regionArea,ym:s:regionCity,ym:s:referer",
                        metrics="ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds",
                        sort="-ym:s:visits",
                        utm_source=utm_source):

                    key = RequestKeyNonAggregated(
                        time=datetime.strptime(ym_row.get_dimension(0), "%Y-%m-%d %H:%M:%S"),
                        utm_source=utm_source,
                        utm_content=ym_row.get_dimension(1),
                        utm_term=ym_row.get_dimension(2),
                        referer=ym_row.get_dimension(5),
                        key_ext="\t".join((ym_row.get_dimension(3), ym_row.get_dimension(4))))

                    data = RequestData(
                        visits=int(ym_row.metrics[0]),
                        bounce=int(ym_row.metrics[1] * ym_row.metrics[0] / 100.0),
                        avg_time=ym_row.metrics[2])

                    self.process_record(
                        SQL_INSERT_SNAPSHOT,
                        SQL_UPDATE_SNAPSHOT,
                        ymref_id,
                        old_records,
                        key,
                        data,
                        self.process_new_record_non_aggregated,
                        action_to_ccg,
                        ctx_adv_act,
                        adv_act_fname_prefix)

                    aggr_key = RequestKeyAggregated(*key[:-1])
                    try:
                        aggr_value = aggr_records[aggr_key]
                    except KeyError:
                        aggr_records[aggr_key] = data
                    else:
                        new_visits = aggr_value.visits + data.visits
                        aggr_value.bounce += data.bounce
                        aggr_value.avg_time =\
                            (aggr_value.avg_time * aggr_value.visits + data.avg_time * data.visits) / new_visits
                        aggr_value.visits = new_visits

                old_records.clear()
                self.cursor.execute(SQL_SELECT_STAT, (ymref_id, utm_source, self.date_begin, self.date_end))
                for time, utm_content, utm_term, key_ext, visits, bounce, avg_time in self.cursor.fetchall():
                    self.verify_running()
                    key = RequestKeyAggregated(time, utm_source, utm_content, utm_term, key_ext)
                    data = RequestData(visits, bounce, avg_time)
                    old_records[key] = data

                for key, data in aggr_records.items():
                    self.process_record(
                        SQL_INSERT_STAT,
                        SQL_UPDATE_STAT,
                        ymref_id,
                        old_records,
                        key,
                        data,
                        self.process_new_record_aggregated,
                        ctx_post_click_orig,
                        ctx_post_click,
                        log_fname_orig,
                        log_fname_prefix)

    def process_record(self, sql_insert, sql_update, ymref_id, old_records, key, data, process_new_record, *args):
        old_value = old_records.get(key)
        if old_value is None:
            self.cursor.execute(sql_insert, (ymref_id,) + key + (data.visits, data.bounce, data.avg_time))
            self.cursor.execute("COMMIT")
        elif data.visits > old_value.visits:
            self.cursor.execute(sql_update, (data.visits, data.bounce, data.avg_time, ymref_id) + key)
            self.cursor.execute("COMMIT")
        else:
            return
        process_new_record(key, data, *args)

    def process_new_record_non_aggregated(self, key, data, action_to_ccg, ctx_adv_act, adv_act_fname_prefix):
        if action_to_ccg and self.ADV_ACTION_PATTERN.fullmatch(key.utm_term) is not None:
            user_id, request_id = key.utm_term.split("/")
            chunk_number = crc32(b64decode(user_id[:-2] + "==", b"-_")) % 24
            log = ctx_adv_act.files.get_line_writer(
                key=chunk_number,
                name=lambda: adv_act_fname_prefix + str(chunk_number))
            if log.first:
                log.write_line("AdvertiserAction\t3.6")
            t = key.time.strftime('%Y-%m-%d_%H:%M:%S')
            for action_id, ccg_ids in action_to_ccg.items():
                action_request_id = "".join(secrets.choice(self.ACTION_REQUEST_ID_CHARS) for _ in range(22)) + ".."
                log.write_line(f"{t}\t{user_id}\t@{request_id}\t@{action_id}\t-\t@{action_request_id}-@{ccg_ids}\t@{key.referer}\t-\t-\t0.0")

    def process_new_record_aggregated(self, key, data, ctx_post_click_orig, ctx_post_click, log_fname_orig, log_fname_prefix):
        t = key.time.strftime('%Y-%m-%d %H:%M:%S')

        log_orig = ctx_post_click_orig.files.get_line_writer(key=0, name=log_fname_orig)
        log_orig.write_line(
            f"{t}\t{key.utm_source}\t{key.utm_term}\t{data.visits}\t{data.bounce}\t{data.avg_time}")
        self.ch_client.execute(
            'INSERT INTO YandexOrigPostClick(time,utm_source,utm_term,visits,bounce,avg_time) VALUES',
            [[key.time, key.utm_source, key.utm_term, data.visits, data.bounce, data.avg_time]])

        try:
            user_id, request_id = key.utm_term.split(":")
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
                log = ctx_post_click.files.get_line_writer(
                    key=chunk_number,
                    name=lambda: log_fname_prefix + str(chunk_number))
                if log.first:
                    log.write_line("YandexPostClick\t1.0")
                log.write_line(f"{t}\t{user_id}\t{request_id}")
                self.ch_client.execute(
                    'INSERT INTO YandexPostClick(time,user_id,request_id) VALUES',
                    [[key.time, user_id, request_id]])

    def on_geo_ip(self, token, metrica_id):
        self.print_(0, f"GeoIP metrica_id={metrica_id}")
        with Context(self, out_dir=self.geo_ip_dir) as ctx_post_click:
            for utm_source in self.UTM_SOURCE_FILTERS:
                for ym_row in self.__ym_api(
                        metrica_id,
                        token,
                        dimensions="ym:s:ipAddress,ym:s:regionArea,ym:s:regionCity",
                        metrics="ym:s:visits",
                        sort="-ym:s:visits",
                        utm_source=utm_source):

                    ip_address = ym_row.get_dimension(0)
                    region_area = ym_row.get_dimension(1)
                    region_city = ym_row.get_dimension(2)

                    log = ctx_post_click.files.get_line_writer(
                        key=0,
                        name=lambda: f"YandexOrigGeo.{ctx_post_click.fname_seed}.csv")
                    log.write_line(f"{ip_address},{region_area}/{region_city}")
                    self.ch_client.execute(
                        'INSERT INTO YandexOrigGeo(ip_address,region_area,region_city) VALUES',
                        [[ip_address, region_area, region_city]])

    def __ym_api(self, metrica_id, token, dimensions, metrics, sort, utm_source):
        offset = 1
        while True:
            self.verify_running()
            api_params = {
                "ids": metrica_id,
                "metrics": metrics,
                "dimensions": dimensions,
                "date1": self.date1,
                "date2": self.date2,
                "sort": sort,
                "accuracy": "full",
                "filters": f"ym:s:<attribution>UTMSource=='{utm_source}'",
                "offset": offset,
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
                params=api_params,
                headers=header_params)
            if response.status_code != 200:
                raise RuntimeError(f"ymapi status code {response.status_code} != 200")
            response_data = response.json()['data']
            if not response_data:
                break
            for row in response_data:
                self.verify_running()
                yield YaMetrikaRow(row)
            offset += 100000


if __name__ == "__main__":
    service = Application()
    service.run()

