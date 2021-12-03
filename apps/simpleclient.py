import signal

import dothat.backlight as backlight
import dothat.lcd as lcd
import dothat.touch as touch

import fcntl
import socket
import struct

import knx

from time import sleep

IDLE_BL = (128, 128, 128)
DARK_BL = (0, 0, 0)
FULL_BL = (0, 255, 0)
BLINK_TIME_S = 0.03

def get_addr(ifname):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15].encode('utf-8'))
        )[20:24])
    except IOError:
        return 'Not Found!'

def init():
    backlight.rgb(*IDLE_BL)
    lcd.clear()

    eth0 = get_addr('eth0')
    host = socket.gethostname()

    lcd.set_cursor_position(0,0)
    lcd.write('{}'.format(host))

    lcd.set_cursor_position(0,1)
    if eth0 != 'Not Found!':
        lcd.write(eth0)
    else:
        lcd.write('eth0 {}'.format(eth0))

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