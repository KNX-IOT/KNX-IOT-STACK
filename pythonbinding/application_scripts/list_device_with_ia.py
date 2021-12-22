#!/usr/bin/env python
#############################
#
#    copyright 2021 Cascoda Ltd.
#    Redistribution and use in source and binary forms, with or without modification,
#    are permitted provided that the following conditions are met:
#    1.  Redistributions of source code must retain the above copyright notice,
#        this list of conditions and the following disclaimer.
#    2.  Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation and/or other materials provided
#        with the distribution.
#
#    THIS SOFTWARE IS PROVIDED BY THE OPEN CONNECTIVITY FORUM, INC. "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
#    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR
#    WARRANTIES OF NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE OPEN CONNECTIVITY FORUM, INC. OR
#    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#    OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#############################

import getopt
import socket
import sys
import cbor
import json
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

    
def do_discover(my_stack, internal_address, scope = 2):
  time.sleep(1)
  query = "if=urn:knx:ia."+str(internal_address)
  devices = my_stack.discover_devices_with_query( query, int(scope))
  if my_stack.get_nr_devices() > 0:
     print ("SN :", my_stack.device_array[0].sn)

def get_sn(my_stack):
    print("Get SN :");
    sn = my_stack.device_array[0].sn
    response = my_stack.issue_cbor_get(sn, "/dev/sn")
    print ("response:",response)
    my_stack.purge_response(response)

    
def do_reset(my_stack):
    if my_stack.get_nr_devices() > 0:
        sn = my_stack.device_array[0].sn
        content = {2: "reset"}
        response =  my_stack.issue_cbor_post(sn,"/a/sen",content)
        print ("response:",response)
        my_stack.purge_response(response)
    

if __name__ == '__main__':  # pragma: no cover

    parser = argparse.ArgumentParser()

    # input (files etc.)
    parser.add_argument("-ia", "--internal_address",
                    help="internal address of the device", nargs='?', const=1, required=True)
    parser.add_argument("-scope", "--scope",
                    help="scope of the multicast request [2,5]", nargs='?', default=2, const=1, required=False)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()

    print("scope            :" + str(args.scope))
    print("internal address :" + str(args.internal_address))

    my_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, my_stack.sig_handler)
    
    try:
      do_discover(my_stack, args.internal_address, args.scope)
      do_reset(my_stack)
    except:
      traceback.print_exc()
      
    time.sleep(2)
    my_stack.quit()
    sys.exit()
