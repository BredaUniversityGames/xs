# This script will deploy the current Publish build 
print("making a build...")

import os
from pathlib import Path
import json
import shutil
from shutil import copytree, ignore_patterns

os.system('MSBuild xs.sln /p:Platform=PC /p:Configuration=Release')

source_dir = os.getcwd()
source_dir = source_dir + "\\executable\\x64\\Release"
asset_dir = os.getcwd()
asset_dir = asset_dir + "\\games"

output_dir = os.getcwd()
output_dir = output_dir + "\\build"
if os.path.exists(output_dir):
    shutil.rmtree(output_dir)

print(source_dir)
print(output_dir)

if os.path.exists(source_dir):
    shutil.copytree(source_dir, output_dir, False, ignore=ignore_patterns('*.pdb', '*.ipdb', '*.iobj', '*.cap' , "*.bnvib", 'steam_appid.txt'))
    output_dir = output_dir + "\\games"
    shutil.copytree(asset_dir, output_dir, False, ignore=ignore_patterns('*.obj', ".bnvib"))    
else:
    print("Source dir does not exist: " + source_dir)
