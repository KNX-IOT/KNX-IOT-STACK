#!/usr/bin/env python
#############################
#
#    copyright 2021 Cascoda Ltd.
#    Redistribution and use in source and binary forms, with or without modification,
#    are permitted provided that the following conditions are met:
#    1.  Redistributions of source code must retain the above copyright notice,
#        this list of conditions and the following disclaimer.
#    2.  Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation
#        and/or other materials provided with the distribution.
#
#    THIS SOFTWARE IS PROVIDED "AS IS"
#    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR
#    WARRANTIES OF NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE
#    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
#    OR CONSEQUENTIAL DAMAGES
#    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#    LOSS OF USE, DATA, OR PROFITS;OR BUSINESS INTERRUPTION)
#    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#############################


# pylint: disable=C0103
# pylint: disable=C0114
# pylint: disable=C0116
# pylint: disable=C0209

# pylint: disable=E0401
# pylint: disable=R0402
# pylint: disable=C0411

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
def handle_left(_ch, _evt):
    backlight.left_rgb(*FULL_BL)
    knx.handle_left()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

@touch.on(touch.BUTTON)
def handle_mid(_ch, _evt):
    backlight.mid_rgb(*FULL_BL)
    knx.handle_mid()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

@touch.on(touch.RIGHT)
def handle_right(_ch, _evt):
    backlight.right_rgb(*FULL_BL)
    knx.handle_right()
    sleep(BLINK_TIME_S)
    backlight.rgb(*IDLE_BL)

if __name__ == "__main__":
    init()
    print("Pausing from within Python main")
    signal.pause()
