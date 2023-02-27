# This script will deploy the current Publish build 
print("Making a build...")

import os
import shutil
from pathlib import Path
from shutil import copytree, ignore_patterns

game_dir = "sample"
itch_name = "xs"
user = "xs-engine"

os.system('MSBuild xs.sln /p:Platform=PC /p:Configuration=Test')

source_dir = os.getcwd()
source_dir = source_dir + "\\executable\\x64\\Develop"

output_dir = os.getcwd()
output_dir = output_dir + "\\build"
if os.path.exists(output_dir):
    shutil.rmtree(output_dir)

    
print(source_dir)
print(output_dir)

current_dir = os.getcwd()
if os.path.exists(source_dir):
    
    shutil.copytree(source_dir, output_dir, False, ignore=ignore_patterns('*.pdb', '*.ipdb', '*.iobj', '*.cap' , "*.bnvib", 'steam_appid.txt', '*.ilk'))

    asset_dir = current_dir + "\\games\\" + game_dir
    output_dir = current_dir + "\\build\\games\\" + game_dir
    shutil.copytree(asset_dir, output_dir, False, ignore=ignore_patterns('*.obj', ".bnvib", "*.py", "*.pyc"))

    asset_dir = current_dir + "\\games\\" + "shared"
    output_dir = current_dir + "\\build\\games\\" + "shared"
    shutil.copytree(asset_dir, output_dir, False, ignore=ignore_patterns('*.obj', ".bnvib", "*.py"))    

    ini_src = current_dir + "\\games\\.ini"
    ini_dst = current_dir + "\\build\\games\\.ini"
    shutil.copyfile(ini_src, ini_dst)
    
    
    toml_src = current_dir + "\\tools\\.itch.toml"
    toml_dst = current_dir + "\\build\\.itch.toml"
    shutil.copyfile(toml_src, toml_dst)
    
    butler_command = "butler push build " + user + "/" + itch_name + ":win"
    os.system(butler_command)

else:
    print("Source dir does not exist: " + source_dir)

