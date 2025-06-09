import os
import sys

#add external dependencies: for logger_config.py and other in this folder
sys.path.append(os.path.abspath('../../Commons/Python/'))
from logger_config import get_logger

import requests
import json
import time
from datetime import datetime
import argparse
import logging

logger = get_logger('GPT', logging.DEBUG)

def split_urls_into_chunks(url_string, max_chunk_size):
    # Separating the string by commas and removing spaces
    urls = [url.strip() for url in url_string.split(',')]
    chunks = []
    current_chunk = ""

    for url in urls:
        # Check how many characters will be in the subarray with the addition of the current URL
        if len(current_chunk) + len(url) + 2 <= max_chunk_size:  # +2 for ", "
            if current_chunk:
                current_chunk += ", "  #  add comma before next URL
            current_chunk += url
        else:
            # If the current subarray overflows, add it to the list and start a new one
            chunks.append(current_chunk)
            current_chunk = url

    # Add the last subarray if there is anything left
    if current_chunk:
        chunks.append(current_chunk)

    return chunks

def send_message(websites_str):
    # logger.info(f"get categories for websites:{websites}")
    prompt = {
        "modelUri": "gpt://{}/yandexgpt-lite".format(os.getenv('YANDEX_ACCOUNT_ID')),
        "completionOptions": {
            "stream": False,
            "temperature": 0.5,
            "maxTokens": "2000"
        },
        "messages": [
            {
                "role": "user",
                "text":"Ты выдаешь ответы в формате json сообщений - "
                       "{<запрошеный вебсайт>:[<его категории>]}"
                       "Не пишешь вступительных слов и не используешь конструкцию command and results."
                       "Каждая категория как отдельный элемент."
                       "Json должен быть валидным. Вариантs ответов {] и [] является не валидным."
                       f"{{ \"command\": \"какие категории у сайтов {websites_str}?\" }}"
            }
        ]
    }

    url = "https://llm.api.cloud.yandex.net/foundationModels/v1/completion"
    headers = {
        "Content-Type": "application/json",
        "Authorization": "Api-Key {}".format(os.getenv('YANDEX_GPT_API_KEY'))
    }

    response = requests.post(url, headers=headers, json=prompt)
    return response

def send_message_with_retry(message, retries_max, timeout_attempts_max, timeout_ms):
    """Function for sending a request with repeated attempts when receiving != 200 response codes"""
    for timeout_attempt in range(timeout_attempts_max):
        retries = 0
        while retries < retries_max:
            try:
                response = send_message(message)
                if response.status_code == 200:
                    message_json = response.json()
                    ## can be usefeull when we want to see information about used tokens in GPT
                    logger.debug(f"message_json: {message_json}")
                    message_result_str = message_json["result"]["alternatives"][0]["message"]["text"].strip('`').strip()
                    logger.info(f"message_result_str: {message_result_str}")
                    message_result_json = json.loads(message_result_str)
                    return message_result_json
                else:
                    logger.warning(f"Received response.status_code({response.status_code}) != 200. Retrying in {timeout_ms / 1000} seconds... (Attempt {timeout_attempt + 1}/{timeout_attempts_max})")
                    time.sleep(timeout_ms / 1000)
                    break

            except json.JSONDecodeError as json_error:
                logger.error(f"Received JSONDecodeError error: {json_error} for data: {message_result_str}")
                logger.warning(f"Retrying to ask the same data. (retries {retries+1}/{retries_max})");
                retries += 1
                if retries >= retries_max:
                    logger.error(f"The response could not be converted after {retries_max} attempts.")
            except Exception as e:
                logger.error(f"Exception in send_message_with_retry: {e}")

    logger.error(f"Failed to get response after {timeout_attempts_max} attempts")
    return {}

def merge_json(json1, json2):
    merged = json1.copy()
    for key, value in json2.items():
        if key in merged:
            if not isinstance(merged[key], list):
                merged[key] = [merged[key]]
            if not isinstance(value, list):
                value = [value]

            merged[key] = list(set(merged[key] + value))
        else:
            merged[key] = value
    return merged

def main(args):
    json_filename = args.json_file
    max_chunk_size = args.maxsize
    timeout_ms = args.timeout
    attempts_max = args.attempts
    retries_max = args.retries
    output_dir = args.output_dir
    store_gpt_results = args.storeGpt

    os.makedirs(output_dir, exist_ok=True)

    if not os.path.exists(json_filename):
        print(f"File '{json_filename}' not found.")
        return 1

    with open(json_filename, "r") as f:
        data = json.load(f)
    websites = ", ".join(data[list(data.keys())[0]])

    chunks = split_urls_into_chunks(websites, max_chunk_size)
    combined_results_json = {}

    for attempt in range(attempts_max):
        for i, chunk in enumerate(chunks):
            try:
                logger.info(f"attempt {attempt +1}/{attempts_max} chunk {i +1}/{len(chunks)}: {chunk}")
                message_result_json = send_message_with_retry(chunk, retries_max=retries_max, timeout_attempts_max=attempts_max, timeout_ms=timeout_ms)
                combined_results_json = merge_json(combined_results_json, message_result_json)
            except Exception as e:
                logger.error(f"Exception in chunk{i+1}: {e}")

    output_file = output_dir + '/' + 'GPTresults.json'
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(combined_results_json, f, ensure_ascii=False, indent=4)

    if( store_gpt_results):
        now = datetime.now().strftime("%Y-%m-%d-%H%M%S")
        output_file_backup = output_dir + '/' + list(data.keys())[0] + '_' + now + '.json'
        with open(output_file_backup, 'w', encoding='utf-8') as f:
            json.dump(combined_results_json, f, ensure_ascii=False, indent=4)

    logger.debug(f"result:{combined_results_json}")
    logger.info(f"done. Write to a file: {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a list of websites.')
    parser.add_argument('-f', '--json_file', type=str, required=True, help='json named array with websites. example: {<name>:[<url1>,<url2>,...]}')
    parser.add_argument('-m', '--maxsize', type=int, default=300, help='Maximum number of characters per subarray')
    parser.add_argument('-o', '--output_dir', type=str, default='GPTresults', help='Result output dir')
    parser.add_argument('-s', '--storeGpt', action='store_false', help='true - save all gpt results, false - save only last one')
    parser.add_argument('-t', '--timeout', type=int, default=300, help='timeout in ms in case of 429 response code or other != 200')
    parser.add_argument('-a', '--attempts', type=int, default=3, help='how many times will ask for the same data - for collect diffrent range of categories')
    parser.add_argument('-r', '--retries', type=int, default=3, help='in case of failure to receive valid data - how many times to retry')

    args = parser.parse_args()

    main(args)
