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


# pylint: disable=C0114
# pylint: disable=C0116
# pylint: disable=C0209

# pylint: disable=E0401
# pylint: disable=R0402
# pylint: disable=C0411

import dothat.backlight as backlight
import dothat.lcd as lcd

import fcntl
import socket
import struct

def get_addr(ifname):
    try:
        my_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            my_socket.fileno(),
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
    if value is True:
        backlight.rgb(255, 255, 255)
    else:
        backlight.rgb(0, 0, 0)
