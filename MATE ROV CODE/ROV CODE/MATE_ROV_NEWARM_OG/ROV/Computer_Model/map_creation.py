import tkinter as tk
from tkinter import simpledialog, filedialog
from PIL import Image, ImageTk
import json

# Load the original image
MAP_IMAGE_PATH = "map.png"
image = Image.open(MAP_IMAGE_PATH)
image_width, image_height = image.size

# Keep full-size image for accurate coordinate saving

rivers = []
current_line = []
current_name = ""

def save_data():
    # Remove consecutive duplicates and include color
    cleaned_rivers = []

    for river in rivers:
        cleaned = []
        prev = None
        for pt in river["points"]:
            if pt != prev:
                cleaned.append(pt)
                prev = pt

        cleaned_rivers.append({
            "name": river["name"],
            "color": river.get("color", "blue"),
            "points": cleaned
        })

    filepath = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON Files", "*.json")])
    if filepath:
        with open(filepath, "w") as f:
            json.dump(cleaned_rivers, f)
        print(f"Saved {len(cleaned_rivers)} river(s) to {filepath}")

def on_click(event):
    global current_line
    x = canvas.canvasx(event.x)
    y = canvas.canvasy(event.y)
    current_line = [(x, y)]

def on_drag(event):
    global current_line
    x = canvas.canvasx(event.x)
    y = canvas.canvasy(event.y)
    if current_line:
        x1, y1 = current_line[-1]
        canvas.create_line(x1, y1, x, y, fill="blue", width=2)
        if (x, y) != (x1, y1):
            current_line.append((x, y))

def on_release(event):
    global current_line, current_name
    if current_line:
        current_name = simpledialog.askstring("River Name", "Enter a name for this river:")
        if current_name:
            color_input = simpledialog.askstring("River Color", "Enter a color (e.g. #ff0000, or rgb(0,255,255)):")

            # Parse RGB string if needed
            if color_input and color_input.startswith("rgb("):
                try:
                    rgb = color_input[4:-1].split(",")
                    r, g, b = [int(c.strip()) for c in rgb]
                    color_input = f"#{r:02x}{g:02x}{b:02x}"
                except:
                    color_input = "#ff0000"  # Fallback default

            if not color_input:
                color_input = "#ff0000"

            # Draw the line again with the selected color
            for i in range(1, len(current_line)):
                x1, y1 = current_line[i - 1]
                x2, y2 = current_line[i]
                canvas.create_line(x1, y1, x2, y2, fill=color_input, width=2)

            river = {
                "name": current_name,
                "color": color_input,
                "points": [(int(px), int(py)) for px, py in current_line]
            }
            rivers.append(river)

        current_line = []


root = tk.Tk()
root.title("River Mapper")

tk_img = ImageTk.PhotoImage(image)

# Scrollable canvas setup
frame = tk.Frame(root)
frame.pack(fill=tk.BOTH, expand=True)

canvas = tk.Canvas(frame, width=1000, height=700, scrollregion=(0, 0, image_width, image_height))
canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

# Scrollbars
x_scroll = tk.Scrollbar(frame, orient=tk.HORIZONTAL, command=canvas.xview)
x_scroll.pack(side=tk.BOTTOM, fill=tk.X)
y_scroll = tk.Scrollbar(frame, orient=tk.VERTICAL, command=canvas.yview)
y_scroll.pack(side=tk.RIGHT, fill=tk.Y)

canvas.configure(xscrollcommand=x_scroll.set, yscrollcommand=y_scroll.set)

# Display image
canvas.create_image(0, 0, anchor="nw", image=tk_img)

# Mouse bindings
canvas.bind("<Button-1>", on_click)
canvas.bind("<B1-Motion>", on_drag)
canvas.bind("<ButtonRelease-1>", on_release)

# Save button
btn_save = tk.Button(root, text="Save Rivers", command=save_data)
btn_save.pack(pady=10)

root.mainloop()
