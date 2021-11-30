import signal

import dothat.backlight as backlight
import dothat.lcd as lcd
import dothat.touch as touch

IDLE_BL = (128, 128, 128)
FULL_BL = (255, 255, 255)

def init():
    backlight.rgb(*IDLE_BL)
    lcd.clear()
    lcd.set_cursor_position(0, 0)
    lcd.write("KNX Client")

@touch.on(touch.LEFT)
def handle_left():
    backlight.left_rgb(*FULL_BL)
    # knx_handle_left()
    backlight.left_rgb(*IDLE_BL)

@touch.on(touch.BUTTON)
def handle_mid():
    backlight.mid_rgb(*FULL_BL)
    # knx_handle_mid()
    backlight.mid_rgb(*IDLE_BL)

@touch.on(touch.RIGHT)
def handle_right():
    backlight.right_rgb(*FULL_BL)
    # knx_handle_right()
    backlight.right_rgb(*IDLE_BL)

if __name__ == "__main__":
    init()
    signal.pause()