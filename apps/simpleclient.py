import signal

import dothat.backlight as backlight
import dothat.lcd as lcd
import dothat.touch as touch

import knx

from time import sleep

IDLE_BL = (128, 128, 128)
DARK_BL = (0, 0, 0)
FULL_BL = (0, 255, 0)
BLINK_TIME_S = 0.03

def init():
    backlight.rgb(*IDLE_BL)
    lcd.clear()
    lcd.set_cursor_position(0, 0)
    lcd.write("KNX Client")

@touch.on(touch.LEFT)
def handle_left(ch, evt):
    backlight.left_rgb(*FULL_BL)
    knx.handle_left()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

@touch.on(touch.BUTTON)
def handle_mid(ch, evt):
    backlight.mid_rgb(*FULL_BL)
    knx.handle_mid()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

@touch.on(touch.RIGHT)
def handle_right(ch, evt):
    backlight.right_rgb(*FULL_BL)
    knx.handle_right()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

if __name__ == "__main__":
    init()
    print("Pausing from within Python main")
    signal.pause()