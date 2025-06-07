import pandas as pd
import os
import json
import shutil
import sys

# Check for Excel file path argument
if len(sys.argv) < 2:
    print("Usage: python3 exel_to_jsons.py file.xlsx")
    sys.exit(1)

file_path = sys.argv[1]

# Check if file exists
if not os.path.exists(file_path):
    print(f"Error: File '{file_path}' not found.")
    sys.exit(1)

# Load the Excel file
xls = pd.ExcelFile(file_path)

# Create output directory for JSON files
output_dir = 'json_sheets'
os.makedirs(output_dir, exist_ok=True)

# Process each sheet
for sheet_name in xls.sheet_names:
    df = xls.parse(sheet_name, header=None)  # Load without headers
    first_column = df.iloc[:, 0].dropna().astype(str).tolist()
    trimmed_column = first_column[1:]  # Remove the first row (header)

    # Save to individual JSON file
    filename = f"{sheet_name}.json"
    full_path = os.path.join(output_dir, filename)
    with open(full_path, 'w', encoding='utf-8') as f:
        json.dump({sheet_name: trimmed_column}, f, ensure_ascii=False, indent=2)

print("Done.")