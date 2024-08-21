import os
import requests
import json
import time
import argparse
import logging
import colorlog

logger = logging.getLogger('main')
logger.setLevel(logging.DEBUG)

console_handler = colorlog.StreamHandler()
console_handler.setLevel(logging.DEBUG)

formatter = colorlog.ColoredFormatter(
    '%(log_color)s%(asctime)s - %(name)s - %(levelname)s - %(message)s\n',
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
            "temperature": 0.6,
            "maxTokens": "2000"
        },
        "messages": [
            {
                "role": "user",
                "text":"Ты выдаешь ответы в виде json сообщений - {<запрошеный вебсайт>:[<его категории>]}"
                       "Не пишешь вступительных слов и не используешь конструкцию command and results."
                       "Каждая категория как отдельный элемент."
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
    """Function for sending a request with repeated attempts when receiving 429 errors"""
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
                logger.warning(f"Received JSONDecodeError error. Retrying to ask the same data. (retries {retries+1}/{retries_max})")

                logger.error(f"Error converting a string to JSON: {json_error}")
                retries += 1
                if retries >= retries_max:
                    logger.error(f"The response could not be converted after {retries_max} attempts.")
            except Exception as e:
                logger.error(f"Exception in send_message_with_retry: {e}")

    logger.error(f"Failed to get response after {timeout_attempts_max} attempts")
    return {}

def merge_json(json1, json2):
    merged = json1.copy()  # copy of a first json
    for key, value in json2.items():
        if key in merged:
            # merge without dublicates
            merged[key] = list(set(merged[key] + value))
        else:
            # add from json2
            merged[key] = value
    return merged

def main(websites):
    websites = args.websites
    output_file = args.output
    max_chunk_size = args.maxsize
    timeout_ms = args.timeout
    attempts_max = args.attempts
    retries_max = args.retries

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

    logger.debug(f"result:{combined_results_json}")
    logger.info(f"done. Write to a file: {output_file}")
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(combined_results_json, f, ensure_ascii=False, indent=4)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a list of websites.')
    parser.add_argument('-w','--websites', type=str, required=True, help='Comma-separated list of websites')
    parser.add_argument('-m', '--maxsize', type=int, default=300, help="Maximum number of characters per subarray")
    parser.add_argument('-o', '--output', type=str, default='result.json', help='Result output filename')
    parser.add_argument('-t', '--timeout', type=int, default=300, help='timeout in ms in case of 429 response code or other != 200')
    parser.add_argument('-a', '--attempts', type=int, default=2, help='how many times will ask for the same data - for collect diffrent range of categories')
    parser.add_argument('-r', '--retries', type=int, default=3, help='in case of failure to receive valid data - how many times to retry')

    args = parser.parse_args()

    main(args)
