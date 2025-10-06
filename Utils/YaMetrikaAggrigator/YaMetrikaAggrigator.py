#!/usr/local/bin/python3
import os
import sys
import argparse
import logging
import colorlog
import re
import subprocess
import json
import signal
from typing import Iterable, Mapping, Any, List, Tuple

from clickhouse_driver import Client
from datetime import datetime, date, timedelta, timezone
import time
from psycopg2 import sql, connect
from typing import Any, Dict, List
from pathlib import Path
import requests

def parse_time_interval(time_str):
    """
    Parse string in format 'Xd Xh Xm Xs' and returns timedelta.
    """
    pattern = r"(?:(\d+)d)?\s*(?:(\d+)h)?\s*(?:(\d+)m)?\s*(?:(\d+)s)?"
    match = re.match(pattern, time_str)
    if not match:
        raise argparse.ArgumentTypeError("Wrong time format. Expected 'Xd Xh Xm Xs'")

    days, hours, minutes, seconds = match.groups(default='0')

    return timedelta(days=int(days), hours=int(hours), minutes=int(minutes), seconds=int(seconds))

def get_logger(name='main', level="INFO"):
    """
    Returns the configured logger with the specified name.
    """
    level = level.upper()
    if level == "DEBUG":
        logLevel = logging.DEBUG
    elif level == "INFO":
        logLevel=logging.INFO
    elif level == "WARNING":
        logLevel=logging.WARNING
    elif level == "ERROR":
        logLevel=logging.ERROR
    elif level == "CRITICAL":
        logLevel=logging.CRITICAL
    else:
        raise ValueError(f"Unknown log level: {level}")

    logger = logging.getLogger(name)
    logger.setLevel(logLevel)

    if not logger.handlers:
        console_handler = colorlog.StreamHandler()
        console_handler.setLevel(logging.DEBUG) # Set minimum level for console output(
        # meaning all messages will be shown higher than DEBUG)

        formatter = colorlog.ColoredFormatter(
            '%(log_color)s%(asctime)s.%(msecs)03d - %(name)s:%(lineno)d - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S',
            log_colors={
                'DEBUG': 'cyan',
                'INFO': 'green',
                'WARNING': 'yellow',
                'ERROR': 'red',
                'CRITICAL': 'bold_red',
            }
        )
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)

    return logger

def write_pid_file(pid_file_path):
    with open(pid_file_path, 'w') as f:
        f.write(str(os.getpid()))

def create_connection_postgres():
    db_config = {
        "dbname": config.data['db_name'],
        "user": config.data['db_user'],
        "password": config.data['db_password'],
        "host": config.data['db_host'],
        "port": config.data['db_port']
    }
    try:
        conn = connect(**db_config)
        conn.autocommit = False
        logger.info("Postgres connected.")
        return conn
    except Exception as e:
        logger.error(f"Error: {e}")
        sys.exit(1)

def create_connection_clickhouse():
    db_config = {
        "database": config.data['ch_name'],
        "user": config.data['ch_user'],
        "password": config.data['ch_password'],
        "host": config.data['ch_host'],
        "port": config.data['ch_port']
    }
    try:
        conn = Client(**db_config)
        conn.connection.connect()
        logger.info("Clickhouse connected.")
        return conn
    except Exception as e:
        logger.error(f"Error: {e}")
        sys.exit(1)
def execute_query(query, params=(), fetch_one=False, fetch_all=False, commit=False):
    try:
        logger.debug(f"Executing query: {query} | Params: {params}")
        with pg_connection.cursor() as cursor:
            cursor.execute("SET statement_timeout TO %s", (config.data['statement_timeout'],))
            cursor.execute(query, params)

            result = None

            if fetch_one and fetch_all:
                logger.warning("Cannot use both fetch_one and fetch_all simultaneously. fetch_all will be used.")

            if fetch_all:
                result = cursor.fetchall()
                logger.debug(f"Fetch all result: {result}")
            elif fetch_one:
                result = cursor.fetchone()
                logger.debug(f"Fetch one result: {result}")

            if commit:  # DELETE is committed immediately(have to not use commit=True)
                pg_connection.commit()
                logger.debug("Commit successful.")

            return result

    except Exception as e:
        logger.error(f"Database error: {e}")
        pg_connection.rollback()
        # raise


class Config:
    def __init__(self, config_path: str):
        self._load_config(Path(config_path))
        self._validate_required_fields()
        self.setDefaults()

    def _load_config(self, config_path) -> None:
        if not config_path.exists():
            raise FileNotFoundError(f"Config file not found: {config_path}")

        with open(config_path, "r", encoding="utf-8") as f:
            self.data: Dict[str, Any] = json.load(f)

        self.data['loglevel'] = self.data['loglevel'].upper()

    def _validate_required_fields(self) -> None:
        REQUIRED_FIELDS: List[str] = ["db_name", "db_user", "db_password", "db_host", "db_port", "ch_name", "ch_user",
                                      "ch_password", "ch_host", "ch_port"]
        missing_keys = [key for key in REQUIRED_FIELDS if key not in self.data]

        if missing_keys:
            print(f"Config error: Missing required fields: {', '.join(missing_keys)}", file=sys.stderr)
            exit(1)

    def setDefaults(self):
        defaults = {
            "pidFilePath": None,
            "loglevel": "INFO",
            "checkDaysSinceNow": 10,
            "statement_timeout": 5000
        }
        for key, value in defaults.items():
            self.data.setdefault(key, value)

    def print(self):
        msg = "Config:\n"
        msg += json.dumps(self.data, indent=4, ensure_ascii=False) + "\n"
        logger.info(msg)

def get_yandexMetrikaRef():
    query = sql.SQL( """
        SELECT ymref_id, token, metrika_id 
        FROM YandexMetrikaRef 
        WHERE status = 'A'
    """)
    return execute_query(query, fetch_all=True)

def fetch_metrika(token, counter_id, date, timeout=60, max_retries=3, sleep_seconds=1.0):
    url = "https://api-metrika.yandex.net/stat/v1/data"
    headers = {"Authorization": f"OAuth {token}"}
    params = {
        "ids": counter_id,
        "metrics": "ym:s:visits,ym:s:bounces,ym:s:avgVisitDurationSeconds",
        "dimensions": "ym:s:date,ym:s:hour,ym:s:minute,ym:s:UTMSource,ym:s:UTMContent,ym:s:UTMTerm,ym:s:referer",
        "date1": date,
        "date2": date,
        "limit": "100000",
        "sort": "ym:s:date,ym:s:hour,ym:s:minute",
    }
    for attempt in range(1, max_retries + 1):
        try:
            resp = requests.get(url, headers=headers, params=params, timeout=timeout)
            resp.raise_for_status()
            return resp.json()

        except requests.HTTPError as e:
            r = e.response
            status = r.status_code if r is not None else "unknown"
            body = (r.text[:300] if r is not None and r.text else "").replace("\n", " ")
            logger.warning(f"[{attempt}/{max_retries}] HTTP {status}: {body}")

        except requests.Timeout:
            logger.warning(f"[{attempt}/{max_retries}] timeout")

        except requests.RequestException as e:
            logger.warning(f"[{attempt}/{max_retries}] network error: {e}")

        except ValueError as e:
            logger.warning(f"[{attempt}/{max_retries}] invalid JSON: {e}")

        if attempt < max_retries and sleep_seconds > 0:
            time.sleep(sleep_seconds * (2 ** (attempt - 1)))
    return None

def build_yametrika_rows(
        records: Iterable[Mapping[str, Any]],
        ymref_id_int: int,
) -> List[Tuple]:
    result: List[Tuple] = []
    for r in records:
        demensions = r["dimensions"];
        metrics = r["metrics"]

        date   = str(demensions[0]["name"]).strip()            # date - '2025-09-02'
        hour   = str(demensions[1]["name"]).split(":", 1)[0]   # hour - '23:00' -> '23'
        minute = str(demensions[2]["name"]).strip()            # minutes - '40'
        time_str = f"{date} {hour.zfill(2)}:{minute.zfill(2)}" # '2025-09-02 23:40'
        time_dt = datetime.strptime(time_str, "%Y-%m-%d %H:%M").replace(
            tzinfo=timezone(timedelta(hours=4)) # Server time zone is UTC+4
        )
        utm_source  = (demensions[3].get("name") or "")
        utm_content = (demensions[4].get("name") or "")
        utm_term    = (demensions[5].get("name") or "")
        referer     = str(demensions[6]["name"])

        visits  = int(metrics[0])
        bounces = int(metrics[1])
        avg_sec = int(metrics[2])

        result.append((
            int(ymref_id_int),
            time_dt,
            str(utm_source),
            str(utm_content),
            str(utm_term),
            referer,
            visits,
            bounces,
            avg_sec,
        ))
    return result

def insert_yametrika_rows(rows: List[Tuple]) -> int:
    if not rows:
        return 0
    ch_connection.execute(
        "INSERT INTO YaMetrikaStats "
        "(ymref_id,time,utm_source,utm_content,utm_term,referer,visits,bounce,avg_time) VALUES",
        rows
    )
    return len(rows)

def main():
    global config
    global logger

    global pg_connection
    global ch_connection

    global channel_cache
    global last_checked_day

    parser = argparse.ArgumentParser(description="YaMetricaAggrigator")
    parser.add_argument("config_file", help="Path to JSON config file")
    args = parser.parse_args()

    config = Config(args.config_file)
    logger = get_logger('segmentLoader', config.data['loglevel'])
    config.print()

    current_directory = os.getcwd()
    logger.info(f"Current working directory: {current_directory}")
    if( config.data['pidFilePath'] is not None):
        pid_file = config.data['pidFilePath']
        logger.info(f"PID file path: {current_directory + '/' + pid_file}")
        write_pid_file(pid_file)
    else:
        logger.warning("PID file path is not set. PID file will not be created.")

    pg_connection = create_connection_postgres()
    ch_connection = create_connection_clickhouse()

    while True:
        ymrefs = get_yandexMetrikaRef()
        if not ymrefs:
            logger.warning("No active Yandex Metrika references found.")
            return

        today = date.today()
        for i in range(config.data['checkDaysSinceNow'] -1, -1, -1): # Days in order from past to present
            dayToGetData = (today - timedelta(days=i)).isoformat()
            for ymref in ymrefs:
                ymref_id = ymref[0]
                token = ymref[1]
                metrika_id = ymref[2]
                logger.info(f"Processing ymref_id: {ymref_id}, token: {token}, metrika_id: {metrika_id}, date: {dayToGetData}")

                metrica_data = fetch_metrika(token, metrika_id, dayToGetData)
                if metrica_data is None:
                    continue

                rows = build_yametrika_rows(metrica_data["data"], ymref_id)
                logger.debug(f"Built {len(rows)} rows for ymref_id {ymref_id}. Rows:")
                for row in rows:
                    logger.debug(row)
                inserted = insert_yametrika_rows(rows)
                logger.info(f"Inserted {inserted} rows for ymref_id {ymref_id} for {dayToGetData}")
        logger.info(f"Sleeping for {config.data['interval']}.")
        time.sleep(parse_time_interval(config.data['interval']).total_seconds())

if __name__ == "__main__":
    sys.exit(main())
