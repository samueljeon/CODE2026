import cv2
import csv
import json

def hex_to_bgr(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (4, 2, 0))  # RGB to BGR

def draw_text_with_bg(img, text, org, font, scale, text_color, bg_color, thickness=1, padding=2):
    # Get the size of the text box
    (text_w, text_h), baseline = cv2.getTextSize(text, font, scale, thickness)
    x, y = org

    # Draw filled rectangle background
    cv2.rectangle(
        img,
        (x - padding, y - text_h - padding),
        (x + text_w + padding, y + baseline + padding),
        bg_color,
        thickness=cv2.FILLED
    )

    # Draw the text over it
    cv2.putText(img, text, org, font, scale, text_color, thickness, lineType=cv2.LINE_AA)

# returns the river (list) from river_path with same name
def find_river(name, river_path):
    for i, river in enumerate(river_path):
        if river["name"] == name:
            return river
    return river

river_data = {}

with open("dataset.csv", mode='r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        year = int(row['Year'])
        # Remove the "Year" key and store the rest
        area_status = {key: row[key] for key in row if key != 'Year'}
        river_data[year] = area_status

base_image = cv2.imread("map.png")
with open("rivers.json", "r") as f:
    river_path = json.load(f)

instruction_coord = (20, 1200) # the starting coordinates to write instructions
legend_coord = (100,1200)

while True:
    for year in river_data:
        image = base_image.copy() # refreshes the image for every year
        draw_text_with_bg(image, "Press ESC to quit or ANY key to skip.", instruction_coord, cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,0,0), (255, 255, 255), 2)

        rivers_with_carps = []
        for area in river_data[year]:
            if river_data[year][area] == 'Y':
                rivers_with_carps.append(find_river(area, river_path))

        for river in rivers_with_carps:
            points = river["points"]
            name = river["name"]
            color = river.get("color", "#0000ff")
            bgr = hex_to_bgr(color)

            for i in range(1, len(points)):
                pt1 = tuple(points[i - 1])
                pt2 = tuple(points[i])
                cv2.line(image, pt1, pt2, bgr, 6)

            # Compute center of the path to place label
            mid_index = len(points) // 2
            mid_point = tuple(points[mid_index])

            # Offset the label slightly (e.g., 10 pixels right and 10 down)
            label_position = (mid_point[0] + 10, mid_point[1] + 10)

            # Draw river name in same color as the line
            draw_text_with_bg(image, name, label_position, cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255,255,255), bgr, 2)

            # Draw year (same as before)
            draw_text_with_bg(image, str(year), (80, 80), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0,0,0), (255, 255, 255), 5)


        if not rivers_with_carps:
            print("No Invasive Carps for Year", year)   
            draw_text_with_bg(image, str(year), (80, 80), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0,0,0), (255, 255, 255), 5)

        cv2.imshow("Rivers", image)

        if year != list(river_data.keys())[-1]:
            key = cv2.waitKey(2000) # wait unless it's the last year

        if key == 27:  # ESC
            cv2.destroyAllWindows()
            exit()

    # After full loop
    print("Press SPACE to replay or ESC to quit.")
    draw_text_with_bg(image, "Press SPACE to replay or ESC to quit.", instruction_coord, cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,0,0), (255, 255, 255), 2)
    cv2.imshow("Rivers", image)

    while True:
        key = cv2.waitKey(0)
        if key == 27:  # ESC
            cv2.destroyAllWindows()
            exit()
        elif key == 32:  # SPACE
            break  # Replay loop