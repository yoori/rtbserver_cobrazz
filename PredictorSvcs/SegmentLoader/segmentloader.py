#!/usr/bin/env python

import os
import sys
import argparse
import logging
import json

from PIL.ExifTags import GPSTAGS
from clickhouse_driver import Client
from datetime import datetime, timedelta
import time
from psycopg2 import sql, connect
from typing import Any, Dict, List
from pathlib import Path

from pyparsing import GoToColumn

from logger_config import get_logger
import re
import subprocess


def create_connection_postgres():
    db_config = {
        "dbname": config.data['db_name'],
        "user": config.data['db_user'],
        "password": config.data['db_password'],
        "host": config.data['db_host'],
        "port": config.data['db_port']
    }
    conn = connect(**db_config)
    conn.autocommit = False
    logger.info("Postgres connected.")
    return conn


def execute_query(query, params=(), fetch_one=False, fetch_all=False, commit=False):
    try:
        logger.debug(f"Executing query: {query} | Params: {params}")
        with connection.cursor() as cursor:
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
                connection.commit()
                logger.debug("Commit successful.")

            return result

    except Exception as e:
        logger.error(f"Database error: {e}")
        connection.rollback()
        # raise


def get_channel_cache(account_id, name_prefix):
    logger.debug(f"Initializing cache for account {account_id} with prefix {name_prefix}")
    query = sql.SQL("""SELECT channel_id, name FROM channel WHERE account_id = %s AND name LIKE %s || '%%' """)
    rows = execute_query(query, (account_id, name_prefix,), fetch_all=True)

    channel_cache = dict()
    if not rows:
        return channel_cache
    logger.debug("channel_cache initialized with:")
    for ch_id, name in rows:
        logger.debug(f"{name} -> {ch_id}")
        channel_cache[name] = ch_id
    logger.info(f"Channel cache initialized with {len(rows)} entries.")
    return channel_cache

def get_urls_cache():
    query = sql.SQL("""SELECT url, last_updated FROM gpt_update_time""")
    rows = execute_query(query, (), fetch_all=True)

    cache = dict()
    if not rows:
        return cache
    logger.debug("urls_with_date_cache initialized with:")
    for url, date in rows:
        logger.debug(f"{url} -> {date}")
        cache[url] = date
    logger.info(f"Urls cache initialized with {len(rows)} entries.")
    return cache


def get_or_create_channels(account_id, categories):
    if categories:
        query = sql.SQL("""SELECT adserver.get_taxonomy_channel(%s, %s, 'RU', 'ru')""")

        result = {}
        for category in categories:
            logger.debug(f"Fetching channel_id for {category}")
            channel_id = execute_query(query, (category, account_id,), fetch_one=True, commit=True)
            if channel_id:
                upsert_channel_params(channel_id[0])
                result[category] = channel_id[0]
        return result
    return {}

def upsert_channel_params(channel_id):
    query = sql.SQL("""SELECT adserver.insert_or_update_taxonomy_channel_parameters(%s, 'U', 5184000, 1)""")
    execute_query(query, (channel_id,), commit=True)

def update_cache(account_id, categories):
    categories_to_add = get_channel_names_not_in_cache(categories)
    if categories_to_add:
        logger.debug(f"Categories to add to cache: {categories_to_add}")
        new_channels = get_or_create_channels(account_id, categories_to_add)
        channel_cache.update(new_channels)
        logger.debug(f"Updated cache: {new_channels}")


def get_channel_names_not_in_cache(categories):
    categorysNotInCache = set()
    for category in categories:
        if category not in channel_cache.keys():
            categorysNotInCache.add(category)
    return categorysNotInCache


def get_channel_ids(categories):
    result = set()
    for category in categories:
        if category in channel_cache.keys():
            result.add(channel_cache[category])
        else:
            logger.error(f"Category {category} not found in cache")
    return result


def get_channelids_for_url(url, prefix):
    query = """
    SELECT ct.channel_id FROM channeltrigger ct JOIN channel c ON ct.channel_id = c.channel_id
    WHERE ct.trigger_type = 'U' AND ct.original_trigger = %s AND c.name LIKE %s || '%%'
    """
    logger.debug(f"Checking channeltrigger for URL: {url}")
    rows = execute_query(query, (url, prefix,), fetch_all=True)
    existing_channel_ids = set(row[0] for row in rows) if rows else set()
    return existing_channel_ids


def get_to_del_and_add(existing_channelids, incoming_channelids):
    logger.debug(f"Existing channel IDs: {existing_channelids}")
    logger.debug(f"Incoming channel IDs: {incoming_channelids}")
    to_delete = existing_channelids - incoming_channelids
    to_add = incoming_channelids - existing_channelids
    logger.info(f"Channels to delete: {to_delete}")
    logger.info(f"Channels to add: {to_add}")

    return to_delete, to_add


def add_triggers_if_not_exists(url):
    query = sql.SQL("""
            INSERT INTO public.triggers (
                trigger_type, normalized_trigger, qa_status, channel_type, country_code
            )
            SELECT 'U', %s,'A','A','RU'
             WHERE NOT EXISTS (
                SELECT * FROM triggers
                 WHERE trigger_type = 'U'
                   AND normalized_trigger = %s
                   AND channel_type = 'A'
                   AND country_code = 'RU'
            )
        """)
    logger.debug(f"Adding trigger for URL: {url}")
    execute_query(query, (url, url,), commit=True)


def get_trigger_id(url):
    query = sql.SQL("""
        SELECT trigger_id
        FROM triggers
        WHERE normalized_trigger = %s and trigger_type = 'U' and country_code = 'RU'and channel_type = 'A'
    """)
    rows = execute_query(query, (url,), fetch_one=True)
    trigger_id = None
    if rows:
        trigger_id = rows[0]
        logger.info(f"Trigger ID: {trigger_id}")
    return trigger_id


def delete_channel_triggers(channel_Ids, url):
    if channel_Ids:
        logger.info(f"Deleting: {url} => {channel_Ids}")
        query = sql.SQL("""
            DELETE FROM channeltrigger WHERE channel_id in %s
            and original_trigger = %s
        """)
        execute_query(query, (tuple(channel_Ids), url,), commit=False)


def add_channel_triggers(url, channel_ids, trigger_id):
    if channel_ids:
        logger.info(f"Add: {url}({trigger_id}) => {channel_ids}")

        query = """
            INSERT INTO channeltrigger
            (trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative)
            SELECT %s, %s, 'A', 'U', 'RU', %s, 'A', false
            WHERE NOT EXISTS (
                SELECT * FROM channeltrigger
                 WHERE trigger_id = %s
                   AND channel_id = %s
                   AND channel_type = 'A'
                   AND trigger_type = 'P'
                   AND country_code = 'RU'
                   AND original_trigger = %s
            );
        """
        for channel_id in channel_ids:
            logger.debug(f"Adding {url}({trigger_id}) => {channel_id}")
            execute_query(query, (trigger_id, channel_id, url, trigger_id, channel_id, url,), commit=True)


def url_time_upsert(url):
    now = datetime.now()

    query = """
        WITH upsert AS (
            UPDATE gpt_update_time 
            SET last_updated = %s 
            WHERE url = %s
            RETURNING *
        )
        INSERT INTO gpt_update_time (url, last_updated)
        SELECT %s, %s
        WHERE NOT EXISTS (SELECT 1 FROM upsert);
    """
    execute_query(query, (now, url, url, now), commit=True)


def process_urls_category(account_id, url, categories, prefix, isUpdate=False):
    logger.info(f"Processing {url} => {categories}")

    update_cache(account_id, categories)
    new_channel_ids = get_channel_ids(categories)

    if(isUpdate):
        existing_channelIds = get_channelids_for_url(url, prefix)
        channelid_delete, channelid_add = get_to_del_and_add(existing_channelIds, new_channel_ids)
    else: # it is insert(no need to check presents of existing channel ids)
        channelid_delete = set()
        channelid_add = new_channel_ids

    delete_channel_triggers(channelid_delete, url)
    if channelid_add:
        add_triggers_if_not_exists(url)
        trigger_id = get_trigger_id(url)
        add_channel_triggers(url, channelid_add, trigger_id)


def remove_duplicates(categorys):
    return list(set(categorys))


def make_start_with_capital(categorys):
    formated_category = []
    for category in categorys:
        if category is not None:
            formated_category.append(category.capitalize())
    return formated_category


def add_prefix_to_categories(categories, prefix):
    category_with_prefix = set()
    for category in categories:
        if category is not None:
            category_with_prefix.add(prefix + category)
    return category_with_prefix

def preprocess_url(url):
    return remove_last_question_mark(url) # gpt sometimes return result with last question mark


def preprocess_categories(categories, prefix):
    if categories is None:
        categories = ['Неизвестная категория']
    return add_prefix_to_categories(remove_duplicates(make_start_with_capital(categories)), prefix)


def remove_last_question_mark(text):
    if text.endswith('?'):
        return text[:-1]
    return text


def process_file(filePath, account_id, prefix, isUpdate=False):
    if not os.path.exists(filePath):
        logger.error(f"File not found: {filePath}")
        return 4
    elif filePath is None:
        logger.error("File path is None")
        return 5

    logger.info(f"Processing file: {filePath}")
    try:
        with open(filePath, "r", encoding="utf-8") as f:
            data = json.load(f)

        if not isinstance(data, dict):
            logger.error(f"Invalid JSON format {filePath}: Expected a dictionary.")
            return 1
        for url, categories in data.items():
            url = preprocess_url(url)
            categories = preprocess_categories(categories, prefix)
            process_urls_category(account_id, url, categories, prefix, isUpdate=isUpdate)
            url_time_upsert(url)
        return 0

    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON in {filePath}: {e}")
        return 2
    except Exception as e:
        logger.error(f"Unexpected error processing file {filePath}: {e}")
        return 3


def extract_main_domain(domain):
    domain = domain.lower().replace('www.', '').replace('http://', '').replace('https://', '')
    parts = domain.split('.')

    # In case of IP-adress or single-component domain (localhost)
    if len(parts) <= 1 or domain.replace('.', '').isdigit():
        return domain

    # Special treatment for new gTLDs (as .game, .app, .dev, etc.)
    # If the TLD is short (2-3 characters), but is not a standard ccTLD
    if len(parts[-1]) > 2:  # Это новый gTLD (как .game, .app)
        return f"{parts[-2]}.{parts[-1]}" if len(parts) >= 2 else domain

    # For standard domains (com, net, org, etc.)
    return '.'.join(parts[-2:])


def load_domains_from_clickhouse():
    client = Client('click00')
    query = f"""
        SELECT domain
        FROM ccgsitereferrerstats
        WHERE adv_sdate = '{str(last_checked_day)}'
        GROUP BY domain
        HAVING count() = 1
    """
    rows = client.execute(query)

    # Filter and process domains
    unique_domains = set()
    for domain, in rows:
        if not domain.isdigit() and '.' in domain:
            unique_domains.add(extract_main_domain(domain))
    logger.info(f"load {len(rows)}/{len(unique_domains)}(all/unique) domains from clickhouse for {last_checked_day}")
    return unique_domains


def separate_to_chunks(domains, chunk_size):
    if not domains:
        return []

    domains_list = list(domains) if isinstance(domains, set) else domains
    chunks = [domains_list[i:i + chunk_size] for i in range(0, len(domains_list), chunk_size)]
    logger.debug(f"Separated into {len(chunks)} chunks of size <= {chunk_size}")
    for i, chunk in enumerate(chunks):
        logger.debug(f"Chunk {i + 1}: {len(chunk)}")
    return chunks


def get_last_checked_day():
    query = sql.SQL("""
        SELECT updated_at FROM gpt_processing_tracker
    """)
    rows = execute_query(query, (), fetch_one=True)
    if not rows:
        logger.error("No last checked day found in gpt_processing_tracker.")
        exit(6)

    last_checked_day = rows[0]
    logger.info(f"last_checked_day: {last_checked_day}")
    return last_checked_day


def update_last_checked_day(prev_date, cur_date):
    query = sql.SQL(
        """
            UPDATE gpt_processing_tracker 
            SET updated_at = %s 
            WHERE updated_at = %s
        """)
    execute_query(query, (cur_date, prev_date,), commit=True)


def get_domains(chunkSize=1000):
    domains_from_ch = load_domains_from_clickhouse()
    urls_with_date_cache = get_urls_cache()
    new_domains = domains_from_ch - set(urls_with_date_cache.keys())
    logger.info(f"add domains: {len(domains_from_ch)} - {len(domains_from_ch) - len(new_domains)} = {len(new_domains)}")
    chunks = separate_to_chunks(new_domains, chunkSize)
    return chunks


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


def askGPT(filename):
    output_dir = config.data['gptdir']
    output_file = config.data['gptFile']

    os.makedirs(output_dir, exist_ok=True)
    env = os.environ.copy()
    env["YANDEX_GPT_API_KEY"] = config.data['yandex_gpt_api_key']
    env["YANDEX_ACCOUNT_ID"] = config.data['yandex_account_id']
    command = [
        'python3', config.data['pathGPT'],
        '-f', filename,
        '-d', output_dir,
        '-o', output_file,
        '-l', config.data['loglevel'],
        '-a', str(config.data['attempts']),
        '-m', str(config.data['messagesize']),
        '-t', str(config.data['checkTimeout'])
    ]
    if config.data['storeGpt']:
        command.append('--storeGpt')

    result = subprocess.run(command, env=env)

    if result.returncode == 0:
        logger.info(f"{filename} processed successfully.")
        return os.path.join(output_dir, output_file)
    else:
        logger.error(f"Error processing {filename}: {result.stderr}")
        return None


def write_websites_to_file(websites):
    dir = config.data['websitesdir']
    os.makedirs(dir, exist_ok=True)
    data = {"websites": websites}
    output_file = os.path.join(dir, 'websites.json')
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)
    if (config.data['storeSites']):
        output_file_backup = os.path.join(dir,
                                          "websites_" + datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + '.json')
        with open(output_file_backup, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
    return output_file


def process_domains(domain_chunks, account_id, prefix, isUpdate=False):
    if not domain_chunks:
        return

    for i, domain_chunk in enumerate(domain_chunks):
        logger.info(f"Processing domain chunk {i + 1}/{len(domain_chunks)}")
        file_to_process = write_websites_to_file(domain_chunk)
        gpt_result_file_path = askGPT(file_to_process)
        process_file(gpt_result_file_path, account_id, prefix, isUpdate=isUpdate)


def get_urls_for_update(chunkSize):
    query = """
        SELECT url
        FROM gpt_update_time
        WHERE last_updated < ( CURRENT_DATE - INTERVAL '%s day')
        """
    domains = execute_query(query, (config.data['expTimeDate'],), fetch_all=True)
    domains = [row[0] for row in domains]
    logger.info(f"domains to update : {len(domains)}")
    chunks = separate_to_chunks(domains, chunkSize)
    return chunks

def increment_last_checked_day():
    global last_checked_day
    next_day = last_checked_day + timedelta(days=1)
    update_last_checked_day(last_checked_day, next_day)
    last_checked_day = next_day

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
        REQUIRED_FIELDS: List[str] = ["yandex_account_id", "yandex_gpt_api_key", "db_name", "db_user", "db_password",
                                      "db_host", "db_port", "account_id"]
        missing_keys = [key for key in REQUIRED_FIELDS if key not in self.data]

        if missing_keys:
            print(f"Config error: Missing required fields: {', '.join(missing_keys)}", file=sys.stderr)
            exit(1)

    def setDefaults(self):
        defaults = {
            "loglevel": "INFO",
            "prefix": "Taxonomy.ChatGPT.",
            "interval": "1d",
            "statement_timeout": 5000,
            "checkDays": 3,
            "chunkSize": 1000,
            "gptdir": "GPTresults",
            "gptFile": "GPTresults.json",
            "storeGpt": False,
            "websitesdir": "websites",
            "storeSites": True,
            "checkTimeout": 300,
            "expTimeDate": 60,
            "pathGPT": "../../Utils/GPT/getSiteCategories.py",
            "attempts": 3,
            "messagesize": 300
        }
        for key, value in defaults.items():
            self.data.setdefault(key, value)

    def print(self):
        msg = "Config:\n"
        msg += json.dumps(self.data, indent=4, ensure_ascii=False) + "\n"
        logger.info(msg)

def main():
    global config
    global logger

    global connection

    global channel_cache
    global last_checked_day

    parser = argparse.ArgumentParser(description="Segment loader")
    parser.add_argument("config_file", help="Path to JSON config file")
    args = parser.parse_args()

    config = Config(args.config_file)
    logger = get_logger('segmentLoader', config.data['loglevel'])
    config.print()

    connection = create_connection_postgres()

    channel_cache = get_channel_cache(config.data['account_id'], config.data['prefix'])
    last_checked_day = get_last_checked_day()

    while True:
        day_tobe_checked = datetime.now().date() - timedelta(days=config.data['checkDays'])
        logger.debug(f"{config.data['checkDays']} days from current date was {day_tobe_checked}")
        if last_checked_day < day_tobe_checked:
            logger.info("Need to update domains")
            domain_chunks_new = get_domains(config.data['chunkSize'])
            process_domains(domain_chunks_new, config.data['account_id'],  config.data['prefix'])
            increment_last_checked_day()
            continue

        logger.info("Not need to update domains - Check old domains for update")
        domain_chunks_update = get_urls_for_update(config.data['chunkSize'])
        process_domains(domain_chunks_update, config.data['account_id'], config.data['prefix'], isUpdate=True)
        logger.debug(f"sleep for {config.data['interval']} until the next check...")
        time.sleep(parse_time_interval(config.data['interval']).total_seconds())


if __name__ == "__main__":
    main()
