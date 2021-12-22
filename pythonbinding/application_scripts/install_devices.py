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

    
def do_discover(my_stack):
  time.sleep(1)
  devices = my_stack.discover_devices()
  if my_stack.get_nr_devices() > 0:
     print ("SN :", my_stack.device_array[0].sn)


def do_install_device(my_stack, sn, ia, iid, fp_content):
   # sensor, e.g sending  
   print ("--------------------")
   print ("Installing SN: ", sn)
   
   content = { 2: "reset"}
   print("reset :", content);
   response =  my_stack.issue_cbor_post(sn,"/.well-known/knx",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   content = True
   print("set PM :", content);
   response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   
   content = ia
   print("set IA :", content)
   response =  my_stack.issue_cbor_put(sn,"/dev/ia",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   content = iid
   response =  my_stack.issue_cbor_put(sn,"/dev/iid",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   content = { 2: "startLoading"}
   print("lsm :", content);
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
   content = [ {0: 1, 11: "p/push", 7:[1], 8: [2] } ] 
   content = fp_content
   response =  my_stack.issue_cbor_post(sn,"/fp/g",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   response =  my_stack.issue_linkformat_get(sn,"/fp/g")
   print ("response:",response)
   my_stack.purge_response(response)
   
   
   # recipient table
   # id (0)= 1
   # ia (12)
   # url (11)= .knx
   # ga (7 )= 1
   # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
   content = [ {0: 1, 11: "/p/push", 7:[1], 12 :"blah.blah" } ] 
   response =  my_stack.issue_cbor_post(sn,"/fp/r",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   
   content = False
   print("set PM :", content);
   response =  my_stack.issue_cbor_put(sn,"/dev/pm",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   content = { 2: "loadComplete"}
   print("lsm :", content);
   response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
   print ("response:",response)
   my_stack.purge_response(response)
   
   response =  my_stack.issue_cbor_get(sn,"/a/lsm")
   print ("response:",response)
   my_stack.purge_response(response)


def do_install(my_stack):
    sn = my_stack.device_array[0].sn
    print (" SN : ", sn)
    iid = "5"  # installation id
    ia = 0
    fp_content = []
    
    if "000001" == sn :
       ia = 1
       fp_content = [ {0: 1, 11: "p/push", 7:[1], 8: [2] } ] 
    if "000002" == sn :
       ia = 2
       fp_content = [ { 0: 1, 11: "/p/light", 7:[1], 8: [1] } ] 
       
    do_install_device(my_stack, sn, ia, iid, fp_content )
       
        
if __name__ == '__main__':  # pragma: no cover

    my_stack = knx_stack.KNXIOTStack()
    signal.signal(signal.SIGINT, my_stack.sig_handler)
    
    
    try: 
      do_discover(my_stack)
      do_install(my_stack)
    except:
      traceback.print_exc()
     
    time.sleep(2)
    my_stack.quit()
    sys.exit()
