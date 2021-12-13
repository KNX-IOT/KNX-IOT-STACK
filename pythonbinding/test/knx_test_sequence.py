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
    response =  my_stack.issue_cbor_get(sn, "/.well-known/knx")
    print ("response:",response)
    my_stack.purge_response(response)
    
    content = { 1 : 5, 2: "reset"}
    response =  my_stack.issue_cbor_post(sn,"/.well-known/knx",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    
def do_sequence_a_sen(my_stack):
    sn = my_stack.device_array[0].sn
    content = {2: "reset"}
    response =  my_stack.issue_cbor_post(sn,"/a/sen",content)
    print ("response:",response)
    my_stack.purge_response(response)
    
    
def do_sequence_f(my_stack):
    sn = my_stack.device_array[0].sn
    response =  my_stack.issue_cbor_get(sn, "/f")
    print ("response:",response)
    #response.payload
    
    my_stack.purge_response(response)


    return
    execute_get("coap://"+my_base+"/f", 40)
    # note this one is a bit dirty hard coded...
    execute_get("coap://"+my_base+"/f/417", 40)
    execute_get("coap://"+my_base+"/.well-known/core", 40)
    

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
        
    

def install(my_base):
    sn = get_sn(my_base)
    print (" SN : ", sn)
    iid = "5"  # installation id
    if "000001" == sn :
       # sensor, e.g sending  
       print ("--------------------")
       print ("Installing SN: ", sn)
       
       content = { 2: "reset"}
       print("reset :", content);
       execute_post("coap://"+my_base+"/.well-known/knx", 60, 60, content)
       
       content = True
       print("set PM :", content);
       execute_put("coap://"+my_base+"/dev/pm", 60, 60, content)
       content = 1
       print("set IA :", content);
       execute_put("coap://"+my_base+"/dev/ia", 60, 60, content)
       content = iid
       execute_put("coap://"+my_base+"/dev/iid", 60, 60, content)
       
       
       content = { 2: "startLoading"}
       print("lsm :", content);
       execute_post("coap://"+my_base+"/a/lsm", 60, 60, content)
       execute_get("coap://"+my_base+"/a/lsm", 60)
       # group object table
       # id (0)= 1
       # url (11)= /p/light
       # ga (7 )= 1
       # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
       content = [ {0: 1, 11: "p/push", 7:[1], 8: [2] } ] 
       execute_post("coap://"+my_base+"/fp/g", 60, 60, content)
       
       execute_get("coap://"+my_base+"/fp/g", 40)
       
       
       # recipient table
       # id (0)= 1
       # ia (12)
       # url (11)= .knx
       # ga (7 )= 1
       # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
       content = [ {0: 1, 11: "/p/push", 7:[1], 12 :"blah.blah" } ] 
       execute_post("coap://"+my_base+"/fp/r", 60, 60, content)
       
       content = False
       print("set PM :", content);
       execute_put("coap://"+my_base+"/dev/pm", 60, 60, content)
       
       content = { 2: "loadComplete"}
       print("lsm :", content);
       execute_post("coap://"+my_base+"/a/lsm", 60, 60, content)
       execute_get("coap://"+my_base+"/a/lsm", 60)
       
       
    if "000002" == sn :
       # actuator ==> receipient
       # should use /fp/r
       print ("--------------------")
       print ("installing SN: ", sn)
       content = True
       print("set PM :", content);
       execute_put("coap://"+my_base+"/dev/pm", 60, 60, content)
       content = 2
       print("set IA :", content);
       execute_put("coap://"+my_base+"/dev/ia", 60, 60, content)
       content = iid
       execute_put("coap://"+my_base+"/dev/iid", 60, 60, content)
       
       
       content = { 2: "startLoading"}
       print("lsm :", content);
       execute_post("coap://"+my_base+"/a/lsm", 60, 60, content)
       execute_get("coap://"+my_base+"/a/lsm", 60)
       
       # group object table
       # id (0)= 1
       # url (11)= /p/light
       # ga (7 )= 1
       # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
       content = [ { 0: 1, 11: "/p/light", 7:[1], 8: [1] } ] 
       execute_post("coap://"+my_base+"/fp/g", 60, 60, content)
       
       execute_get("coap://"+my_base+"/fp/g", 40)
       # publisher table
       # id (0)= 1
       # ia (12)
       # url (11)= .knx
       # ga (7 )= 1
       # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
       content = [ {0: 1, 11: ".knx", 7:[1], 12 :"blah.blah" } ] 
       execute_post("coap://"+my_base+"/fp/p", 60, 60, content)
       
       content = False
       print("set PM :", content);
       execute_put("coap://"+my_base+"/dev/pm", 60, 60, content)
       
       content = { 2: "loadComplete"}
       print("lsm :", content);
       execute_post("coap://"+my_base+"/a/lsm", 60, 60, content)
       execute_get("coap://"+my_base+"/a/lsm", 60)
       
       

    # do a post
    content = {"sia": 5678, "st": 55, "ga": 1, "value": 100 }
    content = { 4: 5678, "st": 55, 7: 1, "value": 100 }
    #                 st       ga       value (1)
    #content = { 5: { 6: 1, 7: 1, 1: True } } 
    #execute_post("coap://"+my_base+"/.knx", 60, 60, content)
    
    content = {4: 5678, 5: { 6: 1, 7: 1, 1: False } } 
    execute_post("coap://"+my_base+"/.knx", 60, 60, content)
    #execute_post("coap://[FF02::FD]:5683/.knx", 60, 60, content)
        
if __name__ == '__main__':  # pragma: no cover

    my_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, my_stack.sig_handler)
    
    test_discover(my_stack)
    do_sequence(my_stack)
    
    time.sleep(2)
    my_stack.quit()
    sys.exit()
