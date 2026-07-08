import subprocess
from datetime import datetime

def run_git(args):
    try:
        result = subprocess.run(['git'] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        raise Exception("git not found in PATH; it is required to generate code/version.hpp")
    if result.returncode != 0:
        raise Exception(f"git {' '.join(args)} failed: " + result.stderr.decode('utf-8').strip())
    return result.stdout.decode('utf-8').strip()

def get_last_commit_hash():
    return run_git(['rev-parse', 'HEAD'])

def get_commit_count_this_year():
    """Get the number of commits since January 1st of the current year"""
    current_year = datetime.now().year
    year_start = f"{current_year}-01-01"
    return int(run_git(['rev-list', '--count', f'--since={year_start}', 'HEAD']))

def generate_version_string():
    """Generate version string in format: YY.BuildNumber"""
    current_year = datetime.now().strftime('%y')  # Two-digit year
    build_number = get_commit_count_this_year()
    version_string = f"{current_year}.{build_number}"
    return version_string

def generate_version_with_hash():
    """Generate version string with commit hash: YY.BuildNumber+hash"""
    base_version = generate_version_string()
    commit_hash = get_last_commit_hash()
    return f"{base_version}+{commit_hash[:7]}"

if __name__ == "__main__":
    version = generate_version_string()
    version_with_hash = generate_version_with_hash()
    commit_hash = get_last_commit_hash()[:7]

    header_file_path = 'code/version.hpp'

    # Parse version components
    current_year = int(datetime.now().strftime('%y'))
    build_number = get_commit_count_this_year()

    # Generate the header file with version constants
    new_content = f'''#pragma once
#include <string>

namespace xs::version
{{
    // Version components (single source of truth)
    constexpr int XS_VERSION_YEAR = {current_year};
    constexpr int XS_VERSION_BUILD = {build_number};

    // Short commit hash
    constexpr const char* XS_COMMIT_HASH = "{commit_hash}";

    // Version string builder function (implemented in version.cpp)
    // Builds version strings from the integer components above
    // Parameters:
    //   include_hash: Include commit hash (default: false)
    //   include_config: Include build configuration like [dbg], [dev], [rel] (default: false)
    //   include_platform: Include platform like [pc], [switch], [ps5] (default: false)
    std::string get_version_string(bool include_hash = false, bool include_config = false, bool include_platform = false);
}}
'''

    try:
        with open(header_file_path, 'r', encoding='utf-8', newline='') as file:
            current_content = file.read()
    except FileNotFoundError:
        current_content = None

    if current_content != new_content:
        with open(header_file_path, 'w', encoding='utf-8', newline='') as file:
            file.write(new_content)
        print("Updated version.hpp")
    else:
        print("version.hpp unchanged, skipping write")

    print(f"Version: {version}")
    print(f"Full version: {version_with_hash}")
