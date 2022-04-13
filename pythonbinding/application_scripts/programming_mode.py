#!/usr/bin/env python
#############################
#
#    copyright 2021-2022 Cascoda Ltd.
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

def safe_print(response):
    if response is not None:
        response.print_payload()
    else:
        print("no response")

def do_discover(my_stack, serial_number, scope = 2):
    time.sleep(1)
    query = "ep=urn:knx:sn."+str(serial_number)
    my_stack.discover_devices_with_query( query, int(scope))
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

def do_programming_mode(my_stack, pm_value):
    print("Get SN :")
    if my_stack.get_nr_devices() == 0:
        return -1
    sn = my_stack.device_array[0].sn
    response = my_stack.issue_cbor_get(sn, "/dev/pm")
    print ("current value response:",response)
    if response is None:
        return 1
    if response.status != 0:
        print("ERROR {} {}".format(response.status,
            my_stack.get_error_string_from_code(response.status)))
        return 2
    my_stack.purge_response(response)
    pm_val = False
    if pm_value is True:
        pm_val = True
    content = { 1 : pm_val}
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    safe_print(response)
    my_stack.purge_response(response)
    return 0

def self_reset(my_stack):
    """
    reset myself
    """
    my_stack.reset_myself()

def do_spake(my_stack, password):
    """
    do spake handshake
    """
    if my_stack.get_nr_devices() > 0:
        sn = my_stack.device_array[0].sn
        print("========spake=========", sn)
        my_stack.initiate_spake(sn, password, sn)

if __name__ == '__main__':  # pragma: no cover

    parser = argparse.ArgumentParser()
    # input (files etc.)
    parser.add_argument("-scope", "--scope", default=2,
                    help="scope of the multicast request (defaul =2 : same machine)", nargs='?',
                    const="", required=False)
    parser.add_argument("-sn", "--serial_number",
                    help="serial number", nargs='?',
                    const=1, required=True)
    parser.add_argument("-pm", "--programming_mode", default="True",
                    help="set the programming mode (default True) e.g. -pm False", nargs='?',
                    const="true", required=False)
    parser.add_argument("-password", "--password", default="LETTUCE",
                    help="password default:LETTUCE", nargs='?',
                    const="true", required=False)
    parser.add_argument("-reset", "--reset",default=False,
                    help="reset myself", nargs='?',
                    const="true", required=False)
    parser.add_argument("-wait", "--wait",
                    help="wait after issuing the command", nargs='?',
                    default=2, const=1, required=False)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()

    print("scope            :" + str(args.scope))
    print("serial_number    :" + str(args.serial_number))
    print("programming_mode :" + str(args.programming_mode))
    print("password         :" + str(args.password))
    print("reset myself     :" + str(args.reset))
    print("wait [sec]       :" + str(args.wait))

    value = False
    if  str(args.programming_mode) == "True":
        value = True

    the_stack = knx_stack.KNXIOTStack()
    the_stack.start_thread()
    signal.signal(signal.SIGINT, the_stack.sig_handler)
    time.sleep(2)
    try:
        if args.reset:
            self_reset(the_stack)
            time.sleep(1)
        do_discover(the_stack, args.serial_number, args.scope)
        time.sleep(1)
        ret = 1
        #ret = do_programming_mode(the_stack, value)
        if ret > 0:
            do_spake(the_stack, str(args.password))
            time.sleep(5)
            ret = do_programming_mode(the_stack, value)
    except:
        traceback.print_exc()

    time.sleep(int(args.wait))
    the_stack.quit()
    sys.exit()
