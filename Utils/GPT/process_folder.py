import os
import sys
import subprocess
import getSiteCategories

env = os.environ.copy()

not_processed_files = []
def process_all_files_in_directory(directory_path):
    # Iterate through all entries in the directory
    for filename in os.listdir(directory_path):
        full_path = os.path.join(directory_path, filename)
        # Only process regular files (not directories)
        if os.path.isfile(full_path):
            # Run the script with the urls argument
            # to run getSiteCategories.py it is requared to have this two in env:
            #   env["YANDEX_GPT_API_KEY"] = os.getenv('YANDEX_GPT_API_KEY')
            #   env["YANDEX_ACCOUNT_ID"] = os.getenv('YANDEX_ACCOUNT_ID')
            result = subprocess.run(['python3', 'getSiteCategories.py', '-f', full_path], env=env)
            if result.returncode == 0:
                print(f"{filename} processed successfully.")
            else:
                print(f"Error processing {filename}: {result.stderr.decode()}")
                not_processed_files.append(filename)

if __name__ == "__main__":
    # Expect exactly one command-line argument: the directory path
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <directory_path>")
        sys.exit(1)

    folder_path = sys.argv[1]
    # Check if the provided path is a valid directory
    if not os.path.isdir(folder_path):
        print(f"Error: '{folder_path}' is not a directory")
        sys.exit(1)

    # Process all files in the given directory
    process_all_files_in_directory(folder_path)

    # Print the list of files that were not processed
    if not_processed_files:
        print("The following files were not processed:")
        for file in not_processed_files:
            print('    ', file)
    else:
        print("All files processed successfully.")
