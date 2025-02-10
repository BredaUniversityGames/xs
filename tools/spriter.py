import customtkinter

customtkinter.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
customtkinter.set_default_color_theme("dark-blue")  # Themes: "blue" (standard), "green", "dark-blue"


class App(customtkinter.CTk):
    def __init__(self):
        super().__init__()

        self.title("CustomTkinter Spriter")
        self.geometry("400x400")
        
        self.canvas = customtkinter.CTkCanvas(self, width=400, height=400)
        self.canvas.create_line(0, 0, 200, 200, fill="red")
        self.canvas.create_line(0, 400, 200, 200, fill="red")
        self.canvas.place(x=0, y=0)


        self.label = customtkinter.CTkLabel(self, text="Hello, World!")
        self.label.pack(pady=10)

        self.button = customtkinter.CTkButton(self, text="Click Me!", command=self.change_text)
        self.button.pack(pady=10)


    def change_text(self):
        self.label.configure(text="Button Clicked!")
        self.canvas.create_line(0, 200, 200, 0, fill="blue")
        self.canvas.create_line(0, 200, 200, 400, fill="blue")        

if __name__ == "__main__":
    app = App()
    app.mainloop()

