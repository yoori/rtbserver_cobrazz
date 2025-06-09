import json
import math
import os
from urllib.parse import urlparse

CHUNK_SIZE = 1000
OUTPUT_DIR = "domains"

# def extract_main_domain(domain):
#     domain = domain.lower().replace('www.', '').replace('http://', '').replace('https://', '')
#     parts = domain.split('.')
#
#     # In case of IP-adress or single-component domain (localhost)
#     if len(parts) <= 1 or domain.replace('.', '').isdigit():
#         return domain
#
#     # Special treatment for new gTLDs (as .game, .app, .dev, etc.)
#     # If the TLD is short (2-3 characters), but is not a standard ccTLD
#     if len(parts[-1]) > 2:  # Это новый gTLD (как .game, .app)
#         return f"{parts[-2]}.{parts[-1]}" if len(parts) >= 2 else domain
#
#     # For standard domains (com, net, org, etc.)
#     return '.'.join(parts[-2:])

os.makedirs(OUTPUT_DIR, exist_ok=True)

client = Client('click00')

query = """
    SELECT domain
    FROM ccgsitereferrerstats
    GROUP BY domain
    HAVING count() = 1
"""

print("[1/3] Loading data from ClickHouse...")
rows = client.execute(query)
print(f"[2/3] Found {len(rows)} domains")

# Filter and process domains
unique_domains = set()

for domain, in rows:
    if not domain.isdigit() and '.' in domain:
        unique_domains.add(extract_main_domain(domain))

total_domains = len(unique_domains)
print(f"[2/3] Found {total_domains} domains (after processing)")

# Split into files
num_files = math.ceil(total_domains / CHUNK_SIZE)
print(f"[3/3] Saving to {num_files} JSON files...")

sorted_domains = sorted(unique_domains)
for i in range(num_files):
    chunk = sorted_domains[i*CHUNK_SIZE : (i+1)*CHUNK_SIZE]
    file_path = os.path.join(OUTPUT_DIR, f"domains_{i+1}.json")

    with open(file_path, "w", encoding="utf-8") as f:
        json.dump({"Websites": chunk}, f, ensure_ascii=False, indent=2)

    print(f"  → Saved {file_path} ({len(chunk)} domains)")

print(f"Done! Results in folder '{OUTPUT_DIR}'")