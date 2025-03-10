import os
import time
import argparse
import logging
import json
import psycopg2
from psycopg2 import sql
from logger_config import get_logger

logger = get_logger('urlIndexWatcher', logging.DEBUG)

db_config = {
    "dbname": "",
    "user": "",
    "password": "",
    "host": "",
    "port": ""
}

connection = psycopg2.connect(**db_config)
channel_cache = dict()

def execute_query(query, params=(), fetch_one=False, fetch_all=False):
    """Executes a query with automatic resource management."""
    try:
        with connection.cursor() as cursor:
            cursor.execute(query, params)
            if fetch_one:
                return cursor.fetchone()
            if fetch_all:
                return cursor.fetchall()
            connection.commit()
    except Exception as e:
        logger.error(f"Database error: {e}")
        connection.rollback()
        return None

def initialize_cache(account_id, name_prefix):
    query = """
        SELECT channel_id, name FROM channel 
        WHERE account_id = %s AND name LIKE %s || '%'
    """
    rows = execute_query(query, (account_id, name_prefix), fetch_all=True)

    if not rows:
        return {}

    logger.debug("Initializing cache:")
    for ch_id, name in rows:
        logger.debug(f"{name} -> {ch_id}")
        channel_cache[name] = ch_id
    logger.info(f"Cache initialized with {len(channel_cache)} entries.")

def get_or_create_channels(account_id, categories):
    if categories:
        query = sql.SQL("""
            SELECT adserver.get_taxonomy_channel(%s, %s, 'RU', 'ru')
        """)

        result = {}
        for category in categories:
            channel_id = execute_query(query, (category, account_id), fetch_one=True)
            if channel_id:
                result[category] = channel_id[0]
            else:
                logger.error(f"Error fetching channel_id for {category}")
        return result
    return {}


def update_cache(account_id, categories):
    categories_to_add = get_channel_names_not_in_cache(categories)
    if categories_to_add:
        new_channels = get_or_create_channels(account_id, categories_to_add)
        channel_cache.update(new_channels)
        logger.debug(f"Updated cache: {new_channels}")

def saveCache(newChannels):
    if not newChannels:
        return

    logger.debug("Added to channels cache:")
    for name, channel_id in newChannels.items():
        logger.debug(f"{name} -> {channel_id}")
        channel_cache[name] = channel_id

def get_channel_names_not_in_cache(categories):
    categorysNotInCache = []
    for category in categories:
        if category not in channel_cache.keys():
            categorysNotInCache.append(category)
    return categorysNotInCache

def get_channel_ids(categorys):
    result = []
    for category in categorys:
        if category in channel_cache.keys():
            result.append(channel_cache[category])
        else:
            logger.error(f"Category {category} not found in cache")
    return result

def check_channel_triggers(url):
    query = """
        SELECT channel_id FROM channeltriggers WHERE original_trigger = %s;
    """
    rows = execute_query(query, (url,), fetch_all=True)
    existing_channel_ids = set(row[0] for row in rows) if rows else set()
    return existing_channel_ids

def get_to_del_and_add(existing_channel_Ids, channel_ids):
    to_add = []
    to_delete = []

    for ch_id in channel_ids:
        if ch_id not in existing_channel_Ids:
            to_add.append(ch_id)
        else:
            to_delete.append(ch_id)

    return to_delete, to_add

def getChannelIdsFromDB(channelNames, accountId):
    if not channelNames:
        return

    channelIds = []
    for channelName in channelNames:
        query = sql.SQL("""
            SELECT adserver.get_taxonomy_channel(%s, %s, 'RU', 'ru')
        """)
        try:
            cursor = connection.cursor()
            cursor.execute(query, (channelName, accountId))
            result = cursor.fetchone()
            channelIds.append(result[0])
        except Exception as e:
            logger.error(f"Error fetching channel_id: {e}")
            return None
    return channelIds

def add_triggers_if_not_exists(url):
    query = sql.SQL("""
            INSERT INTO public.triggers (
                trigger_type, normalized_trigger, qa_status, channel_type, country_code
            )
            SELECT 'U', %s,'A','A','RU'
            WHERE NOT EXISTS (
                SELECT * FROM triggers WHERE trigger_type = 'U' AND normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU'
            )
        """)
    execute_query(query, (url, url))

def get_trigger_id(url):
    query = sql.SQL("""
        SELECT trigger_id
        FROM triggers
        WHERE normalized_trigger = %s and trigger_type = 'U' and country_code = 'RU'and channel_type = 'A'
    """)
    try:
        cursor = connection.cursor()
        cursor.execute(query, (url,))
        result = cursor.fetchone()
        return result[0]
    except Exception as e:
        logger.error(f"Error fetching trigger_id: {e}")
        return None

def delete_channel_triggers(channel_Ids, url):
    if channel_Ids:
        query = sql.SQL("""
            DELETE FROM channeltrigger WHERE channel_id IN (%s)
            and original_trigger = %s
        """)
        execute_query(query, (channel_Ids,url,))

def add_channel_triggers(url, channel_ids, trigger_id):
    if channel_ids:
        query = """
            INSERT INTO channeltriggers (trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative)
            SELECT %s, %s, 'A', 'P', 'RU', %s, 'A', false
            WHERE NOT EXISTS (
                SELECT * FROM channeltriggers WHERE trigger_id = %s AND channel_id = %s AND channel_type = 'A' AND trigger_type = 'P' AND country_code = 'RU' AND original_trigger = %s
            );
        """
        for channel_id in channel_ids:
            execute_query(query, (trigger_id, channel_id, url, trigger_id, channel_id, url))

def process_urls_category(account_id, url, categories):
    logger.debug(f"Processing URL: {url}, Categories: {categories}")

    update_cache(account_id, categories)
    channel_ids = get_channel_ids(categories)

    add_triggers_if_not_exists(url)
    trigger_id = get_trigger_id(url)

    urls_channel_ids = check_channel_triggers(url)
    channelid_delete, channelid_add = get_to_del_and_add(urls_channel_ids, channel_ids)

    delete_channel_triggers(channelid_delete, url)
    add_channel_triggers(url, channelid_add, trigger_id)

def process_file(filePath, account_id, prefix):
    logger.info(f"Processing file: {filePath}")
    try:
        with open(filePath, "r", encoding="utf-8") as f:
            data = json.load(f)  # Load JSON content

        if not isinstance(data, dict):
            logger.error(f"Invalid JSON format {filePath}: Expected a dictionary.")
            return

        for url, categorys in data.items():
            process_urls_category(account_id, url, prefix + categorys)

    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON in {filePath}: {e}")
        return 2
    except Exception as e:
        logger.error(f"Unexpected error processing file {filePath}: {e}")
        return 3

def monitorFolder(folder_path, account_id, prefix, interval):
    processed_files = set()

    while True:
        try:
            files = set(os.listdir(folder_path))
            new_files = files - processed_files

            if not new_files:
                logger.info("No new files detected.")
                time.sleep(interval)
                continue

            logger.info(f"New files detected: {new_files}")

            for file_name in new_files:
                file_path = os.path.join(folder_path, file_name)
                if os.path.isfile(file_path):
                    if not process_file(file_path, account_id, prefix):
                        logger.warning(f"File {file_name} was not processed successfully. Retrying next time.")
                    else:
                        processed_files.add(file_name)
            time.sleep(interval)
        except KeyboardInterrupt:
            logger.warning("Monitoring stopped by user.")
            break
        except Exception as e:
            logger.error(f"Error in monitoring loop: {e}")
            time.sleep(interval)

def main():
    parser = argparse.ArgumentParser(description="Monitor a folder for new files.")
    parser.add_argument("--folder", default="../UrlIndexer/GPTresults/", help="Path to the folder to monitor")
    parser.add_argument("--account_id", required=True, help="Account ID for processing")
    parser.add_argument("--prefix", default="Taxonomy.ChatGPT.", help="Prefix for taxonomy")
    parser.add_argument("--interval", type=int, default=30, help="Interval in seconds")
    args = parser.parse_args()

    if not os.path.isdir(args.folder):
        logger.error(f"The folder '{args.folder}' does not exist.")
        return 1

    logger.info(f"Using Account ID: {args.account_id}")
    logger.info(f"Using Taxonomy Prefix: \"{args.prefix}\"")
    logger.info(f"Monitoring folder: {args.folder}, checking every {args.delta} seconds")
    logger.info("=================================")

    initialize_cache(args.account_id, args.prefix)

    monitorFolder(args.folder, args.account_id, args.prefix, args.interval)


if __name__ == "__main__":
    main()

