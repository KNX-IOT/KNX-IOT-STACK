import dothat.backlight as backlight
import dothat.lcd as lcd

lcd.clear()
# turn off the blinding bright LEDS to the left of the screen
backlight.graph_off() 


def print_in_python():
    print("Python binding was succesful!")

def set_backlight(value):
    print("Set backlight to {}".format(value))
    if value == True:
        backlight.rgb(255, 255, 255)
    else:
        backlight.rgb(0, 0, 0)
