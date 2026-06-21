import pygame.camera
import json
import time
import pygame
import math  # needed for joystick
import widgets2 as widgets
import serial  # needed to talk with Arduino

# GUI window setup
sideBarWidth = 300
pygame.init()  # Initializes the pygame modules
size = width, height = 700 + sideBarWidth, 800  # size of GUI
pygame.display.set_caption('ROV Control')
screen = pygame.display.set_mode(size)
screen.fill((16, 43, 87))

# camera setup
# camera_width = 640
# camera_height = 400
# pygame.camera.init()  # Initializes the camera modules from the pygame library : Added 5/8/23

# initialize two cameras (loading the camera modules)
# cam1 = pygame.camera.Camera(1, (camera_width, camera_height))  # for taking pictures
# cameraSurface = pygame.Surface((camera_width, camera_height*2), pygame.SRCALPHA)
# Creating camera surface for camera images to be rendered onto
# transparency = 0
# cameraSurface.fill((0, 0, 0, transparency))

# start the cameras (turn on cameras)
# cam1.start()

# setup displays in GUI
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
claw_display = widgets.display("Main Claw Value", sideBarWidth)  # Added 4/14/23 to set up object for claw value displays GUI
rotate_display = widgets.display("Rotating Claw Value", sideBarWidth)  # Added 4/6/24 to set up object for rotating claw value displays GUI

font = pygame.font.SysFont("monospace", 16)
leftText = font.render("Left", True, (255, 255, 255))
rightText = font.render("Right", True, (255, 255, 255))
leftUpText = font.render("Left Up", True, (255, 255, 255))
rightUpText = font.render("Right Up", True, (255, 255, 255))

# open serial com to Arduino
ser = serial.Serial(port='/dev/cu.usbserial-112410', baudrate=9600, timeout=.1, dsrdtr=True)
# dsrdtr=True stops Arduino Mega from auto resetting

time.sleep(1)

trigger_button = [False, False]  # Initialize False Boolean values for Left Button and Right Button
# trigger_button = [(LeftButton State) = False, (RightButton State) = False].
# This is so that the trigger buttons do not increment/decrement the clawValue
# unless triggered by user.
x_y_button = [False, False]
# setup for spare servo motor; Initialize False Boolean values for X (button 1) and Y button (button 3)
# Initialize min/max and clawValues
max_value = 90
rotate_max_value = 180  # After initial tests with the claw, 0-65 is the ideal safe operating range for the claw (0 = fully closed, 65 = fully opened.)
min_value = 0
clawValue = 0
rotateValue = 0

'''
*Note*: The range is specified here in Python, but is then rescaled in the Arduino sketch to be in the range from 1000-1650
Notice that when this Python program runs, the claw/servo values displayed will range from 1000-1650. This range can be freely adjusted.
'''

# Initialize joystick
# joystick = None
# if pygame.joystick.get_count() == 0:
#     print('No joystick Detected')

#     pygame.quit()
#     quit()
# else:
#     joystick = pygame.joystick.Joystick(0)
#     joystick.init()  # initalize joystick

pygame.joystick.init()

joysticks = [pygame.joystick.Joystick(i) for i in range(pygame.joystick.get_count())]
for joystick in joysticks:
    print(joystick.get_name()) # Prints the name of the button or joystick being used


# Set the variable to control image capture
# capture_count = 0

# Main Event Loop
while True:
    # Get input from joystick and keyboard
    pygame.event.pump()
    key = pygame.key.get_pressed()
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            # cam1.stop()

            pygame.quit()
            quit()
        if event.type == pygame.JOYBUTTONDOWN:
            if event.button == 0:  # Button A; Button ID: 0 used for toggling max thruster status On/Off
                print("Button A Pressed")
                onStatus.toggle()

        if event.type == pygame.JOYBUTTONDOWN:  # Using JoyButtonDown method (trigger button is pressed or x/y buttons are pressed)
            print(event.button)
            if event.button == 9:  # Left Button on Controller
                trigger_button[0] = True
            elif event.button == 10:  # Right Button on Controller
                trigger_button[1] = True
            elif event.button == 2:  # X button on the Controller
                x_y_button[0] = True
                print(event)
            elif event.button == 3:  # Y button on the Controller
                x_y_button[1] = True
                print(event)
        if event.type == pygame.JOYBUTTONUP:  # Using JoyButtonUp method (trigger button is released or x/y buttons are released)
            if event.button == 9:  # If Left Trigger Button is released, then set Boolean value back to False
                trigger_button[0] = False
            elif event.button == 10:  # If Right Trigger Button is released, then set Boolean value back to False
                trigger_button[1] = False
            elif event.button == 2:  # If X button is released, then set Boolean value back to False
                x_y_button[0] = False
                print(event)
            elif event.button == 3:  # If Y button is released, then set Boolean value back to False
                x_y_button[1] = False
                print(event)

    # Incrementing/Decrementing the Main Claw Values
    if trigger_button[0] == True:
        clawValue += 5  # After some testing, increment and decrement value of 5 is the most optimal. Adjust if needed.
    elif trigger_button[1] == True:
        clawValue -= 5
    # Sets the ClawValue to the value the moment the button is released.
    elif trigger_button[0] == False:
        clawValue = clawValue
    elif trigger_button[1] == False:
        clawValue = clawValue

    # Incrementing/Decrementing the Servo1 Values
    if x_y_button[0] == True:
        rotateValue += 5  # After some testing, increment and decrement value of 5 is the most optimal. Adjust if needed.
    if x_y_button[1] == True:
        rotateValue -= 5

    # Sets the 2nd Servo to the value the moment the button is released.
    if x_y_button[0] == False:
        rotateValue = rotateValue
    if x_y_button[1] == False:
        rotateValue = rotateValue

    # Limit value to minimum; Establishes floor condition
    if clawValue < min_value:
        clawValue = min_value
    # Limit value to maximum; Establishs ceiling condition
    if clawValue > max_value:
        clawValue = max_value

    # Limit value to minimum; Establishes floor condition for second servo motor
    if rotateValue < min_value:
        rotateValue = min_value
    # Limit value to maximum; Establishs ceiling condition for second servo motor
    if rotateValue > rotate_max_value:
        rotateValue = rotate_max_value

    '''
    Archived Code: Repeated button presses to Open and Close the claw
    if event.button == 2: # X Button (Opens the claw)
        clawValue -= 10 # set 
        if clawValue < 0: # set minimum value/ Prevents any negative value for the motor direction values. Establishes a floor condition.
            clawValue = 0 # Resets any negative motor direction values to the minimum value.
    if event.button == 3: # Y Button (Close the claw)
        clawValue += 10   # Increments value for fine tuning control
        if clawValue > 100: # set maximum value (ceiling)
            clawValue = 100 # If this value is reached, the claw is completely open.
    '''

# Commands to send ROV
    commands = {}  # define python dictionary
    if joystick is not None:
        x = joystick.get_axis(1)  # left joystick -1 is left to +1 is right (left thruster)
        y = joystick.get_axis(0)  # left joystick -1 is up +1 is down (right thruster)
        z = joystick.get_axis(3)  # right joystick x-axis, used for vertical
        #J
        v = joystick.get_axis (2)  # used for horizontal (crabbing) movement in the right joystick

    if abs(x) < .2:  # define a dead zone
        x = 0
    if abs(y) < .2:  # define a dead zone
        y = 0
    if abs(z) < .2:  # define a dead zone
        z = 0
    if abs(v) < .2: # define a dead zone, J
        v = 0

    if onStatus.state:  # When the status is toggled to "On" by pressing Button A on the controller: Limits thrust for SURGE direction (forward/backward).
        y = y * 1.414  # gives value of 1 for full thrust forward and backwards
        x = x * 1.414  # gives value of 1 for full thrust forward and backwards

    cutoff = 0.2
        
    
    
    # rotate x and y-axis of joystick 45 degrees    
    x_new = (x*(math.pi/4)) + (y*(math.pi/4))               #Fixed signs to match --> https://en.wikipedia.org/wiki/Rotation_of_axes_in_two_dimensions
    y_new = -(x*(math.pi/4)) + (y*(math.pi/4))
    
    magnitudeAvg = abs(x_new)+abs(y_new) / 2
    
    if (x_new < 0 and y_new > 0) : #Forward 
        x_new = -magnitudeAvg
        y_new = magnitudeAvg
    elif (x_new > 0 and y_new < 0): #Backward 
        x_new = magnitudeAvg
        y_new = -magnitudeAvg
    elif (x_new < 0 and y_new < 0): #Rotation in place to the left
        x_new = -magnitudeAvg
        y_new = x_new
    elif (x_new > 0 and y_new > 0): #Rotation in place to the right
        x_new = magnitudeAvg
        y_new = x_new
    
   # print(x_new, y_new)

    # limits joystick values to +/- 1
    if x_new > 1:
        x_new = 1.0
    if y_new > 1:
        y_new = 1.0
    if x_new < -1:
        x_new = -1.0
    if y_new < -1:
        y_new = -1.0


    commands['tleft'] = x_new ** 3
    commands['tright'] = y_new ** 3
    
    #print(commands['tleft'], commands['tright'])
    
    #tune scale_factor as desired. 0.2 is the lowest value other than 0. values between 0 and 0.2 are cast to 0(i.e, dominant thrust at 100%, non-dominant thruster is off)
    scale_factor = 0.2              #Controls how fast the non-dominant vertical thruster spins relative to the dominant vertical thruster.
                                    #e.g, when "crabbing" right, the LEFT vertical thruster is the dominant thruster. The right
                                    #vertical thruster is the non-dominant thruster. You may want the non-dominant thruster to spin
                                    #at 20%, 30%, 40%, or maybe even 0% the speed of the dominant thruster. This is up to what the pilot is most comfortable with.
    if v > 0:  # Crabbing right - We want more power allocated to the thruster on the opposite side to move in the correct direction
        commands['tup_left'] = round(v ** 3) # Controler will return +-0.99xx instead of 1, this float gets integer cast to 0, causing the motor to not turn, hence the round
        commands['tup_right'] = commands['tup_left'] * scale_factor # Minimal thrust on the right horizontal thruster 
        print(commands['tup_left'], commands['tup_right'])

    elif v < 0:  # Crabbing left
        commands['tup_right'] = round(v ** 3)
        commands['tup_left'] = commands['tup_right'] * scale_factor
    else: # No crabbing, going striaght up or down
        commands['tup_left'] = round(z ** 3) 
        commands['tup_right'] = round(z ** 3) #Integer round horseshit
        

    commands['claw'] = clawValue  # send the claw value
    commands['rotate'] = rotateValue  # send the servo value to arduino Addded: 4/6/24
    

    mLeftSlider.value = commands['tleft']  # assign thruster values to a display
    mRightSlider.value = commands['tright']
    leftUpSlider.value = commands['tup_left']
    rightUpSlider.value = commands['tup_right']

    MESSAGE = json.dumps(commands)  # puts python dictionary in Json format
    ser.write(bytes(MESSAGE, 'utf-8'))  # byte format sent to arduino
    ser.flush()

    print("sending message", MESSAGE)

    try:
        data = ser.readline().decode("utf-8")  # decode into byte from Arduino in duino

        # data = ser.readline().strip()
        # if data:
        #     try:
        #         dict_json = json.loads(data)
        #         # ... do your processing ...
        #     except json.JSONDecodeError as e:
        #         print("Could not decode JSON:", e, "| Raw data was:", data)
        # else:
        #     print("No data received from serial this cycle.")

        dict_json = json.loads(data)  # data from ardictionary form

        volt_display.value = dict_json['volt']  # assign temp to dispaly
        temp_display.value = dict_json['temp']  # assign temp to dispaly
        th_up_display.value = dict_json['sig_up_1']  # vertical thruster value from Arduino
        th_left_display.value = dict_json['sig_lf']  # horizontal thruster value from Arduino
        th_right_display.value = dict_json['sig_rt']  # horizontal thruster value from Arduino
        claw_display.value = dict_json['claw']  # claw value from Arduino
        rotate_display.value = dict_json['rotate']  # servo value from Arduino

        ser.flush()

    except Exception as e:
        print(e)

    pass
    # Draw Stuff (Rendering the data as a display for the GUI)
    dheight = onStatus.get_height()
    guiScreen.blit(onStatus.render(), (0, 0))
    guiScreen.blit(mLeftSlider.render(), (0, 9 * dheight))
    guiScreen.blit(mRightSlider.render(), (100, 9 * dheight))
    guiScreen.blit(leftUpSlider.render(), (200, 9 * dheight))  # blitting thruster values
    guiScreen.blit(rightUpSlider.render(), (300, 9 * dheight))  # blitting thruster values

    guiScreen.blit(temp_display.render(), (0, dheight))  # blitting temperature values# pick a font you have and set its size
    guiScreen.blit(th_up_display.render(), (0, 2 * dheight))
    guiScreen.blit(th_left_display.render(), (0, 3 * dheight))
    guiScreen.blit(th_right_display.render(), (0, 4 * dheight))
    guiScreen.blit(claw_display.render(), (0, 5 * dheight))  # display the claw value on the screen
    guiScreen.blit(rotate_display.render(), (0, 6 * dheight))  # display the claw value on the screen

    # Capture images from the camera (cameras 1 and 2)
    # img1 = cam1.get_image()

    # Rendering camera display images onto the GUI menu
    # cameraSurface.blit(img1, (0, 0))  # surface for camera 1

    # screen.blit(cameraSurface, (460, 0))
    screen.blit(guiScreen, (0, 140))  # all the gui

    screen.blit(leftText, (15, 290))
    screen.blit(rightText, (115, 290))
    screen.blit(leftUpText, (215, 290))
    screen.blit(rightUpText, (315, 290))
    # pygame.display.update()

    pygame.display.flip()  # update screen for all rectangular images
    time.sleep(0.01)