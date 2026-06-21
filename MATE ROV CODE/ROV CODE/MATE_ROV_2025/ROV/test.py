import pygame
import cv2
import numpy as np

# initialize pygame
pygame.init()

# create a window
window_width = 640
window_height = 480
window = pygame.display.set_mode((window_width, window_height))
pygame.display.set_caption("Camera Display")

# create a font object
font = pygame.font.SysFont(None, 24)

# create a camera object
cap = cv2.VideoCapture(0)

# define GUI elements
button_width = 100
button_height = 30
button_x = window_width - button_width - 10
button_y = 10
button = pygame.Rect(button_x, button_y, button_width, button_height)
button_text = font.render("Take Picture", True, (255, 255, 255))

slider_width = 100
slider_height = 20
slider_x = 10
slider_y = window_height - slider_height - 10
slider = pygame.Rect(slider_x, slider_y, slider_width, slider_height)

# define colors
black = (0, 0, 0)
white = (255, 255, 255)

# define a function to convert a numpy array to a pygame surface
def np_to_surface(frame):
    return pygame.surfarray.make_surface(frame.swapaxes(0, 1))

# run the game loop
running = True
while running:

    # get events
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if button.collidepoint(event.pos):
                # take a picture
                ret, frame = cap.read()
                if ret:
                    # convert the frame to a pygame surface
                    surface = np_to_surface(frame)
                    # save the picture
                    pygame.image.save(surface, "picture.jpg")

    # get the slider value
    slider_value = int((slider.x - slider_x) / (slider_width - slider.height) * 255)

    # draw the camera display
    ret, frame = cap.read()
    if ret:
        surface = np_to_surface(frame)
        window.blit(surface, (0, 0))

    # draw the GUI elements
    pygame.draw.rect(window, white, button)
    window.blit(button_text, (button_x + 10, button_y + 5))
    pygame.draw.rect(window, white, slider)
    pygame.draw.rect(window, (slider_value, slider_value, slider_value), pygame.Rect(slider.x + 1, slider.y + 1, slider.width - slider.height + 2, slider.height - 2))

    # update the display
    pygame.display.update()

# release the camera and quit pygame
cap.release()
pygame.quit()