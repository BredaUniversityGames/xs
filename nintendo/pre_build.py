import os
import json

user_path = os.path.join(os.path.expanduser("~"))
user_path = os.path.join(user_path, "AppData/Roaming/xs/settings.json")
with open(user_path, "r") as f:

    # get the game path from the settings.json file
    user_data = json.load(f)
    game_json = user_data["game"]
    input_game_path = game_json["value"]
    print("Input game path:" + input_game_path)
    
    output_game_folder = os.path.join("./games", "game")
    print("Output game folder:" + output_game_folder)
    
    if not os.path.exists(output_game_folder):
        print("Game folder does not exist. Creating game folder.")
        os.makedirs(output_game_folder)

    # sync the input and output game folders
    os.system("robocopy " + input_game_path + " " + output_game_folder + " /E /MIR /XF .gitignore .gitattributes *.py *.md *.pyc /XD .git __pycache__")

    