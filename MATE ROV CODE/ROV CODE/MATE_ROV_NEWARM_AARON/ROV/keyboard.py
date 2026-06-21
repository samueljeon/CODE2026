import pygame
import json
import time
import math
import widgets2 as widgets
import serial

# GUI window setup
sideBarWidth = 300
pygame.init()  # Initializes the pygame modules
size = width, height = 700 + sideBarWidth, 800  # size of GUI
pygame.display.set_caption('ROV Control')
screen = pygame.display.set_mode(size)
screen.fill((16, 43, 87))

# Setup displays in GUI
guiScreen = pygame.Surface((80 + sideBarWidth, 800), pygame.SRCALPHA)
guiTransparency = 0
guiScreen.fill((0, 0, 0, guiTransparency))

onStatus = widgets.toggleable("Running", sideBarWidth)  # label and size toggle

leftUpSlider = widgets.sliderdisplay("leftUp", 100, 320)
rightUpSlider = widgets.sliderdisplay("rightUp", 100, 320)
mLeftSlider = widgets.sliderdisplay("LeftSlider", 100, 320)
mRightSlider = widgets.sliderdisplay("RightSlider", 100, 320)

volt_display = widgets.display("Volt", sideBarWidth)
temp_display = widgets.display("Temp (C)", sideBarWidth)
th_up_display = widgets.display("Servo Up", sideBarWidth)
th_left_display = widgets.display("Servo Left", sideBarWidth)
th_right_display = widgets.display("Servo Right", sideBarWidth)
claw_display = widgets.display("Main Claw Value", sideBarWidth)
rotate_display = widgets.display("Rotating Claw Value", sideBarWidth)

font = pygame.font.SysFont("monospace", 16)
leftText = font.render("Left", True, (255, 255, 255))
rightText = font.render("Right", True, (255, 255, 255))
leftUpText = font.render("Left Up", True, (255, 255, 255))
rightUpText = font.render("Right Up", True, (255, 255, 255))

# Open serial com to Arduino
ser = serial.Serial(port='/dev/cu.usbserial-112410', baudrate=9600, timeout=.1, dsrdtr=True)
time.sleep(1)

# We will track claw and rotating‐servo values via keyboard
max_value = 90
rotate_max_value = 180
min_value = 0
clawValue = 0
rotateValue = 0

# Main loop
running = True
while running:
    # 1) Handle events
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            quit()

        # Toggle the “Running” status on key press. 
        # (You can change to whichever key you prefer, e.g. pygame.K_t)
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_RETURN:
                onStatus.toggle()

    # 2) Get the pressed keys
    keys = pygame.key.get_pressed()

    # 3) Keyboard-based increments/decrements for the claw
    #    e.g. Z = open, X = close (or vice versa—swap if you like)
    if keys[pygame.K_z]:  
        clawValue += 5  
    if keys[pygame.K_x]:
        clawValue -= 5

    # 4) Keyboard-based increments/decrements for the rotating servo
    #    e.g. C = rotate one direction, V = rotate the other
    if keys[pygame.K_c]:
        rotateValue += 5
    if keys[pygame.K_v]:
        rotateValue -= 5

    # 5) Limit the claw and rotate values
    if clawValue < min_value:
        clawValue = min_value
    if clawValue > max_value:
        clawValue = max_value
    if rotateValue < min_value:
        rotateValue = min_value
    if rotateValue > rotate_max_value:
        rotateValue = rotate_max_value

    # 6) Create variables for thrusters that used to come from the joystick
    #    Suppose arrow keys = forward/back (Up/Down) and turn left/right (Left/Right).
    #    W/S could handle vertical thruster, A/D for “crabbing” horizontally.
    x = 0  # will map to forward/back
    y = 0  # will map to turning left/right
    z = 0  # vertical thruster
    v = 0  # horizontal “crabbing”

    # ARROW KEYS for forward/back and turning
    if keys[pygame.K_UP]:
        x = -1  # negative used to match original code’s “-1 is forward”
    elif keys[pygame.K_DOWN]:
        x = 1   # +1 is backward

    if keys[pygame.K_LEFT]:
        y = -1  # turn left
    elif keys[pygame.K_RIGHT]:
        y = 1   # turn right

    # W & S for vertical thruster
    if keys[pygame.K_w]:
        z = -1  # up
    elif keys[pygame.K_s]:
        z = 1   # down

    # A & D for “crabbing” (horizontal movement)
    if keys[pygame.K_a]:
        v = -1  # crab left
    elif keys[pygame.K_d]:
        v = 1   # crab right

    # 7) If the toggled “Running” is on, scale up x,y as done before
    if onStatus.state:
        # The original code multiplied by 1.414 for full thrust
        x *= 1.414
        y *= 1.414

    # 8) Convert to the old 45-degree rotation
    #    (Same logic from your original code.)
    x_new = (x * (math.pi/4)) + (y * (math.pi/4))
    y_new = -(x * (math.pi/4)) + (y * (math.pi/4))

    # Limit to +/-1
    if x_new > 1:
        x_new = 1.0
    if x_new < -1:
        x_new = -1.0
    if y_new > 1:
        y_new = 1.0
    if y_new < -1:
        y_new = -1.0

    # 9) Build the commands dict
    commands = {}
    # Cubing the final thruster values as you did previously
    commands['tleft']  = x_new**3
    commands['tright'] = y_new**3

    # Use the same scale factor logic for v
    scale_factor = 0.2
    if v > 0:  # crabbing right
        commands['tup_left'] = round(v**3)
        commands['tup_right'] = commands['tup_left'] * scale_factor
    elif v < 0:  # crabbing left
        commands['tup_right'] = round(v**3)
        commands['tup_left'] = commands['tup_right'] * scale_factor
    else:
        # no crabbing => vertical thruster from z
        commands['tup_left'] = round(z**3)
        commands['tup_right'] = round(z**3)

    commands['claw']   = clawValue
    commands['rotate'] = rotateValue

    # Update GUI sliders
    mLeftSlider.value   = commands['tleft']
    mRightSlider.value  = commands['tright']
    leftUpSlider.value  = commands['tup_left']
    rightUpSlider.value = commands['tup_right']

    # 10) Send JSON to Arduino
    MESSAGE = json.dumps(commands)
    ser.write(bytes(MESSAGE, 'utf-8'))
    ser.flush()
    print("sending message", MESSAGE)

    # 11) Read from Arduino
    try:
        data = ser.readline().decode("utf-8")
        dict_json = json.loads(data)

        volt_display.value   = dict_json['volt']
        temp_display.value   = dict_json['temp']
        th_up_display.value  = dict_json['sig_up_1']
        th_left_display.value = dict_json['sig_lf']
        th_right_display.value = dict_json['sig_rt']
        claw_display.value   = dict_json['claw']
        rotate_display.value = dict_json['rotate']

        ser.flush()
    except Exception as e:
        print(e)

    # 12) Render everything on screen
    dheight = onStatus.get_height()
    guiScreen.blit(onStatus.render(), (0, 0))
    guiScreen.blit(mLeftSlider.render(),  (0,   9*dheight))
    guiScreen.blit(mRightSlider.render(), (100, 9*dheight))
    guiScreen.blit(leftUpSlider.render(), (200, 9*dheight))
    guiScreen.blit(rightUpSlider.render(),(300, 9*dheight))

    guiScreen.blit(temp_display.render(),       (0,   1*dheight))
    guiScreen.blit(th_up_display.render(),      (0,   2*dheight))
    guiScreen.blit(th_left_display.render(),    (0,   3*dheight))
    guiScreen.blit(th_right_display.render(),   (0,   4*dheight))
    guiScreen.blit(claw_display.render(),       (0,   5*dheight))
    guiScreen.blit(rotate_display.render(),     (0,   6*dheight))

    screen.blit(guiScreen, (0, 140))
    screen.blit(leftText, (15, 290))
    screen.blit(rightText, (115, 290))
    screen.blit(leftUpText, (215, 290))
    screen.blit(rightUpText, (315, 290))

    pygame.display.flip()
    time.sleep(0.01)
