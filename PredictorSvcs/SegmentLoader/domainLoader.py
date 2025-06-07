from clickhouse_driver import Client
import json
import math
import os
from urllib.parse import urlparse

CHUNK_SIZE = 1000  # Количество доменов в одном JSON-файле
OUTPUT_DIR = "domains"  # Папка для сохранения результатов

def extract_main_domain(domain):
    """Извлекает основной домен (SLD + TLD) с учетом новых gTLD"""
    # Удаляем возможные www и протоколы
    domain = domain.lower().replace('www.', '').replace('http://', '').replace('https://', '')

    # Разбиваем на части
    parts = domain.split('.')

    # Если это IP-адрес или односоставный домен (localhost)
    if len(parts) <= 1 or domain.replace('.', '').isdigit():
        return domain

    # Специальная обработка для новых gTLD (как .game, .app, .dev и т.д.)
    # Если TLD короткий (2-3 символа), но не является стандартным ccTLD
    if len(parts[-1]) > 2:  # Это новый gTLD (как .game, .app)
        return f"{parts[-2]}.{parts[-1]}" if len(parts) >= 2 else domain

    # Для стандартных доменов (com, net, org и т.д.)
    return '.'.join(parts[-2:])

# Создаём папку, если её нет
os.makedirs(OUTPUT_DIR, exist_ok=True)

client = Client('click00')

query = """
    SELECT domain
    FROM ccgsitereferrerstats
    GROUP BY domain
    HAVING count() = 1
"""

print("[1/3] Загрузка данных из ClickHouse...")
rows = client.execute(query)
print(f"[2/3] Найдено {len(rows)} доменов")

# Фильтруем и обрабатываем домены
unique_domains = set()

for domain, in rows:
    if not domain.isdigit() and '.' in domain:
        unique_domains.add(extract_main_domain(domain))

total_domains = len(unique_domains)
print(f"[2/3] Найдено {total_domains} доменов (после обработки)")

# Разбиваем на файлы
num_files = math.ceil(total_domains / CHUNK_SIZE)
print(f"[3/3] Сохранение в {num_files} JSON-файлов...")

sorted_domains = sorted(unique_domains)
for i in range(num_files):
    chunk = sorted_domains[i*CHUNK_SIZE : (i+1)*CHUNK_SIZE]
    file_path = os.path.join(OUTPUT_DIR, f"domains_{i+1}.json")

    with open(file_path, "w", encoding="utf-8") as f:
        json.dump({"Websites": chunk}, f, ensure_ascii=False, indent=2)

    print(f"  → Сохранён {file_path} ({len(chunk)} доменов)")

print(f"Готово! Результаты в папке '{OUTPUT_DIR}'")