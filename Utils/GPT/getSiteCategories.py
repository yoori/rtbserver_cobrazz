import os
import requests
import json
import argparse

outputfilename = 'result.json'

def main(websites):
    print("get categories for websites:", websites)
    prompt = {
        "modelUri": "gpt://{}/yandexgpt-lite".format(os.getenv('YANDEX_ACCOUNT_ID')),
        "completionOptions": {
            "stream": False,
            "temperature": 0.6,
            "maxTokens": "2000"
        },
        "messages": [
            {
                "role": "system",
                "text": "Ты выдаешь короткии ответы в виде json сообщений с категориями предложенных сайтов."
            },
            {
                "role": "user",
                "text": "хочу видеть формат в виде {<websitesite:> <категории>} коротко"
            },
            {
                "role": "user",
                "text": "{{ \"command\": \"какие все категории у сайтов {}?\" }}".format(websites)
            }
        ]
    }

    url = "https://llm.api.cloud.yandex.net/foundationModels/v1/completion"
    headers = {
        "Content-Type": "application/json",
        "Authorization": "Api-Key {}".format(os.getenv('YANDEX_GPT_API_KEY'))
    }

    response = requests.post(url, headers=headers, json=prompt)
    result = response.text
    # print(result)
    json_obj = json.loads(result)
    message_text = json_obj["result"]["alternatives"][0]["message"]["text"]
    # print(message_text)
    message_text = message_text.strip('`').strip()
    inner_json_obj = json.loads(message_text)
    
    print("Done. Write to a file:", outputfilename)
    print(inner_json_obj)
    with open("result.json", 'w', encoding='utf-8') as f:
        json.dump(inner_json_obj, f, ensure_ascii=False, indent=4)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a list of websites.')
    parser.add_argument('--websites', type=str, required=True, help='Comma-separated list of websites')

    args = parser.parse_args()
    main(args.websites)
