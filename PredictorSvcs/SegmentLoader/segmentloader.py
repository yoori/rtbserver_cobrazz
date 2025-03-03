import os
import sys
import time
import argparse
import logging
import json

import psycopg2
from psycopg2 import sql

# # add external dependencies: logger_config.py
# sys.path.append(os.path.abspath('../../PyCommons/'))
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
channelCache = dict()

def getСhIdsByAccAndPreFromDB(account_id, namePrefix):
    query = sql.SQL("""
    SELECT channel_id, name 
    FROM channel 
    WHERE account_id = %s;
    """)
    try:
        cursor = connection.cursor()
        cursor.execute(query, (int(account_id),))

        result = cursor.fetchall()

        filtered_channels = dict()
        for channel_id, name in result:
            if name.startswith(namePrefix):
                logger.debug(f"Channel: {name} -> {channel_id}")
                filtered_channels[name] = channel_id

        return filtered_channels
    except Exception as e:
        logger.error(f"Error fetching channels: {e}")
        return []

def getСhIdsByAccAndCatsFromDB(account_id, categorys):
    query = sql.SQL("""
    SELECT channel_id, name 
    FROM channel 
    WHERE account_id = %s;
    """)
    try:
        cursor = connection.cursor()
        cursor.execute(query, (int(account_id),))

        result = cursor.fetchall()

        filtered_channels = dict()
        for channel_id, name in result:
            if name in categorys:
                logger.debug(f"Channel: {name} -> {channel_id}")
                filtered_channels[name] = channel_id

        return filtered_channels
    except Exception as e:
        logger.error(f"Error fetching channels: {e}")
        return []

def getCacheFromDB(accountId, taxonomyPrefix):
    channelCache = getСhIdsByAccAndPreFromDB(accountId, taxonomyPrefix)
    if(len(channelCache) == 0):
        logger.warning("Channels cache is empty")
        return
    saveCache(channelCache)

def updateCacheFromDB(accountId, categorys):
    if not categorys:
        return

    newChannels = getСhIdsByAccAndCatsFromDB(accountId, categorys)
    saveCache(newChannels)

def saveCache(newChannels):
    logger.debug("Added to channels cache:")
    for name, channel_id in newChannels.items():
        logger.debug(f"{name} -> {channel_id}")
        channelCache[name] = channel_id

def deleteCache(key):
    logger.debug(f"Removed from cache: {key}")
    del channelCache[key]

def updateChannels(categorys, accountId):
    categorysNotInCache = []
    for category in categorys:
        if category not in channelCache.keys():
            categorysNotInCache.append(category)
    addChannelsToDB(accountId, categorysNotInCache)


def addChannelsToDB(accountId, categorys):
    if not categorys:
        return
    try:
        cursor = connection.cursor()
        for category in categorys:
            query_insert = sql.SQL("""
                INSERT INTO public.channel (
                    account_id, channel_type, status, flags, "name", "version", qa_status,
                    "expression", display_status_id, discover_query, discover_annotation,
                    country_code, parent_channel_id, newsgate_category_name, "language",
                    qa_user_id, qa_date, qa_description, behav_params_list_id, description,
                    visibility, channel_name_macro, keyword_trigger_macro, base_keyword,
                    superseded_by_channel_id, status_change_date, channel_rate_id, "namespace",
                    created_date, message_sent, freq_cap_id, geo_type, latitude, longitude,
                    city_list, triggers_version, channel_list_id, check_interval_num,
                    check_user_id, last_check_date, next_check_date, check_notes, trigger_type,
                    triggers_status, distinct_url_triggers_count, size_id, address, radius,
                    radius_units, last_updated, action_id
                )
                VALUES (
                    %s, 'A', 'A', 0, %s, '2015-06-30 17:03:22.328', 'A', NULL, 5, NULL, NULL, 'GB',
                    NULL, NULL, 'en', 156, '2008-10-09 12:13:59.000', NULL, NULL, NULL,
                    'PUB', NULL, NULL, NULL, NULL, '2015-06-30 16:57:15.000', NULL, 'A',
                    '2011-07-19 06:34:56.496', 0, NULL, NULL, NULL, NULL, NULL,
                    '2015-10-13 10:40:37.908', NULL, 1, NULL, NULL, '2013-05-07 16:33:35.000',
                    NULL, NULL, 'L', 125, NULL, NULL, NULL, NULL, '2015-10-13 10:40:37.908', NULL
                )
            """)
            cursor.execute(query_insert, (accountId,category,))
            logger.info(f"Added accountId and category: {accountId}, {category}")
        connection.commit()

        updateCacheFromDB(accountId, categorys)
    except Exception as e:
        logger.error(f"Error inserting categories: {e}")
        connection.rollback()  # Rollback in case of an error

def getChannelIdsFromCache(categorys):
    channelIds = []
    for category in categorys:
        value = channelCache.get(category)
        if value != None:
            channelIds.append(value)
    return channelIds

def getChTgIdsToDeleteAndAdd(url, channelIds):
    query = sql.SQL("""
        SELECT channel_trigger_id, channel_id
        FROM channeltriggers
        WHERE original_trigger = %s
    """)
    try:
        cursor = connection.cursor()
        cursor.execute(query, (url,))
        result = cursor.fetchall()

        toDelete = []
        toAdd = []
        for channel_trigger_id, channel_id in result:
            if channel_id not in channelIds:
                toDelete.append(channel_trigger_id)
            else:
                channelIds.remove(channel_id)
        toAdd = channelIds
    except Exception as e:
        logger.error(f"Error fetching channeltriggers: {e}")
    return toDelete, toAdd

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

def addTriggersIfNE(url):
    try:
        cursor = connection.cursor()
        query_insert = sql.SQL("""
            INSERT INTO public.triggers (
                trigger_type,normalized_trigger,qa_status,channel_type,country_code
            )
            SELECT 'U', %s,'A','A','RU'
            WHERE NOT EXISTS (
                SELECT * FROM triggers WHERE trigger_type = 'U' AND normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU'
            )
        """)
        cursor.execute(query_insert, (url, url))
        connection.commit()
        logger.info(f"Added new trigger for: {url}")
    except Exception as e:
        logger.error(f"Error inserting new trigger: {e}")
        connection.rollback()  # Rollback in case of an error

def getTriggerIdFromDB(url):
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

def deleteChTgByIdFromDB(chTgIdToDelete):
    if not chTgIdToDelete:
        return

    query = sql.SQL("""
        DELETE FROM channeltriggers
        WHERE channel_trigger_id = %s
    """)
    try:
        cursor = connection.cursor()
        for chTgId in chTgIdToDelete:
            cursor.execute(query, (chTgId,))
            logger.info(f"Deleted channel_trigger_id: {chTgId}")
        connection.commit()
    except Exception as e:
            logger.error(f"Error deleting channeltriggers: {e}")
            connection.rollback()  # Rollback in case of an error

def addToChTgDBIfNE(url, chIdToAdd, triggerId):
    if not chIdToAdd:
        return

    query = sql.SQL("""
        INSERT INTO channeltriggers (
            trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative
        )
        SELECT %s, %s, 'A', 'U', 'RU', %s, 'A', false
        WHERE NOT EXISTS (
            SELECT * FROM channeltriggers
            WHERE trigger_id = %s AND channel_id = %s AND channel_type = 'A' AND trigger_type = 'U' AND country_code = 'RU' AND original_trigger = %s
        )
    """)
    try:
        cursor = connection.cursor()
        for chId in chIdToAdd:
            cursor.execute(query, (triggerId, chId, url, triggerId, chId, url))
            logger.info(f"Added channel_trigger: {chId} -> {url}")
        connection.commit()
    except Exception as e:
        logger.error(f"Error inserting channeltriggers: {e}")
        connection.rollback()

def processUrlsCategory(accountId, url, categorys, prefix):
    logger.debug(f"URL: {url}, Categories: {categorys}")
    try:
        addTriggersIfNE(url)
        triggerId = getTriggerIdFromDB(url)

        updateChannels(prefix + categorys, accountId)
        channelIds = getChannelIdsFromCache(categorys)

        chTgIdToDelete, chIdToAdd = getChTgIdsToDeleteAndAdd(url, channelIds)
        deleteChTgByIdFromDB(chTgIdToDelete)
        addToChTgDBIfNE(url, chIdToAdd, triggerId)

        # определяем есть этот сайт или нет - есть ли в triggers, получаем trigger_id
        # если нет
        # сценарий 1 добавление(новый вебсайт)

        # если есть
        # определяем какие категории изменились - смотрим channeltrigers - получаем channeltrigers_id для всех original_triger = URL
        # в полученных channeltrigers_id смотрим channel_id и сравниваем с channel_id из переменной channelIds

        # если в channelIds есть теже элементы и еще больше - тогда добавляем недостоющие:
        # сценари 2 обновление - добавление(старый вебсайт - записи теже и новые(было 4(не изминились) но сейчас 7)

        # если в channelIds не все элементы сходятся - тогда удаляем те что отличаются и добавляем недостоющие:
        # сценарий 3 обновление - удаление(старый вебсайт - записи теже но некоторые новые(было 4 и сейчас 4(2 остались 2 поменялись))

        # если в channelIds нет ни одного повторяющегося элемента - тогда удаляем все и добавляем новые:
        # сцeнарий 4 обновление - удаление (старый вебсайт - новые все записи(было 4 - теперь 4(но совсем другие))

        # по сути просто смотрим на элементы, добавляем недостоющие и удаляем те что лишнии
        # - те что должны быть это то что в переменной channelIds

    cursor = connection.cursor()
        for category in categorys:
            # Check if record exists

            cursor.execute(query_check, (url, category))
            exists = cursor.fetchone()[0]

            if not exists:
                # Insert new record
                query_insert = sql.SQL("""
                    INSERT INTO url_categories (url, category)
                    VALUES (%s, %s)
                """)
                cursor.execute(query_insert, (url, category))
                logger.info(f"Added: {url} -> {category}")
            else:
                # Delete existing record
                query_delete = sql.SQL("""
                    DELETE FROM url_categories
                    WHERE url = %s AND category = %s
                """)
                cursor.execute(query_delete, (url, category))
                logger.info(f"Removed: {url} -> {category}")

        connection.commit()

    except Exception as e:
        logger.error(f"Error processing {url}: {e}")
        connection.rollback()  # Rollback in case of an error


def processFile(filePath, accountId, prefix):
    logger.info(f"Processing file: {filePath}")
    try:
        with open(filePath, "r", encoding="utf-8") as f:
            data = json.load(f)  # Load JSON content

        if not isinstance(data, dict):
            logger.error(f"Invalid JSON format in {filePath}: Expected a dictionary.")
            return

        for url, categorys in data.items():
            processUrlsCategory(accountId, url, categorys, prefix)

    except json.JSONDecodeError as e:
        logger.error(f"Error decoding JSON in {filePath}: {e}")
    except Exception as e:
        logger.error(f"Unexpected error processing file {filePath}: {e}")

def monitorFolder(folder_path, accountId, taxonomyPrefix, delta):
    """Monitor the folder and process new files at intervals"""
    processed_files = set()

    while True:
        try:
            # Get the list of files in the folder
            files = set(os.listdir(folder_path))

            # Identify new files that haven't been processed
            new_files = files - processed_files

            if new_files:
                logger.info(f"New files detected: {new_files}")

            for file_name in new_files:
                file_path = os.path.join(folder_path, file_name)
                if os.path.isfile(file_path):  # Process only files, ignore directories
                    processFile(file_path, accountId, taxonomyPrefix)

            # Update the set of processed files
            # todo check case of throwing error in file processing - in this case we need to try one more time
            processed_files.update(new_files)

            # Pause before the next scan
            time.sleep(delta)
        except KeyboardInterrupt:
            logger.warning("Monitoring stopped by user.")
            break
        except Exception as e:
            logger.error(f"Error in monitoring loop: {e}")
            time.sleep(delta)

def main():
    parser = argparse.ArgumentParser(description="Monitor a folder for new files.")
    parser.add_argument("--folder", default="../UrlIndexer/GPTresults/", help="Path to the folder to monitor (default: ../UrlIndexer/GPTresults/)")
    parser.add_argument("--account_id", required=True, help="Account ID for processing")
    parser.add_argument("--prefix", default="Taxonomy.ChatGPT.", help="Prefix for taxonomy (default: Taxonomy.ChatGPT.)")
    parser.add_argument("--delta", type=int, default=30, help="Processing interval in seconds (default: 30)")
    args = parser.parse_args()

    if not os.path.isdir(args.folder):
        logger.error(f"The folder '{args.folder}' does not exist.")
        return

    logger.info(f"Using Account ID: {args.account_id}")
    logger.info(f"Using Taxonomy Prefix: \"{args.prefix}\"")
    logger.info(f"Monitoring folder: {args.folder}, checking every {args.delta} seconds")
    logger.info("=================================")

    getCacheFromDB(args.account_id, args.prefix)

    monitorFolder(args.folder, args.account_id, args.prefix, args.delta)

if __name__ == "__main__":
    main()

