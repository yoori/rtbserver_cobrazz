import os
import time
import argparse
import logging
import json
import psycopg2
from psycopg2 import sql
from logger_config import get_logger

def create_connection():
    required_vars = ["DB_NAME", "DB_USER", "DB_PASSWORD", "DB_HOST", "DB_PORT"]
    missing_vars = [var for var in required_vars if not os.getenv(var)]
    if missing_vars:
        raise ValueError(f"Missing required environment variables: {', '.join(missing_vars)}")
    db_config = {
        "dbname": os.getenv("DB_NAME"),
        "user": os.getenv("DB_USER"),
        "password": os.getenv("DB_PASSWORD"),
        "host": os.getenv("DB_HOST"),
        "port": os.getenv("DB_PORT")
    }
    conn = psycopg2.connect(**db_config)
    conn.autocommit = False
    logger.info("Database configuration loaded successfully.")
    return conn

def execute_query(query, params=(), fetch_one=False, fetch_all=False, commit=False):
    """Executes a query with automatic resource management."""
    try:
        logger.debug(f"Executing query: {query} | Params: {params}")
        with connection.cursor() as cursor:
            cursor.execute("SET statement_timeout TO %s", (statement_timeout,))
            cursor.execute(query, params)

            result = None
            # if cursor.rowcount == 0:
            #     logger.debug("no rows found.")

            if fetch_one:
                result = cursor.fetchone()
                logger.debug(f"Fetch one result: {result}")

            if fetch_all:
                result = cursor.fetchall()
                logger.debug(f"Fetch all result: {result}")

            if commit: # DELETE is committed immediately
                connection.commit()
                logger.debug("Commit successful.")

            return result

    except Exception as e:
        logger.error(f"Database error: {e}")
        connection.rollback()
        raise

def initialize_cache(account_id, name_prefix):
    query = sql.SQL("""SELECT channel_id, name FROM channel WHERE account_id = %s AND name LIKE %s || '%%' """)
    logger.debug(f"Initializing cache for account {account_id} with prefix {name_prefix}")
    rows = execute_query(query, (account_id, name_prefix,), fetch_all=True)

    if not rows:
        return {}
    logger.debug("Cache initialized with:")
    for ch_id, name in rows:
        logger.debug(f"{name} -> {ch_id}")
        channel_cache[name] = ch_id
    logger.info(f"Cache initialized with {len(rows)} entries.")

def get_or_create_channels(account_id, categories):
    if categories:
        query = sql.SQL("""SELECT adserver.get_taxonomy_channel(%s, %s, 'RU', 'ru')""")

        result = {}
        for category in categories:
            logger.debug(f"Fetching channel_id for {category}")
            channel_id = execute_query(query, (category, account_id,), fetch_one=True, commit=True)
            if channel_id:
                result[category] = channel_id[0]
        return result
    return {}


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
    rows = execute_query(query, (url,prefix,), fetch_all=True)
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
                SELECT * FROM triggers WHERE trigger_type = 'U' AND normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU'
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
    rows = execute_query(query, (url, ), fetch_one=True)
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
        execute_query(query, (tuple(channel_Ids),url,), commit=False)

def add_channel_triggers(url, channel_ids, trigger_id):
    if channel_ids:
        logger.info(f"Add: {url}({trigger_id}) => {channel_ids}")

        query = """
            INSERT INTO channeltrigger (trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative)
            SELECT %s, %s, 'A', 'U', 'RU', %s, 'A', false
            WHERE NOT EXISTS (
                SELECT * FROM channeltrigger WHERE trigger_id = %s AND channel_id = %s AND channel_type = 'A' AND trigger_type = 'P' AND country_code = 'RU' AND original_trigger = %s
            );
        """
        for channel_id in channel_ids:
            logger.debug(f"Adding {url}({trigger_id}) => {channel_id}")
            execute_query(query, (trigger_id, channel_id, url, trigger_id, channel_id, url,), commit=True)
def process_urls_category(account_id, url, categories, prefix):
    logger.info(f"Processing {url} => {categories}")

    update_cache(account_id, categories)
    new_channel_ids = get_channel_ids(categories)

    add_triggers_if_not_exists(url)
    trigger_id = get_trigger_id(url)

    existing_channelIds = get_channelids_for_url(url, prefix)
    channelid_delete, channelid_add = get_to_del_and_add(existing_channelIds, new_channel_ids)

    delete_channel_triggers(channelid_delete, url)
    add_channel_triggers(url, channelid_add, trigger_id)

def remove_duplicates(channelIds):
    return list(set(channelIds))
def make_start_with_capital(channelIds):
    return [x.capitalize() for x in channelIds]
def add_prefix_to_categories(categories, prefix):
    category_with_prefix = set()
    for category in categories:
        category_with_prefix.add(prefix + category)
    return category_with_prefix

def preprocess_categories(categories, prefix):
    return add_prefix_to_categories(remove_duplicates(make_start_with_capital(categories)), prefix)

def process_file(filePath, account_id, prefix):
    logger.info(f"Processing file: {filePath}")
    try:
        with open(filePath, "r", encoding="utf-8") as f:
            data = json.load(f)  # Load JSON content

        if not isinstance(data, dict):
            logger.error(f"Invalid JSON format {filePath}: Expected a dictionary.")
            return

        for url, categories in data.items():
            categories = preprocess_categories(categories, prefix)
            process_urls_category(account_id, url, categories, prefix)

        return 0
    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON in {filePath}: {e}")
        return 2
    except Exception as e:
        logger.error(f"Unexpected error processing file {filePath}: {e}")
        return 3

def monitor_folder(folder_path, account_id, prefix, interval):
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
                        processed_files.add(file_name)
                        logger.info(f"{file_name} processed successfully.")
                    else:
                        logger.warning(f"{file_name} not processed successfully. Retrying next time.")

            time.sleep(interval)
        except KeyboardInterrupt:
            logger.warning("Monitoring stopped by user.")
            break
        except Exception as e:
            logger.error(f"Error in monitoring loop: {e}")
            time.sleep(interval)

def main():
    global logger
    global connection
    global channel_cache
    global statement_timeout

    logger = get_logger('urlIndexWatcher', level=logging.INFO)

    parser = argparse.ArgumentParser(description="Monitor a folder for new files.")
    parser.add_argument("--folder", default="../UrlIndexer/GPTresults/", help="Path to the folder to monitor")
    parser.add_argument("--account_id", type=int, required=True, help="Account ID for processing")
    parser.add_argument("--prefix", default="Taxonomy.ChatGPT.", help="Prefix for taxonomy")
    parser.add_argument("--interval", type=int, default=30, help="Interval in seconds")
    parser.add_argument("--statement_timeout", type=int, default=5000, help="Statement timeout in milliseconds")
    args = parser.parse_args()

    if not os.path.isdir(args.folder):
        logger.error(f"The folder '{args.folder}' does not exist.")
        return 1

    logger.info(f"Using Account ID: {args.account_id}")
    logger.info(f"Using Taxonomy Prefix: \"{args.prefix}\"")
    logger.info(f"Monitoring folder: {args.folder}, checking every {args.interval} seconds")
    logger.info(f"Statement timeout: {args.statement_timeout} ms")
    logger.info("=================================")

    statement_timeout = args.statement_timeout
    connection = create_connection()
    channel_cache = dict()

    initialize_cache(args.account_id, args.prefix)

    monitor_folder(args.folder, args.account_id, args.prefix, args.interval)


if __name__ == "__main__":
    main()

