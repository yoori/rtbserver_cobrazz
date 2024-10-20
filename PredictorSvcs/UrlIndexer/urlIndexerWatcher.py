import os
import sys
#add external dependencies: logger_config.py and other
sys.path.append(os.path.abspath('../../Commons/Python/'))
from logger_config import get_logger

import time
import subprocess
import json
from datetime import datetime, timedelta
import clickhouse_connect
import redis
import argparse
import logging
import re
import signal

loggerCH = get_logger('URLindexier ClickHouse', logging.DEBUG)
loggerRedis = get_logger('URLindexier Redis', logging.DEBUG)
logger = get_logger('URLindexier', logging.DEBUG)

def signal_handler(sig, frame):
    print("Получен сигнал завершения, корректно завершаем операции...")
    # Здесь можно вызвать функции для отката изменений или завершения работы
    sys.exit(0)

# Регистрируем обработчики сигналов
signal.signal(signal.SIGTERM, signal_handler)
signal.signal(signal.SIGINT, signal_handler)

def parse_time_interval(time_str):
    """
    Парсит строку формата 'Xd Xh Xm Xs' и возвращает объект timedelta.
    """
    pattern = r"(?:(\d+)d)?\s*(?:(\d+)h)?\s*(?:(\d+)m)?\s*(?:(\d+)s)?"
    match = re.match(pattern, time_str)
    if not match:
        raise argparse.ArgumentTypeError("Неправильный формат времени. Ожидалось 'Xd Xh Xm Xs'")

    days, hours, minutes, seconds = match.groups(default='0')

    return timedelta(days=int(days), hours=int(hours), minutes=int(minutes), seconds=int(seconds))

# connect to dbs one time
clientClickHouse = clickhouse_connect.get_client(
    host='localhost',
    port=8123,
    username='default',
    password=''  # default password is empty
)
clientRedis = redis.Redis(host='localhost', port=6379, db=0)

def is_table_empty():
    """
    Check Clickhouse urls table is empty or not
    """
    initial_check_query = "SELECT COUNT(*) FROM urls"
    initial_result = clientClickHouse.query(initial_check_query)
    return initial_result.result_rows[0][0] == 0

def backup(urls):
    """
    Retrieves current data from ClickHouse and Redis at the specified URLs.
    Returns two dictionaries: data from ClickHouse and data from Redis.
    """
    clickhouse_backup = {}
    redis_backup = {}

    # ClickHouse
    urls_str = "', '".join(urls)
    query = f"SELECT url, indexed_date FROM urls WHERE url IN ('{urls_str}')"
    try:
        result = clientClickHouse.query(query)
        clickhouse_backup = {row[0]: row[1] for row in result.result_rows}
    except Exception as e:
        loggerCH.error(f"backup error: {e}")

    # Redis
    try:
        for url in urls:
            redis_backup[url] = clientRedis.get(url)
    except Exception as e:
        loggerRedis.error(f"backup error: {e}")
    return clickhouse_backup, redis_backup

def restore(clickhouse_backup, redis_backup, isRestoreRedis=True):
    """
    Restore data for ClickHouse and Redis.
    """
    # ClickHouse
    try:
        loggerCH.info(f"restore start")
        # Create Case
        update_case = "CASE "
        for url, indexed_date in clickhouse_backup.items():
            update_case += f"WHEN url = '{url}' THEN '{indexed_date}' "
        update_case += "END"

        # Create url list
        urls_str = "', '".join(clickhouse_backup.keys())

        query = f"ALTER TABLE urls UPDATE indexed_date = {update_case} WHERE url IN ('{urls_str}')"
        clientClickHouse.command(query)
        loggerCH.info(f"restore ok")
    except Exception as e:
        loggerCH.error(f"restore error: {e}")

    if(isRestoreRedis):
        # Redis
        loggerRedis.info(f"restore start")
        for url, value in redis_backup.items():
            try:
                clientRedis.set(url, value)
                loggerRedis.info(f"restore ok for {url}")
            except Exception as e:
                loggerRedis.error(f"restore for {url}, error: {e}")

def update(data_json_str):
    """
    Tries to update data in ClickHouse and Redis.
    In case of an error, it restores old data.
    """

    clickhouse_backup, redis_backup = backup(data_json_str)
    if not clickhouse_backup or not redis_backup:
        logger.error(f"Update error - no backup data.")
        return False

    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # ClickHouse
    urls_str = "', '".join(data_json_str.keys())
    update_query = f"ALTER TABLE urls UPDATE indexed_date = '{current_time}' WHERE url IN ('{urls_str}')"
    try:
        clientClickHouse.command(update_query)
        loggerCH.info(f"update ok.")
    except Exception as e:
        loggerCH.error(f"update error: {e}")
        restore(clickhouse_backup, redis_backup, isRestoreRedis=False)
        return False

    # Redis
    try:
        for url, values in data_json_str.items():
            values_json = json.dumps(values, ensure_ascii=False)
            clientRedis.set(url, values_json)
        loggerRedis.info(f"update ok")
    except Exception as e:
        loggerRedis.error(f"update error: {e}")
        restore(clickhouse_backup, redis_backup)
        return False
    return True

def insert(data_json_str):
    """
    Insert to ClickHouse и Redis.
    """
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # ClickHouse
    try:
        values = [f"('{url}', '{current_time}')" for url in data_json_str.keys()]
        insert_query = f"INSERT INTO urls (url, indexed_date) VALUES {', '.join(values)}"

        clientClickHouse.command(insert_query)
        loggerCH.error(f"insert ok")
    except Exception as e:
        loggerCH.error(f"insert error: {e}")
        return False

    # Redis
    try:
        for url, values in data_json_str.items():
            values_json = json.dumps(values, ensure_ascii=False)
            clientRedis.set(url, values_json)
        loggerRedis(f"insert ok")
    except Exception as e:
        loggerRedis(f"insert error: {e}")
        return False
    return True

def update_and_insert(urls, data_json_str):
    """
    Update and insert to ClickHouse и Redis.
    """

    existing_fields, new_fields = separateUrls(urls)

    if existing_fields:
        existing_data = {url: data_json_str[url] for url in existing_fields}
        if not update(existing_data):
            logger.error(f"update error")
            return False

    if new_fields:
        new_data = {url: data_json_str[url] for url in new_fields}
        if not insert(new_data):
            logger.error(f"insert error")
            return False
    return True

def separateUrls(urls):
    existing_urls = []
    new_urls = []

    urls_str = "', '".join(urls)
    check_query = f"SELECT url FROM urls WHERE url IN ({urls_str})"

    try:
        result = clientClickHouse.query(check_query)
        existing_urls = [row[0] for row in result.result_rows]

        # Convert the fields string to a list by removing spaces, quotes, and delimiters
        fields_list = [url.strip().strip("'") for url in urls.split(",")]

        # Defining new fields (which are missing in existing_fields)
        new_urls = [field for field in fields_list if field not in existing_urls]
    except Exception as e:
        loggerCH(f"error while checking existing fields: {e}")

    return existing_urls, new_urls

def askGPT(urls):
    """
    Runs a script with urls as an argument and outputs the contents of <output_GPT_dir>/<output_file>.json.
    """
    os.makedirs(output_GPT_dir, exist_ok=True)
    if(isGPTresulteStored):
        output_file = os.path.join(output_GPT_dir, datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + '.json')
    else:
        output_file = os.path.join(output_GPT_dir, 'GPTresult.json')

    # Run the script with the urls argument
    env = os.environ.copy()
    # to run getSiteCategories.py it is requared to have this two in env:
    #   env["YANDEX_GPT_API_KEY"] = os.getenv('YANDEX_GPT_API_KEY')
    #   env["YANDEX_ACCOUNT_ID"] = os.getenv('YANDEX_ACCOUNT_ID')
    result = subprocess.run(['python3', '../../Utils/GPT/getSiteCategories.py', '-w', urls, '-o', output_file], env=env)

    if result.returncode == 0:
        try:
            with open(output_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            logger.error(f"file {output_file} not found")
        except json.JSONDecodeError as e:
            logger.error(f"json.load({output_file}) error: {e}")
    else:
        logger.error(f"script ../../Utils/GPT/getSiteCategories.py return {result.stderr}")
    return None

def getExpiredUrls():
    threshold_date = (datetime.now() - timeOfExpiration).strftime('%Y-%m-%d %H:%M:%S')

    query = f"SELECT url FROM urls WHERE indexed_date <= '{threshold_date}'"
    result = clientClickHouse.query(query)

    urls = "', '".join(row[0] for row in result.result_rows)
    if(not urls):
        urls = None
    else:
        urls = f"'{urls}'"
    return urls

def checkExpiration():
    urls = getExpiredUrls()
    if urls != None:
        logger.info(f"Expired URLs: {urls}")
        data_json_str = askGPT(urls)
        if(data_json_str != None):
            ok = update(data_json_str)
            if(not ok):
                logger.error(f"checkExpiration update not ok")
def main():
    try:
        while True:
            if is_table_empty():
                logger.warn(f"Table is empty - ok, wait utill it will be filled")
            else:
                checkExpiration()

            logger.info(f"Sleep for {timeoutBetweenChecks_sec} sec")
            time.sleep(timeoutBetweenChecks_sec)
    finally:
        clientClickHouse.close()
        loggerCH.info(f"connection close")

        clientRedis.close()
        loggerRedis.info(f"connection close")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='URLs indexier watcher')
    parser.add_argument('--gptdir', type=str, default='GPTresults', help='GPT - getSiteCategory.py results folder')
    parser.add_argument('--storeGpt', action='store_true', help='true - save all gpt results, false - save only last one')
    parser.add_argument('--checkTimeout', type=int, default=5*60, help='timeout between checks')
    parser.add_argument('--expTime', type=parse_time_interval, default='60d', help="expiration time in time format 'Xd Xh Xm Xs'")

    args = parser.parse_args()

    global output_GPT_dir, isGPTresulteStored, timeoutBetweenChecks_sec, timeOfExpiration
    output_GPT_dir = args.gptdir
    isGPTresulteStored = args.storeGpt
    timeoutBetweenChecks_sec = args.checkTimeout
    timeOfExpiration = args.expTime

    logger.info(f"output_GPT_dir = {output_GPT_dir}")
    logger.info(f"isGPTresulteStored = {isGPTresulteStored}")
    logger.info(f"timeoutBetweenChecks_sec = {timeoutBetweenChecks_sec}")
    logger.info(f"timeOfExpiration = {timeOfExpiration}")
    logger.info(f"--------------------------")

    main()

    #todo: add logger, add args and make an readme, rewrite comments