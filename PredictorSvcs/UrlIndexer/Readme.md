# URLs Indexer Watcher

This project is a Python script that monitors a URL index in ClickHouse and Redis, periodically checking for expired entries and updating them by fetching data using GPT.

### GPT usage

- Runs the script from the [`GPT Script Folder`](../../Utils/GPT) - `getSiteCategories.py` script that fetch categories from GPT.

## Features

- **Backup and Restore**: Retrieve and restore data from ClickHouse and Redis.
- **Update and Insert**: Automatically update or insert data in ClickHouse and Redis.
- **Expiration Checking**: Periodically checks if URLs have expired and updates them using a GPT-powered categorization tool.
- **Signal Handling**: Gracefully handles termination signals (SIGTERM, SIGINT).

## Requirements

- **ClickHouse** and **Redis** running on your local machine.
- External script `getSiteCategories.py` for GPT-based categorization.
- Have **env** variables **YANDEX_ACCOUNT_ID** and **YANDEX_GPT_API_KEY** set with your Yandex GPT credentials.
### Python Dependencies

```bash
pip install -r requirements.txt
```

## Configuration

The script supports several configuration parameters:

- `--gptdir` Directory where GPT results will be stored. **(default: `GPTresults`)**
- `--storeGpt` If set, all GPT results will be stored. If not set, only the last result will be saved.
- `--checkTimeout` Time interval between URL checks, in seconds. **(default: `300` seconds)**
- `--expTime` URL expiration time, in the format `'Xd Xh Xm Xs'`. **(default: `60d`)**
- `--pathGPT` Path to the `getSiteCategories.py` script for URL categorization. **(default: `../../Utils/GPT/getSiteCategories.py`)**

## Usage
```bash
docker-compose up -d
```
```bash
YANDEX_ACCOUNT_ID=<> YANDEX_GPT_API_KEY=<> python3 urlIndexerWatcher.py
```

