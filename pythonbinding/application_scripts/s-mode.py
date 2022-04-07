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

def test_discover(my_stack):
    time.sleep(1)
    my_stack.discover_devices()
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

def get_sn(my_stack):
    print("Get SN :")
    sn = my_stack.device_array[0].sn
    response = my_stack.issue_cbor_get(sn, "/dev/sn")
    print ("response:",response)
    my_stack.purge_response(response)

def do_reset(my_stack):
    sn = my_stack.device_array[0].sn
    content = {2: "reset"}
    response =  my_stack.issue_cbor_post(sn,"/a/sen",content)
    print ("response:",response)
    my_stack.purge_response(response)

#
#
# {sia: 5678, es: {st: write, ga: 1, value: 100 }}
#
#
if __name__ == '__main__':  # pragma: no cover

    parser = argparse.ArgumentParser()
    # input (files etc.)
    parser.add_argument("-scope", "--scope", default=2,
                    help="scope of the multicast request (defaul =2 : same machine)", nargs='?',
                    const="", required=False)
    parser.add_argument("-sia", "--sia", default=2,
                    help="sending internal address (default ia=2)", nargs='?',
                    const="", required=False)
    parser.add_argument("-iid", "--iid", default=5,
                    help="sending installation identifier (default iid=5)", nargs='?',
                    const="", required=False)
    parser.add_argument("-st", "--st", default="w",
                    help="send target ,e.g w,r,rp", nargs='?',
                    const="", required=False)
    parser.add_argument("-ga", "--ga", default=1,
                    help="group address (default = 1)", nargs='?', const="",
                    required=False)
    parser.add_argument("-valuetype", "--valuetype", default=0,
                    help="0=boolean, 1=integer, 2=float (default boolean)", nargs='?',
                    const="", required=False)
    parser.add_argument("-value", "--value", default=True,
                    help="value of the valuetype (default True)", nargs='?',
                    const="true", required=False)
    parser.add_argument("-wait", "--wait",
                    help="wait after issuing s-mode command", nargs='?',
                    default=2, const=1, required=False)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()

    print("scope      :" + str(args.scope))
    print("sia        :" + str(args.sia))
    print("st         :" + str(args.st))
    print("ga         :" + str(args.ga))
    print("iid        :" + str(args.iid))
    print("valuetype  :" + str(args.valuetype))
    print("value      :" + str(args.value))
    print("wait [sec] :" + str(args.wait))

    the_stack = knx_stack.KNXIOTStack()
    the_stack.start_thread()

    signal.signal(signal.SIGINT, the_stack.sig_handler)
    time.sleep(2)

    try:
        the_stack.issue_s_mode(args.scope, args.sia, args.ga, args.iid,
           str(args.st), args.valuetype, str(args.value))
    except:
        traceback.print_exc()

    time.sleep(int(args.wait))
    the_stack.quit()
    sys.exit()
