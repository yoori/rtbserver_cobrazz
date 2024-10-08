# Website Categories Fetcher

This script fetches categories for a list of websites using Yandex GPT API. The categories are returned in JSON format and saved to a file.

## Requirements

- Python 3.6+
- `requests` library
- `argparse` library

You can install the required libraries using pip:

```bash
pip install -r requirements.txt
```
## Notes
Ensure that you have a valid API key for the Yandex GPT API and add it to your <b>envoirment</b> as variables <b>YANDEX_ACCOUNT_ID</b> and <b>YANDEX_GPT_API_KEY</b>.

## Usage
To run the script, use the following command:

```bash
python getSiteCategories.py --websites "website1.com, website2.com"
```
## Arguments

- `--websites`: A comma-and-space separated list of websites for which you want to fetch categories.

## Example 1
```bash
 python3 getSiteCategories.py --websites "google.com, yandex.ru, facebook.com"
```

## Example 2
You can run it with passing environment variables.
```bash
 YANDEX_ACCOUNT_ID=<accoind_id> YANDEX_GPT_API_KEY=<API_key> python3 getSiteCategories.py --websites "google.com, yandex.ru, facebook.com"
```
The script will output the categories in <b>result.json</b> file in the following format:

```json
{
  "google.com": [
    "поиск",
    "реклама",
    "электронные письма",
    "новости",
    "видео",
    "карты",
    "облачные сервисы"
  ],
  "yandex.ru": [
    "поиск",
    "браузер",
    "картинки",
    "новости",
    "афиша",
    "карты"
  ],
  "facebook.com": [
    "социальные сети",
    "группы",
    "события",
    "поиск",
    "игры",
    "бизнес",
    "мессенджер"
  ]
}
```

