import subprocess
from datetime import datetime

def get_last_commit_hash():
    result = subprocess.run(['git', 'rev-parse', 'HEAD'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise Exception("Error getting the last commit hash: " + result.stderr.decode('utf-8'))
    return result.stdout.decode('utf-8').strip()

def generate_version_string():
    current_date = datetime.now().strftime('%Y%m%d')
    commit_hash = get_last_commit_hash()
    version_string = f"{current_date}-{commit_hash[:7]}"
    return version_string

if __name__ == "__main__":
    version = generate_version_string()
    header_file_path = 'code/version.hpp'
    new_content = f'''#pragma once \n#include <string> \nnamespace xs::version {{ const std::string version_string = "{version}"; }}'''

    try:
        with open(header_file_path, 'r') as file:
            current_content = file.read()
    except FileNotFoundError:
        current_content = ""

    if current_content != new_content:
        with open(header_file_path, 'w') as file:
            file.write(new_content)

    print(f"Generated version string: {version}")