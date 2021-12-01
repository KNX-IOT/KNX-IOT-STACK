import dothat.backlight as backlight
import dothat.lcd as lcd

import fcntl
import socket
import struct
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
    lcd.clear()
    # turn off the blinding bright LEDS to the left of the screen
    backlight.graph_off() 
    
    eth0 = get_addr('eth0')
    host = socket.gethostname()

    lcd.set_cursor_position(0,0)
    lcd.write('{}'.format(host))

    lcd.set_cursor_position(0,1)
    if eth0 != 'Not Found!':
        lcd.write(eth0)
    else:
        lcd.write('eth0 {}'.format(eth0))



def print_in_python():
    print("Python binding was succesful!")

def set_backlight(value):
    print("Set backlight to {}".format(value))
    if value == True:
        backlight.rgb(255, 255, 255)
    else:
        backlight.rgb(0, 0, 0)
