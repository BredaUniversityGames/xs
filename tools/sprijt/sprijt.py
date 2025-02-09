import os
import tkinter
import tkinter.messagebox
from tkinter import filedialog as fd
import customtkinter
from PIL import Image

customtkinter.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
customtkinter.set_default_color_theme("blue")  # Themes: "blue" (standard), "green", "dark-blue"

class App(customtkinter.CTk):
    def __init__(self):
        super().__init__()

        # configure window
        self.title("Sprijt")
        self.geometry(f"{1100}x{580}")

        self.iconphoto(False, tkinter.PhotoImage(file="icons/firefly.png"))


        # self.iconphoto(False, tkinter.PhotoImage(file="resources/icon.png"))

        ''''
        # open a file dialog to select an image
        filetypes = (
            ('png image', '*.png'),
            ('xs animation', '*.xsanim')
        )

        filename = fd.askopenfilename(
            title='Open image or animation',
            initialdir='/',
            filetypes=filetypes)
        
        canvas = customtkinter.CTkCanvas(self, width=500, height=500)
        canvas.pack()
        '''

        image_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "icons")

        self.open_icon_image = customtkinter.CTkImage(Image.open(os.path.join(image_path, "ic_fluent_folder_open_24_regular.png")), size=(16, 16))

        self.home_frame_button_1 = customtkinter.CTkButton(self, text="", image=self.open_icon_image, width=20, height=20, bg_color="transparent", fg_color="transparent", command=self.open_image)
        self.home_frame_button_1.pack()

        # button to open the file dialog
        self.button = customtkinter.CTkButton(self, text="Open image", command=self.open_image, bg_color="transparent", fg_color="transparent", width=50, height=50)
        self.button.pack()

        self.button2 = customtkinter.CTkButton(self, text="Open image", command=self.open_image)

        self.iconphoto(False, tkinter.PhotoImage(file="icons/firefly.png"))        

    ####################################################################################
    # methods
    ####################################################################################
    def recreate_image_panel(self):
        pass

    ####################################################################################
    # event handlers
    ####################################################################################

    def open_image(self):      
        app.iconphoto(False, tkinter.PhotoImage(file="icons/firefly.png"))

        # open a file dialog to select an image
        filetypes = (
            ('png image', '*.png'),
            ('xs animation', '*.xsanim')
        )

        filename = fd.askopenfilename(
            title='Open image or animation',
            initialdir='.',
            filetypes=filetypes)
        
        self.button.destroy()

if __name__ == "__main__":
    app = App()    
    app.mainloop()
