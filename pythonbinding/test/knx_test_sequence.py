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


#add parent directory to path so we can import from there
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

#from knx_stack import KNXIOTStack
import knx_stack


######## old ###################

def do_sequence_knx_spake(my_base):

    #  url, content, accept, contents
    
    # sequence:
    # - parameter exchange: 15 (rnd)- return value
    # - credential exchange: 10 - return value
    # - pase verification exchange: 14  - no return value
    
    content = { 15: b"a-15-sdfsdred"}
    execute_post("coap://"+my_base+"/.well-known/knx/spake", 60, 60, content)
    
    # pa
    content = { 10: b"s10dfsdfsfs" }
    execute_post("coap://"+my_base+"/.well-known/knx/spake", 60, 60, content)
    
    # ca
    content = { 14: b"a15sdfsdred"}
    execute_post("coap://"+my_base+"/.well-known/knx/spake", 60, 60, content)
    # expecting return 
    

def do_sequence_knx_idevid(my_base):
    #  url, content, accept, contents
    execute_get("coap://"+my_base+"/.well-known/knx/idevid", 282)
    
def do_sequence_knx_ldevid(my_base):
    #  url, content, accept, contents
    execute_get("coap://"+my_base+"/.well-known/knx/ldevid", 282)


def do_sequence_oscore(my_base):
    #  url, content, accept, contents
    execute_get("coap://"+my_base+"/f/oscore", 40)
    
    execute_get("coap://"+my_base+"/p/oscore/replwdo", 60)
    content = 105 
    execute_put("coap://"+my_base+"/p/oscore/replwdo", 60, 60, content)
    execute_get("coap://"+my_base+"/p/oscore/replwdo", 60)
    
    execute_get("coap://"+my_base+"/p/oscore/osndelay", 60)
    content =  1050
    execute_put("coap://"+my_base+"/p/oscore/osndelay", 60, 60, content)
    execute_get("coap://"+my_base+"/p/oscore/osndelay", 60)
    
def do_sequence_auth(my_base):
    #  url, content, accept, contents
    
    execute_get("coap://"+my_base+"/auth", 40)
    
    
def do_sequence_auth_at(my_base):
    #  url, content, accept, contents
    
    execute_get("coap://"+my_base+"/auth/at", 40)
    # 
    content = {0: b"id", 1 : 20, 2:b"ms",3:"hkdf", 4:"alg", 5:b"salt", 6:b"contextId"}
    execute_post("coap://"+my_base+"/auth/at", 60, 60, content)
    
    
    content = {0: b"id2", 1 : 20, 2:b"ms",3:"hkdf", 4:"alg", 5:b"salt", 6:b"contextId2"}
    execute_post("coap://"+my_base+"/auth/at", 60, 60, content)
    
    execute_get("coap://"+my_base+"/auth/at", 40)
    
    execute_get("coap://"+my_base+"/auth/at/id", 60)
    execute_del("coap://"+my_base+"/auth/at/id", 60)

######## working ###################

    
def test_discover(my_stack):
  time.sleep(1)
  devices = my_stack.discover_devices()
  if my_stack.get_nr_devices() > 0:
     print ("SN :", my_stack.device_array[0].sn)

def get_sn(my_stack):
    print("Get SN :");
    sn = my_stack.device_array[0].sn
    response = my_stack.issue_cbor_get(sn, "/dev/sn")
    print ("response:",response)
    my_stack.purge_response(response)

# no json tags as strings
def do_sequence_dev(my_stack):
    sn = my_stack.device_array[0].sn
    print("===================")
    print("Get HWT :");
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
    print("Get Model :");
    response =  my_stack.issue_cbor_get(sn,"/dev/model")
    print ("response:",response)
    my_stack.purge_response(response)
    

    print("===================")
    content = True
    print("set PM :", content);
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
    print("set IA :", content);
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    my_stack.purge_response(response)
    
    print("===================")
    content = "my host name"
    print("set hostname :", content);
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    my_stack.purge_response(response)
    
    print("===================")
    content = " iid xxx"
    print("set iid :", content);
    
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
    print("set PM :", content);
    response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    response =  my_stack.issue_cbor_get(sn,"/dev/pm")
    print ("response:",response)
    my_stack.purge_response(response)
    
    print("===================")
    content = 44
    print("set IA :", content);
    response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    response =  my_stack.issue_cbor_get(sn,"/dev/ia")
    print ("response:",response)
    my_stack.purge_response(response)
    
    print("===================")
    content = "my host name"
    print("set hostname :", content);
    response =  my_stack.issue_cbor_put(sn,"/dev/hostname",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    response =  my_stack.issue_cbor_get(sn,"/dev/hostname")
    print ("response:",response)
    my_stack.purge_response(response)
    
    print("===================")
    content = " iid xxx"
    print("set iid :", content);
    
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
# st 6 
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
    
    sn = get_sn(my_stack)
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

    my_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, my_stack.sig_handler)
    
    try: 
      test_discover(my_stack)
      do_sequence(my_stack)
    except:
      traceback.print_exc()
    
    time.sleep(2)
    my_stack.quit()
    sys.exit()
