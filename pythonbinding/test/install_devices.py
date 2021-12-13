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

    
def test_discover(my_stack):
  time.sleep(1)
  devices = my_stack.discover_devices()
  if my_stack.get_nr_devices() > 0:
     print ("SN :", my_stack.device_array[0].sn)


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
    install(my_stack)
    
    time.sleep(2)
    my_stack.quit()
    sys.exit()
