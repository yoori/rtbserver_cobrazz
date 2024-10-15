import os
import time
import subprocess
import json
from datetime import datetime, timedelta
import clickhouse_connect
import redis

env = os.environ.copy()
env["YANDEX_GPT_API_KEY"] = os.getenv('YANDEX_GPT_API_KEY')
env["YANDEX_ACCOUNT_ID"] = os.getenv('YANDEX_ACCOUNT_ID')

# addUrlsPath - for running empty db to fill it
# updateUrlsPath - for checking each iteration if we need to do insert or update what is inside of this file
# after proccessing updateUrlsPath will be cleaned
#
# addUrlsPath and updateUrlsPath MUST no contain the same path
addUrlsPath = "addUrls.txt"
updateUrlsPath = "updateUrls.txt"

output_GPT_dir = 'GPTresults'
isGPTresulteStored  = True

timeoutBetweenChecks = 5*60 #5 min

expiredCheckSeconds = 0
expiredCheckMinutes = 0
expiredCheckHours = 0
expiredCheckDays = 60


# connect to dbs one time
clientClickHouse = clickhouse_connect.get_client(
    host='localhost',
    port=8123,
    username='default',
    password=''  # default password is empty
)
clientRedis = redis.Redis(host='localhost', port=6379, db=0)

# Функция для считывания содержимого файла
def read_file(filename):
    try:
        with open(filename, 'r') as file:
            content = file.read()
        return content
    except FileNotFoundError:
        print(f"Ошибка: не удалось открыть файл {filename}")
        return ""

# Функция для очистки содержимого файла
def clear_file(filename):
    try:
        with open(filename, 'w') as file:
            # Открытие в режиме 'w' автоматически очищает файл
            pass
        print(f"Файл {filename} успешно очищен.")
    except FileNotFoundError:
        print(f"Ошибка: не удалось открыть файл {filename}")

# Функция для проверки, пустой ли файл
def is_file_empty(filename):
    try:
        return os.path.getsize(filename) == 0
    except FileNotFoundError:
        print(f"файл {filename} не найден.")
        return True

def is_table_empty():
    """
    Проверяет, пуста ли таблица, и возвращает True, если пуста.
    """
    initial_check_query = "SELECT COUNT(*) FROM urls"
    initial_result = clientClickHouse.query(initial_check_query)
    return initial_result.result_rows[0][0] == 0

def backup(urls):
    """
    Извлекает текущие данные из ClickHouse и Redis по указанным URL.
    Возвращает два словаря: данные из ClickHouse и данные из Redis.
    """
    clickhouse_backup = {}
    redis_backup = {}

    # Извлечение данных из ClickHouse
    urls_str = "', '".join(urls)
    query = f"SELECT url, indexed_date FROM urls WHERE url IN ('{urls_str}')"
    try:
        result = clientClickHouse.query(query)
        clickhouse_backup = {row[0]: row[1] for row in result.result_rows}
    except Exception as e:
        print(f"Ошибка при извлечении данных из ClickHouse: {e}")

    # Извлечение данных из Redis
    try:
        for url in urls:
            redis_backup[url] = clientRedis.get(url)
    except Exception as e:
        print(f"Ошибка при извлечении данных из Redis: {e}")

    return clickhouse_backup, redis_backup

def restore(clickhouse_backup, redis_backup):
    """
    Восстанавливает данные в ClickHouse и Redis из резервных копий.
    """
    # Восстановление данных в ClickHouse
    for url, indexed_date in clickhouse_backup.items():
        try:
            query = f"ALTER TABLE urls UPDATE indexed_date = '{indexed_date}' WHERE url = '{url}'"
            clientClickHouse.command(query)
            print(f"Восстановлены данные в ClickHouse для {url}")
        except Exception as e:
            print(f"Ошибка при восстановлении данных в ClickHouse для {url}: {e}")

    # Восстановление данных в Redis
    for url, value in redis_backup.items():
        try:
            clientRedis.set(url, value)
            print(f"Восстановлены данные в Redis для {url}")
        except Exception as e:
            print(f"Ошибка при восстановлении данных в Redis для {url}: {e}")

def update(data_json_str):
    """
    Пытается обновить данные в ClickHouse и Redis.
    В случае ошибки восстанавливает старые данные.
    """
    # Создание резервной копии
    clickhouse_backup, redis_backup = backup(data_json_str)
    if not clickhouse_backup or not redis_backup:
        print("Ошибка: не удалось создать резервную копию данных.")
        return False

    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # Попытка обновления в ClickHouse
    urls_str = "', '".join(data_json_str.keys())
    update_query = f"ALTER TABLE urls UPDATE indexed_date = '{current_time}' WHERE url IN ('{urls_str}')"
    try:
        clientClickHouse.command(update_query)
        print("Данные обновлены в ClickHouse.")
    except Exception as e:
        print(f"Ошибка при обновлении данных в ClickHouse: {e}")
        print("Выполняется восстановление...")
        restore(clickhouse_backup, redis_backup)
        return False

    # Попытка обновления в Redis
    try:
        for url, values in data_json_str.items():
            values_json = json.dumps(values, ensure_ascii=False)
            clientRedis.set(url, values_json)
        print("Данные обновлены в Redis.")
    except Exception as e:
        print(f"Ошибка при обновлении данных в Redis: {e}")
        print("Выполняется восстановление...")
        restore(clickhouse_backup, redis_backup)
        return False
    return True

def insert(data_json_str):
    """
    Вставляет новые данные в ClickHouse и Redis.
    """
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # Формирование запроса для вставки данных в ClickHouse
    values = [f"('{url}', '{current_time}')" for url in data_json_str.keys()]
    insert_query = f"INSERT INTO urls (url, indexed_date) VALUES {', '.join(values)}"

    # Попытка вставки данных в ClickHouse
    try:
        clientClickHouse.command(insert_query)
        print("Новые данные успешно добавлены в ClickHouse.")
    except Exception as e:
        print(f"Ошибка при добавлении данных в ClickHouse: {e}")
        return False

    # Попытка вставки данных в Redis
    try:
        for url, values in data_json_str.items():
            values_json = json.dumps(values, ensure_ascii=False)
            clientRedis.set(url, values_json)
        print("Новые данные успешно добавлены в Redis.")
    except Exception as e:
        print(f"Ошибка при добавлении данных в Redis: {e}")
        return False
    return True

def update_and_insert(urls, data_json_str):
    """
    Обновляет существующие записи и добавляет новые записи в ClickHouse и Redis.
    """

    existing_fields, new_fields = separateUrls(urls)
    # Обновление существующих полей в ClickHouse и Redis
    if existing_fields:
        existing_data = {url: data_json_str[url] for url in existing_fields}
        if not update(existing_data):
            print("Ошибка при обновлении существующих данных.")
            return False

    # Вставка новых полей в ClickHouse и Redis
    if new_fields:
        new_data = {url: data_json_str[url] for url in new_fields}
        if not insert(new_data):
            print("Ошибка при добавлении новых данных.")
            return False

    return True

def separateUrls(urls):
    # Подготовка списка для существующих и новых полей
    existing_urls = []
    new_urls = []

    # Запрос для проверки наличия полей
    urls_str = "', '".join(urls)
    check_query = f"SELECT url FROM urls WHERE url IN ({urls_str})"

    try:
        # Выполнение запроса и сбор существующих полей
        result = clientClickHouse.query(check_query)
        existing_urls = [row[0] for row in result.result_rows]

        # Преобразуем строку fields в список, убирая пробелы, кавычки и разделители
        fields_list = [url.strip().strip("'") for url in urls.split(",")]

        # Определение новых полей (которые отсутствуют в existing_fields)
        new_urls = [field for field in fields_list if field not in existing_urls]
    except Exception as e:
        print(f"Ошибка при проверке существующих полей: {e}")

    return existing_urls, new_urls

def askGPT(urls):
    """
    Запускает скрипт с urls в качестве аргумента и выводит содержимое res.json.
    """
    os.makedirs(output_GPT_dir, exist_ok=True)
    if(isGPTresulteStored):
        output_file = os.path.join(output_GPT_dir, datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + '.json')
    else:
        output_file = os.path.join(output_GPT_dir, 'GPTresult.json')

    # Запускаем скрипт с аргументом urls
    result = subprocess.run(['python3', '../../Utils/GPT/getSiteCategories.py', '-w', urls, '-o', output_file], env=env)

    if result.returncode == 0:
        try:
            with open(output_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print('Файл', output_file,'не найден.')
        except json.JSONDecodeError:
            print('Ошибка при разборе',output_file,'JSON файла.')
    else:
        print(f"Скрипт ../../Utils/GPT/getSiteCategories.py завершился с "
              f"ошибкой: {result.stderr}")
    return None
def getExpiredUrls():
    threshold_date = (datetime.now() - timedelta(seconds=expiredCheckSeconds,
                                                 minutes=expiredCheckMinutes,
                                                 hours=expiredCheckHours,
                                                 days=expiredCheckDays)
                      ).strftime('%Y-%m-%d %H:%M:%S')

    query = f"SELECT url FROM urls WHERE indexed_date <= '{threshold_date}'"
    result = clientClickHouse.query(query)

    urls = "', '".join(row[0] for row in result.result_rows)
    if(not urls):
        urls = None
    else:
        urls = f"'{urls}'"

    return urls

def loadUrlsFromFile(urlsFile):
    if not is_file_empty(urlsFile):
        urls = '\'' + read_file(urlsFile) + '\''
        urls = urls.replace('\n', '')
        print("Содержимое файла:\n", urls)
        return urls
    return None

def checkAddFile():
    if is_table_empty():
        print('Table is empty - ok, load urls from', addUrlsPath )
        urls = loadUrlsFromFile(addUrlsPath)
        if ( urls == None ):
            print(addUrlsPath, 'file is empty - error - need to add urls')
            return 1
        data_json_str = askGPT(urls)
        update_and_insert(urls, data_json_str)
    return 0
def checkUpdateFile():
    urls = loadUrlsFromFile(updateUrlsPath)
    if(urls != None):
        data_json_str = askGPT(urls)
        ok = update_and_insert(urls, data_json_str)
        if(ok):
            clear_file(updateUrlsPath)

def checkExpiration():
    urls = getExpiredUrls()
    if urls != None:
        print('Expired URLs:', urls)
        data_json_str = askGPT(urls)
        if(data_json_str != None):
            allgood = update(data_json_str)
            if(not allgood):
                print('error during update')
def main():
    # Если таблица не заполнена - заполняем
    if(checkAddFile() > 0):
        return 1

    # Основной цикл
    try:
        while True:
            checkUpdateFile()
            checkExpiration()

            print('Sleep..')
            time.sleep(timeoutBetweenChecks)  # Спим таймаут минут перед повторной проверкой
    finally:
        # Закрываем соединение с базой данных при завершении работы
        clientClickHouse.close()
        clientRedis.close()
        print("Соединение со всеми бд закрыто.")


if __name__ == "__main__":
    main()

    #todo: add logger, add args and make an readme, rewrite comments