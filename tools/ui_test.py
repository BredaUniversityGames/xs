import tkinter as tk
import customtkinter

# create Ck window
root = customtkinter.CTk()

# Use CkButton instead of tkinter Button
button = customtkinter. CkButton (master=root, text="Hello World!")

#window = tk.Tk()
#label = tk.Label(text="Python rocks!")
#label.pack()

#window.mainloop()