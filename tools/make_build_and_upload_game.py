# This script will deploy the current Publish build 
print("Making a build...")

import json
import os
import shutil
from pathlib import Path
from shutil import copytree, ignore_patterns

itch_name = "xs"
user = "buas"

os.system('MSBuild xs.sln /p:Platform=PC /p:Configuration=Develop')

source_dir = os.getcwd()
source_dir = source_dir + "\\executable\\x64\\Develop"

output_dir = os.getcwd()
output_dir = output_dir + "\\deploy\itch"
if os.path.exists(output_dir):
    shutil.rmtree(output_dir)

# read the settings file (in appdata folder)
settings_path =  os.getenv('APPDATA') + "\\xs\\settings.json" 
settings = {}
if os.path.exists(settings_path):
    with open(settings_path) as f:
        settings = json.load(f)

project_dir = settings["game"]["value"]
print("Project dir is:" + project_dir)

# get the last part of the path
itch_name = project_dir.split("\\")[-1]
print("Itch name is:" + itch_name)

print(source_dir)
print(output_dir)

current_dir = os.getcwd()
if os.path.exists(source_dir):
    
    shutil.copytree(source_dir, output_dir, False, ignore=ignore_patterns('*.pdb', '*.ipdb', '*.iobj', '*.cap' , "*.bnvib", 'steam_appid.txt', '*.ilk'))

    asset_dir = project_dir
    output_dir_game = output_dir + "\\game"
    shutil.copytree(asset_dir, output_dir_game, False, ignore=ignore_patterns('*.obj', "*.bnvib", "*.pyc", ".vscode", ".git", ".gitignore", ".gitattributes", "imgui.config"))

    asset_dir = current_dir + "\\assets"
    output_dir_shared = output_dir + "\\assets"
    shutil.copytree(asset_dir, output_dir_shared, False, ignore=ignore_patterns('*.obj', "*.bnvib", "*.py"))    
        
    toml_src = current_dir + "\\tools\\.itch.toml"
    toml_dst = output_dir + "\\.itch.toml"

    shutil.copyfile(toml_src, toml_dst)
    
    butler_command = "butler push deploy/itch " + user + "/" + itch_name + ":win"
    os.system(butler_command)
    
else:
    print("Source dir does not exist: " + source_dir)