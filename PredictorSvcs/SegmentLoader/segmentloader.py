import os
import argparse
import logging
import json
from clickhouse_driver import Client
from datetime import datetime, timedelta
import time
from psycopg2 import sql, connect

from logger_config import get_logger
import re
import subprocess


def create_connection_postgres():
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
    conn = connect(**db_config)
    conn.autocommit = False
    logger.info("Postgres connected.")
    return conn


def execute_query(query, params=(), fetch_one=False, fetch_all=False, commit=False):
    try:
        logger.debug(f"Executing query: {query} | Params: {params}")
        with connection.cursor() as cursor:
            cursor.execute("SET statement_timeout TO %s", (statement_timeout,))
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
        raise


def initialize_cache(account_id, name_prefix):
    query = sql.SQL("""SELECT channel_id, name FROM channel WHERE account_id = %s AND name LIKE %s || '%%' """)
    logger.debug(f"Initializing cache for account {account_id} with prefix {name_prefix}")
    rows = execute_query(query, (account_id, name_prefix,), fetch_all=True)

    channel_cache = dict()
    if not rows:
        return channel_cache
    logger.debug("Cache initialized with:")
    for ch_id, name in rows:
        logger.debug(f"{name} -> {ch_id}")
        channel_cache[name] = ch_id
    logger.info(f"Cache initialized with {len(rows)} entries.")
    return channel_cache


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


def url_time_insert(urls):
    now = datetime.now()
    for url in urls:
        logger.debug(f"Update time: {url} => {now}")

        query = """
                INSERT INTO gpt_update_time (url, last_updated)
                VALUES (%s, %s)
        """
        execute_query(query, (url, now,), commit=True)


def url_time_update(urls):
    now = datetime.now()
    for url in urls:
        logger.debug(f"Update time: {url} => {now}")

        query = """
            UPDATE gpt_update_time
            SET last_updated = %s
            WHERE url = %s;
        """
        execute_query(query, (url, now,), commit=True)


def get_unique_domains_from_postgre(urls, chunk_size=1000):
    if not urls:
        return set()

    urls_list = list(urls) if isinstance(urls, set) else urls

    existing_urls = set()
    total_urls = len(urls_list)
    processed = 0

    for i in range(0, total_urls, chunk_size):
        chunk = urls_list[i:i + chunk_size]
        placeholders = ",".join(["%s"] * len(chunk))
        query = f"""
            SELECT url
            FROM gpt_update_time
            WHERE url IN ({placeholders})
        """

        chunk_existing = execute_query(query, tuple(chunk), fetch_all=True)
        existing_urls.update([row[0] for row in chunk_existing])

        processed += len(chunk)
        logger.debug(f"Processed {processed}/{total_urls} URLs")

    logger.debug(f"existing domains: {existing_urls}")
    logger.info(f"Len existing domains: {len(existing_urls)}")
    return existing_urls


def process_urls_category(account_id, url, categories, prefix):
    logger.info(f"Processing {url} => {categories}")

    update_cache(account_id, categories)
    new_channel_ids = get_channel_ids(categories)

    existing_channelIds = get_channelids_for_url(url, prefix)
    channelid_delete, channelid_add = get_to_del_and_add(existing_channelIds, new_channel_ids)

    delete_channel_triggers(channelid_delete, url)
    if channelid_add:
        add_triggers_if_not_exists(url)
        trigger_id = get_trigger_id(url)
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
            data = json.load(f)

        if not isinstance(data, dict):
            logger.error(f"Invalid JSON format {filePath}: Expected a dictionary.")
            return 1

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


def load_domains_from_clickhouse(last_days):
    client = Client('click00')
    query = f"""
        SELECT domain
        FROM ccgsitereferrerstats
        WHERE adv_sdate > today() - {last_days}
        GROUP BY domain
        HAVING count() = 1
    """
    rows = client.execute(query)
    logger.debug(f"all domains from clickhouse {len(rows)}")

    # Filter and process domains
    unique_domains = set()
    for domain, in rows:
        if not domain.isdigit() and '.' in domain:
            unique_domains.add(extract_main_domain(domain))
    logger.info(f"load {len(unique_domains)} domains from clickhous for last {last_days} days")
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


def get_domains(checkDays, chunkSize):
    domains_from_ch = load_domains_from_clickhouse(checkDays)
    domains_presents_postgres = get_unique_domains_from_postgre(domains_from_ch)
    new_domains = domains_from_ch - domains_presents_postgres
    logger.info(f"domains to add : {len(new_domains)}")
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
    os.makedirs(output_GPT_dir, exist_ok=True)

    # Run the script with the urls argument
    # to run getSiteCategories.py it is requared to have this two in env:
    #   env["YANDEX_GPT_API_KEY"] = os.getenv('YANDEX_GPT_API_KEY')
    #   env["YANDEX_ACCOUNT_ID"] = os.getenv('YANDEX_ACCOUNT_ID')
    env = os.environ.copy()
    if isGPTresulteStored:
        result = subprocess.run(
            ['python3', scriptPathGPT, '-f', filename, '-d', output_GPT_dir, '-o', output_GPT_file, '--storeGpt', ],
            env=env)
    else:
        result = subprocess.run(
            ['python3', scriptPathGPT, '-f', filename, '-d', output_GPT_dir, '-o', output_GPT_file, '-l', logLevel ], env=env)

    if result.returncode == 0:
        logger.info(f"{filename} processed successfully.")
    else:
        logger.error(f"Error processing {filename}: {result.stderr}")


def write_websites_to_file(websites):
    os.makedirs(output_website_dir, exist_ok=True)
    data = {"websites": websites}
    output_file = os.path.join(output_website_dir, 'websites.json')
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)
    if (isWebsitesresulteStored):
        output_file_backup = os.path.join(output_website_dir,
                                          "websites_" + datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + '.json')
        with open(output_file_backup, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
    return output_file


def process_domains(domain_chunks, account_id, prefix):
    for i, domain_chunk in enumerate(domain_chunks):
        logger.info(f"Processing domain chunk {i + 1}/{len(domain_chunks)}")
        domain_filename = write_websites_to_file(domain_chunk)
        askGPT(domain_filename)
        process_file(domain_filename, account_id, prefix)


def get_urls_for_update(chunkSize):
    query = """
        SELECT url
        FROM gpt_update_time
        WHERE last_updated < ( CURRENT_DATE - INTERVAL '%s day')
        """
    domains = execute_query(query, (timeOfExpiration,), fetch_all=True)
    domains = [row[0] for row in domains]
    logger.info(f"domains to update : {len(domains)}")
    chunks = separate_to_chunks(domains, chunkSize)
    return chunks


def main():
    global logger
    global logLevel

    global connection
    global channel_cache
    global statement_timeout

    global output_GPT_dir
    global output_GPT_file
    global isGPTresulteStored
    global output_website_dir
    global isWebsitesresulteStored
    global timeoutBetweenChecks_sec
    global timeOfExpiration
    global scriptPathGPT

    parser = argparse.ArgumentParser(description="Monitor a folder for new files.")
    parser.add_argument("--loglevel", type=str, default="INFO", help="set log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)")
    parser.add_argument("--account_id", type=int, required=True, help="Account ID for processing")
    parser.add_argument("--prefix", default="Taxonomy.ChatGPT.", help="Prefix for taxonomy")
    parser.add_argument("--interval", type=parse_time_interval, default='60d',
                        help="Interval in seconds between bd checks in format 'Xd Xh Xm Xs'")
    parser.add_argument("--statement_timeout", type=int, default=5000, help="Statement timeout in milliseconds")
    parser.add_argument("--checkDays", type=int, default=3, help="Get domains that was added <checkDays> days ago")
    parser.add_argument("--chunkSize", type=int, default=1000, help="size of chunk for processing domains")
    parser.add_argument('--gptdir', type=str, default='GPTresults', help='GPT - getSiteCategory.py results folder')
    parser.add_argument('--gptFile', type=str, default='GPTresult.json', help='GPT - getSiteCategory.py results file')
    parser.add_argument('--storeGpt', action='store_true',
                        help='true - save all gpt results, false - save only last one')
    parser.add_argument('--websitesdir', type=str, default='websites', help='websites for gpt')
    parser.add_argument('--storeSites', action='store_false',
                        help='true - save all sites results, false - save only last one')
    parser.add_argument('--checkTimeout', type=int, default=300, help='timeout between checks')
    parser.add_argument('--expTimeDate', type=int, default='1', help="expiration time in days'")
    parser.add_argument('--pathGPT', type=str, default='../../Utils/GPT/getSiteCategories.py',
                        help='path to getSiteCategories.py')
    args = parser.parse_args()

    logger = get_logger('urlIndexWatcher', args.loglevel)

    logger.info("=================================")
    logger.info(f"Using log level: {args.loglevel}")
    logger.info(f"Using Account ID: {args.account_id}")
    logger.info(f"Using Taxonomy Prefix: \"{args.prefix}\"")
    logger.info(f"Checking every {args.interval} seconds")
    logger.info(f"Statement timeout: {args.statement_timeout} ms")
    logger.info(f"Check last days: {args.checkDays}")
    logger.info(f"Chunk size for processing domains: {args.chunkSize}")
    logger.info(f"GPT results directory: {args.gptdir}")
    logger.info(f"GPT results file: {args.gptFile}")
    logger.info(f"Store GPT results: {args.storeGpt}")
    logger.info(f"Websites directory: {args.websitesdir}")
    logger.info(f"Store websites results: {args.storeSites}")
    logger.info(f"Check timeout: {args.checkTimeout} seconds")
    logger.info(f"Expiration time: {args.expTimeDate}")
    logger.info(f"Path to getSiteCategories.py: {args.pathGPT}")
    logger.info("=================================")


    statement_timeout = args.statement_timeout
    connection = create_connection_postgres()
    channel_cache = initialize_cache(args.account_id, args.prefix)

    output_GPT_dir = args.gptdir
    output_GPT_file = args.gptFile
    isGPTresulteStored = args.storeGpt
    output_website_dir = args.websitesdir
    isWebsitesresulteStored = args.storeSites
    timeoutBetweenChecks_sec = args.checkTimeout
    timeOfExpiration = args.expTimeDate
    scriptPathGPT = args.pathGPT
    logLevel = args.loglevel.upper()

    while True:
        domain_chunks_new = get_domains(args.checkDays, args.chunkSize)
        process_domains(domain_chunks_new, args.account_id, args.prefix)

        domain_chunks_update = get_urls_for_update(args.chunkSize)
        process_domains(domain_chunks_update, args.account_id, args.prefix)

        time.sleep(args.interval.total_seconds())


if __name__ == "__main__":
    main()
