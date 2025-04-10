# Copy dependencies of non-open-source libraries to the external directory
import os
from shutil import copytree
import platform

# 1 - Copy Fmod dependencies
def copy_fmod():
    # Get the current platform
    plat = platform.system()

    if plat == "Linux":
        print("Copying Fmod dependencies for Linux is not implemented yet.")
    elif plat == "Darwin":
        print("Copying Fmod dependencies for Mac is not implemented yet.")
    elif plat == "Windows":

        # Windows
        fmod_win_core_include = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\\api\core\inc")
        fmod_win_core_lib = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\\api\core\lib\\x64")
        fmod_win_studio_include = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\\api\studio\inc")
        fmod_win_studio_lib = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\\api\studio\lib\\x64")
        fmod_win_external_dir = os.path.join("external", "fmod")
        fmod_win_include_dest = os.path.join(fmod_win_external_dir, "inc")
        fmod_win_lib_dest = os.path.join(fmod_win_external_dir, "lib")
        os.makedirs(fmod_win_external_dir, exist_ok=True)
        os.makedirs(fmod_win_include_dest, exist_ok=True)
        os.makedirs(fmod_win_lib_dest, exist_ok=True)
        # Copy the include files
        copytree(fmod_win_core_include, fmod_win_include_dest, dirs_exist_ok=True)
        copytree(fmod_win_studio_include, fmod_win_include_dest, dirs_exist_ok=True)
        # Copy the lib files
        copytree(fmod_win_core_lib, fmod_win_lib_dest, dirs_exist_ok=True)
        copytree(fmod_win_studio_lib, fmod_win_lib_dest, dirs_exist_ok=True)
        print("Fmod dependencies copied successfully.")

        # Switch
        fmod_switch_core_include = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Switch\\api\core\inc")
        fmod_switch_core_lib = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Switch\\api\core\lib\\x64")
        fmod_switch_studio_include = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Switch\\api\studio\inc")
        fmod_switch_studio_lib = os.path.join("C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Switch\\api\studio\lib\\x64")        
        # Finish later...

    else:
        print("Unsupported platform. Please copy the dependencies manually.")
    

    
# Create UI to ask for user input
def main():
    print("Copy dependencies of non-open-source libraries to the external directory")
    print("1. FMod")
    print("2. Exit")
    choice = int(input("Enter your choice: "))
    if choice == 3:
        exit()
    elif choice == 1:
        copy_fmod()

main()
   


