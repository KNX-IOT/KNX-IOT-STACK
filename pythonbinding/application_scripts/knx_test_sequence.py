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

import os
import signal
import sys
import time
import traceback

#add parent directory to path so we can import from there
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

#from knx_stack import KNXIOTStack
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

# no json tags as strings
def do_sequence_dev(my_stack):
    sn = my_stack.device_array[0].sn
    print("===================")
    print("Get HWT :")
    response = my_stack.issue_cbor_get(sn, "/dev/hwt")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    #print("Get HWV :");
    #response =  my_stack.issue_cbor_get(sn, "/dev/hwv")
    #print ("response:",response)
    #my_stack.purge_response(response)

    print("===================")
    #print("Get FWV :");
    #response =  my_stack.issue_cbor_get(sn, "/dev/fwv")
    #print ("response:",response)
    #my_stack.purge_response(response)

    print("===================")
    print("Get Model :")
    response =  my_stack.issue_cbor_get(sn,"/dev/model")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = True
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    my_stack.purge_response(response)

    content = False
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = 44
    print("set IA :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = "my host name"
    print("set hostname :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = " iid xxx"
    print("set iid :", content)

    response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/iid")
    print ("response:",response)
    my_stack.purge_response(response)

# no json tags as strings
def do_sequence_dev_programming_mode(my_stack):
    sn = my_stack.device_array[0].sn

    print("===================")
    content = True
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = 44
    print("set IA :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = "my host name"
    print("set hostname :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    my_stack.purge_response(response)

    print("===================")
    content = " iid xxx"
    print("set iid :", content)

    response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/dev/iid")
    print ("response:",response)
    my_stack.purge_response(response)

    # reset programming mode
    content = False
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

# cmd ==> 2
def do_sequence_lsm_int(my_stack):
    print("========lsm=========")
    sn = my_stack.device_array[0].sn

    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

    content = {2 : "startLoading"}
    response =  my_stack.issue_cbor_put(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

    content = {2 : "loadComplete"}
    response =  my_stack.issue_cbor_put(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

    content = {2 : "unload"}
    response =  my_stack.issue_cbor_put(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

# ./knx resource
# sia ==> 4
# ga ==> 7
# st ==> 6
def do_sequence_knx_knx_int(my_stack):
    sn = my_stack.device_array[0].sn
    print("========.knx=========")
    response =  my_stack.issue_cbor_get(sn, "/.knx")
    print ("response:",response)
    my_stack.purge_response(response)

    content = {"value": { 4 : 5, 7: 7777 , 6 : "rp"}}
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn, "/.knx")
    print ("response:",response)
    my_stack.purge_response(response)

def do_sequence_knx_osn(my_stack):
    sn = my_stack.device_array[0].sn
    print("========knx/osn=========")
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx/osn")
    print ("response:",response)
    my_stack.purge_response(response)

def do_sequence_knx_crc(my_stack):
    #  url, content, accept, contents
    sn = my_stack.device_array[0].sn
    print("========knx/crc=========")
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx/crc")
    print ("response:",response)
    my_stack.purge_response(response)

def do_sequence_core_knx(my_stack):
    sn = my_stack.device_array[0].sn
    print("========.well-known/knx=========")
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx")
    print ("response:",response)
    my_stack.purge_response(response)

    content = { 1 : 5, 2: "reset"}
    response =  my_stack.issue_cbor_post(sn,"/.well-known/knx",content)
    print ("response:",response)
    my_stack.purge_response(response)

def do_sequence_a_sen(my_stack):
    sn = my_stack.device_array[0].sn
    print("========/a/sen=========")
    content = {2: "reset"}
    response =  my_stack.issue_cbor_post(sn,"/a/sen",content)
    print ("response:", response)
    my_stack.purge_response(response)

def do_sequence_f(my_stack):
    sn = my_stack.device_array[0].sn
    print("========/f=========")
    response =  my_stack.issue_linkformat_get(sn, "/f")
    print ("response:", response)
    lf = knx_stack.LinkFormat(response.payload)
    print(" lines:", lf.get_nr_lines())
    for line in lf.get_lines():
        print(" -------------------------")
        print(" url :", lf.get_url(line))
        print(" ct  :", lf.get_ct(line))
        print(" ct  :", lf.get_rt(line))
        if lf.get_ct(line) == "40" :
            response2 =  my_stack.issue_cbor_get(sn, lf.get_url(line))
            print ("response2:",response2)
            my_stack.purge_response(response2)

    my_stack.purge_response(response)

def do_sequence(my_stack):
    if my_stack.get_nr_devices() > 0:
        get_sn(my_stack)
        do_sequence_dev(my_stack)
        do_sequence_dev_programming_mode(my_stack)
        do_sequence_lsm_int(my_stack)
        # .knx
        do_sequence_knx_knx_int(my_stack)
        do_sequence_knx_crc(my_stack)
        do_sequence_knx_osn(my_stack)
        do_sequence_core_knx(my_stack)
        do_sequence_a_sen(my_stack)
        do_sequence_f(my_stack)

if __name__ == '__main__':  # pragma: no cover
    the_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, the_stack.sig_handler)

    try:
        test_discover(the_stack)
        do_sequence(the_stack)
    except:
        traceback.print_exc()

    time.sleep(2)
    the_stack.quit()
    sys.exit()
