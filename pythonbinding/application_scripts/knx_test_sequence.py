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
# pylint: disable=W0212

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

def print_fail(msg = None):
    print(f"Failure {sys._getframe().f_back.f_lineno}: {msg if msg is not None else ''}")

def compare_dict(dict1, dict2):
    dict1_set = set(dict1.keys())
    dict2_set = set(dict2.keys())
    # compare the keys
    diff1 = dict1_set.difference(dict2_set)
    if len(diff1) > 0:
        print("compare_dict diff1:", diff1)
        return False
    diff2 = dict2_set.difference(dict1_set)
    if len(diff2) > 0:
        print("compare_dict diff2:", diff2)
        return False
    # compare the values
    dictv1_set = set(dict1.values())
    dictv2_set = set(dict2.values())
    # compare the keys
    diffv1 = dictv1_set.difference(dictv2_set)
    if len(diffv1) > 0:
        print("compare_dict diffv1:", diffv1)
        return False
    diffv2 = dictv2_set.difference(dictv1_set)
    if len(diffv2) > 0:
        print("compare_dict difvf2:", diffv2)
        return False
    return True

def test_discover(my_stack):
    time.sleep(1)
    my_stack.discover_devices()
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

def get_sn(my_stack):
    print("Get SN :")
    sn = my_stack.device_array[0].sn
    response = my_stack.issue_cbor_get(sn, "/dev/sn")
    print ("response:", response)
    response.print_payload()
    my_stack.purge_response(response)

# no json tags as strings
def do_sequence_dev(my_stack):
    sn = my_stack.device_array[0].sn
    print("========dev get=========")
    print("Get HWT :")
    response = my_stack.issue_cbor_get(sn, "/dev/hwt")
    print ("response:",response)
    response.print_payload()
    my_stack.purge_response(response)

    #print("===================")
    #print("Get HWV :");
    #response =  my_stack.issue_cbor_get(sn, "/dev/hwv")
    #print ("response:",response)
    #my_stack.purge_response(response)

    #print("===================")
    #print("Get FWV :");
    #response =  my_stack.issue_cbor_get(sn, "/dev/fwv")
    #print ("response:",response)
    #my_stack.purge_response(response)

    print("===================")
    print("-------------------")
    print("Get Model :")
    response =  my_stack.issue_cbor_get(sn,"/dev/model")
    print ("response:",response)
    response.print_payload()
    my_stack.purge_response(response)

    print("-------------------")
    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    #print ("response:",response)
    response.print_payload()
    my_stack.purge_response(response)

    print("-------------------")
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    #print ("response:",response)
    response.print_payload()
    my_stack.purge_response(response)

    print("-------------------")
    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)#
    my_stack.purge_response(response)

    print("-------------------")
    response =  my_stack.issue_cbor_get(sn,"/dev/iid")
    print ("response:",response)#
    my_stack.purge_response(response)

# no json tags as strings
def do_sequence_dev_programming_mode(my_stack):
    sn = my_stack.device_array[0].sn
    print("========dev programming=========")

    print("-------------------")
    content = True
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    response.print_payload()
    if content == response.get_payload_boolean():
        print("PASS : /dev/pm ", content)
    my_stack.purge_response(response)

    print("-------------------")
    content = 44
    print("set IA :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    response.print_payload()
    if content == response.get_payload_int():
        print("PASS : /dev/ia ", content)
    else:
        print_fail(msg="/dev/ia")
    my_stack.purge_response(response)

    print("-------------------")
    content = "my host name"
    print("set hostname :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    if content == response.get_payload_string():
        print("PASS : /dev/hostname ", content)
    else:
        print_fail(msg="/dev/hostname")
    my_stack.purge_response(response)

    print("-------------------")
    content = " iid xxx"
    print("set iid :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/iid")
    print ("response:",response)
    if content == response.get_payload_string():
        print("PASS : /dev/iid ", content)
    else:
        print_fail(msg="/dev/iid")
    my_stack.purge_response(response)
    print("-------------------")

    # reset programming mode
    content = False
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    response.print_payload()
    if content == response.get_payload_boolean():
        print("PASS : /dev/pm ", content)
    else:
        print_fail(msg="/dev/pm")
    my_stack.purge_response(response)
    print("-------------------")

# no json tags as strings
def do_sequence_dev_programming_mode_fail(my_stack):
    sn = my_stack.device_array[0].sn
    print("========dev programming fail=========")

    print("-------------------")
    content = False
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    response.print_payload()
    if content == response.get_payload_boolean():
        print("PASS : /dev/pm ", content)
    else:
        print_fail(msg="/dev/pm")
    my_stack.purge_response(response)

    print("-------------------")
    content = 4444
    print("set IA :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    response.print_payload()
    if content != response.get_payload_int():
        print("PASS : /dev/ia ", content, " != ", response.get_payload_int())
    else:
        print_fail(msg="/dev/ia")
    my_stack.purge_response(response)

    print("-------------------")
    content = "my new host name"
    print("set hostname :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    if content != response.get_payload_string():
        print("PASS : /dev/hostname ", content, " != ", response.get_payload_string())
    else:
        print_fail(msg="/dev/hostname")
    my_stack.purge_response(response)

    print("-------------------")
    content = " xxx iid xxx"
    print("set iid :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/dev/iid")
    print ("response:",response)
    if content != response.get_payload_string():
        print("PASS : /dev/iid ", content, " != ", response.get_payload_string())
    else:
        print_fail(msg="/dev/iid")
    my_stack.purge_response(response)
    print("-------------------")


# cmd ==> 2
def do_sequence_lsm(my_stack):
    print("========lsm=========")
    sn = my_stack.device_array[0].sn

    print("-------------------")
    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

    print("-------------------")
    content = {2 : "startLoading"}
    expect = { "3": "loading"}
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    if compare_dict(expect, response.get_payload_dict()):
        print("PASS : /a/lsm ", expect, response.get_payload_dict())
    else:
        print_fail(msg="/a/lsm")
    my_stack.purge_response(response)

    print("-------------------")
    content = {2 : "loadComplete"}
    expect  = { "3": "loaded"}
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    if compare_dict(expect, response.get_payload_dict()):
        print("PASS : /a/lsm ", expect, response.get_payload_dict())
    else:
        print_fail(msg="/a/lsm")
    my_stack.purge_response(response)

    print("-------------------")
    content = {2 : "unload"}
    expect  = { "3": "unloaded"}
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn, "/a/lsm")
    print ("response:",response)
    if compare_dict(expect, response.get_payload_dict()):
        print("PASS : /a/lsm ", expect, response.get_payload_dict())
    else:
        print_fail(msg="/a/lsm")
    my_stack.purge_response(response)
    print("-------------------")

def do_check_table(my_stack, sn, table_url, content):
    response =  my_stack.issue_cbor_post(sn,table_url,content)
    print ("response:",response.get_payload())
    my_stack.purge_response(response)

    response =  my_stack.issue_linkformat_get(sn,table_url)
    print ("response:",response)
    lf = knx_stack.LinkFormat(response.payload)
    print(" lines:", lf.get_nr_lines())
    # list the response
    for line in lf.get_lines():
        print(" -get------------------------", line)
        print(" url :", lf.get_url(line))
        print(" ct  :", lf.get_ct(line))
        print(" rt  :", lf.get_rt(line))
        # print the cbor data from the entry
        if lf.get_ct(line) == "60" :
            response2 =  my_stack.issue_cbor_get(sn, lf.get_url(line))
            response2.print_payload()
            print ("response2:",response2)
            my_stack.purge_response(response2)
    # delete the entries
    for line in lf.get_lines():
        if lf.get_ct(line) == "60" :
            print(" -delete------------------------", line)
            response2 =  my_stack.issue_cbor_delete(sn, lf.get_url(line))
            response2.print_payload()
            print ("response2:",response2)
            my_stack.purge_response(response2)
    my_stack.purge_response(response)

    response =  my_stack.issue_linkformat_get(sn,table_url)
    print ("response:",response)
    lf = knx_stack.LinkFormat(response.payload)
    print(" lines:", lf.get_nr_lines())
    for line in lf.get_lines():
        print(" line :", line)
    if lf.get_nr_lines() == 0:
        print("PASS : (after delete)", table_url)
    else:
        message = table_url + " delete failed"
        print_fail(msg=message)

# cmd ==> 2
def do_sequence_fp_programming(my_stack):
    print("========fp programming=========")
    sn = my_stack.device_array[0].sn

    print("-------------------")
    content = { 2: "reset"}
    print("reset :", content)
    response =  my_stack.issue_cbor_post(sn,"/.well-known/knx",content)
    print ("response:",response)
    my_stack.purge_response(response)

    print("-------------------")
    content = True
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    print("-------------------")
    content = 455
    print("set IA :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)

    print("-------------------")
    content = 555
    response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
    print ("response:",response)
    my_stack.purge_response(response)

    print("-------------------")
    content = { 2: "startLoading"}
    print("lsm :", content)
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/a/lsm")
    print ("response:",response)
    my_stack.purge_response(response)

    # group object table
    # id (0)= 1
    # url (11)= /p/light
    # ga (7 )= 1
    # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
    content = [ { 0: 1, 11: "p/push", 7:[1], 8: [2] } , {0: 2, 11: "p/light", 7:[2], 8: [2,4] }]
    do_check_table(my_stack, sn, "/fp/g",content)

    content = [ {0: 1, 11: "/p/push", 7:[1], 12 :"blah.blah" },
                {0: 5, 11: "/p/pushxx", 7:[1], 12 :"ss.blah.blah" } ]
    do_check_table(my_stack, sn, "/fp/r",content)

    content = [ {0: 1, 11: "/p/pushpp", 7:[1], 12 :"blah.blahxx" },
                {0: 5, 11: "/p/pushxx", 7:[1], 12 :"ss.blah.blah" } ]
    do_check_table(my_stack, sn, "/fp/p",content)

    content = False
    print("set PM :", content)
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)

    content = { 2: "loadComplete"}
    print("lsm :", content)
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    print ("response:", response)
    my_stack.purge_response(response)

    response =  my_stack.issue_cbor_get(sn,"/a/lsm")
    print ("response:", response)
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
    print("response:",response)
    print("   value:", response.get_payload_int())
    my_stack.purge_response(response)

def do_sequence_knx_crc(my_stack):
    #  url, content, accept, contents
    sn = my_stack.device_array[0].sn
    print("========knx/crc=========")
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx/crc")
    print ("response:",response)
    print("   value:", response.get_payload_int())
    my_stack.purge_response(response)

def do_sequence_core_knx(my_stack):
    sn = my_stack.device_array[0].sn
    print("=get=======.well-known/knx=========")
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx")
    print ("response:",response)
    print ("    value:", response.get_payload_dict())
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
        print(" rt  :", lf.get_rt(line))
        if lf.get_ct(line) == "40" :
            response2 =  my_stack.issue_linkformat_get(sn, lf.get_url(line))
            print ("response2:",response2)
            my_stack.purge_response(response2)
    my_stack.purge_response(response)


def do_discovery_tests(my_stack):
    print("========discovery=========")
    # all devices
    data = my_stack.discover_devices_with_query_data("rt=urn:knx:dpa.*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with 353
    data = my_stack.discover_devices_with_query_data("rt=urn:knx:dpa.353*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with 352
    data = my_stack.discover_devices_with_query_data("rt=urn:knx:dpa.352*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with ia = 5
    data = my_stack.discover_devices_with_query_data("if=urn:knx:ia.5")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with all interfaces
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with sensor interfaces
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.s")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices with actuator interfaces
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.a")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # devices programming mode filtering
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.pm")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # compound filtering actuator datapoint
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.a&rt=urn:knx:dpa.353*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # compound filtering sensor and data point
    data = my_stack.discover_devices_with_query_data("if=urn:knx:if.s&rt=urn:knx:dpa.352*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # serial number wild card
    data = my_stack.discover_devices_with_query_data("ep=urn:knx:sn.*")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    # serial specific value
    data = my_stack.discover_devices_with_query_data("ep=urn:knx:sn.012346")
    print(" -------------------------")
    print (data)
    print(" -------------------------")
    

def do_sequence(my_stack):
    if my_stack.get_nr_devices() > 0:
        get_sn(my_stack)
        do_sequence_dev(my_stack)
        do_sequence_dev_programming_mode(my_stack)
        do_sequence_dev_programming_mode_fail(my_stack)

        do_sequence_f(my_stack)
        do_sequence_lsm(my_stack)
        do_sequence_fp_programming(my_stack)
        
        do_sequence_knx_crc(my_stack)
        do_sequence_knx_osn(my_stack)
        do_sequence_core_knx(my_stack)
        do_discovery_tests(my_stack)
        return
        # .knx
        #do_sequence_knx_knx_int(my_stack)
        #do_sequence_a_sen(my_stack)

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
