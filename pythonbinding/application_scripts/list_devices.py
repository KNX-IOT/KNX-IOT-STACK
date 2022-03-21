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
# pylint: disable=C0115
# pylint: disable=C0116
# pylint: disable=C0201
# pylint: disable=C0209
# pylint: disable=C0413
# pylint: disable=R0801
# pylint: disable=R0902
# pylint: disable=R0913
# pylint: disable=R0915
# pylint: disable=R1732
# pylint: disable=W0702
# pylint: disable=W1514

import sys
import time
import traceback
import os
import signal
import argparse

#add parent directory to path so we can import from there
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

import knx_stack

def do_ia_discover(my_stack, internal_address, scope = 2):
    time.sleep(1)
    query = "if=urn:knx:ia."+str(internal_address)
    my_stack.discover_devices_with_query( query, int(scope))
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)


def do_pm_discover(my_stack, scope = 2):
    time.sleep(1)
    query = "if=urn:knx:if.pm"
    my_stack.discover_devices_with_query( query, int(scope))
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

def do_sn_discover(my_stack, serial_number, scope = 2):
    time.sleep(1)
    query = "ep=urn:knx:sn."+str(serial_number)
    my_stack.discover_devices_with_query( query, int(scope))
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

if __name__ == '__main__':  # pragma: no cover

    parser = argparse.ArgumentParser()

    # input (files etc.)
    parser.add_argument("-ia", "--internal_address",
                    help="internal address of the device", nargs='?',
                    const=1, required=False)
    parser.add_argument("-pm", "--programming_mode",
                    help="device in programming mode", nargs='?',
                    const=1, required=False)
    parser.add_argument("-sn", "--serial_number",
                    help="serial number", nargs='?',
                    const=1, required=False)
    parser.add_argument("-scope", "--scope",
                    help="scope of the multicast request [2,5]", nargs='?',
                    default=2, const=1, required=False)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()

    print("scope            :" + str(args.scope))
    print("internal address :" + str(args.internal_address))
    print("programming mode :" + str(args.programming_mode))
    print("serial number    :" + str(args.serial_number))

    the_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, the_stack.sig_handler)

    if args.internal_address:
        try:
            do_ia_discover(the_stack, args.internal_address, args.scope)
        except:
            traceback.print_exc()
    if args.programming_mode:
        try:
            do_pm_discover(the_stack, args.scope)
        except:
            traceback.print_exc()
    if args.serial_number:
        try:
            do_sn_discover(the_stack, args.serial_number, args.scope)
        except:
            traceback.print_exc()

    time.sleep(2)
    the_stack.quit()
    sys.exit()
