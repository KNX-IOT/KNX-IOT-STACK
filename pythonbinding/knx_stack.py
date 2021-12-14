#############################
#
#    copyright 2021 Open Connectivity Forum, Inc. All rights reserved.
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


#sudo apt-get -y install python3-pip
#sudo pip3 install numpy
# 
#

import ctypes, os, sys
from ctypes import *

import signal

import time
import os
import json
import random
import sys
import argparse
import traceback
from datetime import datetime
from time import gmtime, strftime
from sys import exit
#import jsonref
import os.path
from os import listdir
from os.path import isfile, join
from shutil import copyfile
from  collections import OrderedDict
from termcolor import colored

import cbor

import numpy.ctypeslib as ctl
#import ctypes

import threading
import time

import json

import requests

import copy

unowned_return_list=[]

discover_event = threading.Event()
#owned_event = threading.Event()
#resource_event = threading.Event()
#diplomat_event = threading.Event()
#so_event = threading.Event()
client_event = threading.Event()
client_mutex = threading.Lock()
#device_event = threading.Event()
resource_mutex = threading.Lock()

ten_spaces = "          "

_int_types = (c_int16, c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (c_int64,)
for t in _int_types:
    if sizeof(t) == sizeof(c_size_t):
        c_ptrdiff_t = t
del t
del _int_types


class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __cmp__(self, string):
        if isinstance(string, UserString):
            return cmp(self.data, string.data)
        else:
            return cmp(self.data, string)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, Union):

    _fields_ = [("raw", POINTER(c_char)), ("data", c_char_p)]

    def __init__(self, obj=""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(POINTER(c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, c_char_p):
            return obj

        # Convert from POINTER(c_char)
        elif isinstance(obj, POINTER(c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(cast(obj, POINTER(c_char)))

        # Convert from c_char array
        elif isinstance(obj, c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)



@CFUNCTYPE(c_int)
def init_callback():
    print("init_callback")
    return c_int(0)


@CFUNCTYPE(None)
def signal_event_loop():
    print("signal_event_loop")
    

CHANGED_CALLBACK = CFUNCTYPE(None, c_char_p, c_char_p, c_char_p)
RESOURCE_CALLBACK = CFUNCTYPE(None, c_char_p, c_char_p, c_char_p, c_char_p)

#typedef void (*clientCB)(char *sn, char* r_format, char *r_id, char *url, int payload_size, char *payload);
CLIENT_CALLBACK = CFUNCTYPE(None, c_char_p, c_char_p, c_char_p, c_char_p, c_int, c_char_p)


#----------LinkFormat parsing ---------------

class LinkFormat():

    def __init__(self, response):
        self.response = response
        self.lines = response.splitlines() 

    def get_nr_lines(self):
        return len(self.lines)
    
    def get_line(self, index):
        return self.lines[index]
        #return len(self.lines)

    def get_lines(self):
        return self.lines

    def get_url(self, line):
        data = line.split(">")
        url = data[0]
        return url[1:]

    def get_ct(self, line):
        tagvalues = line.split(";")
        for tag in tagvalues:
           if tag.startswith("ct"):
              ct_value_all = tag.split("=")
              ct_value = ct_value_all[1].split(",")
              return ct_value[0]
        return ""

    def get_rt(self, line):
        tagvalues = line.split(";")
        for tag in tagvalues:
           if tag.startswith("rt"):
              ct_value_all = tag.split("=")
              ct_value = ct_value_all[1].split(",")
              return ct_value[0]
        return ""

    def get_base(self, url):
        # python3 knxcoapclient.py -o GET -p coap://[fe80::6513:3050:71a7:5b98]:63914/a -c 50
        my_url = url.replace("coap://","")
        mybase = my_url.split("/")
        return mybase[0]

    def get_base_from_link(self, payload):
        print("get_base_from_link\n")
        global paths
        global paths_extend
        lines = payload.splitlines()
    
        # add the 
        if len(paths) == 0:
            my_base = get_base(get_url(lines[0]))
            return my_base

#----------Devices ---------------

class Device():

    def __init__(self, sn, ip_address=None, name="", resources=None, resource_array=None, credentials=None, last_event=None):
      self.sn = sn
      self.ip_address = ip_address
      #self.owned_state = owned_state
      self.name = name 
      self.credentials = credentials
      self.resource_array = []
      self.last_event = last_event 
        
    def __str__(self):
      return ("Device Sn:{} r_id:{}".format(self.sn))


#----------Responses ---------------

class CoAPResponse():
            
    def __init__(self, sn, format, r_id, url, payload_size, payload):
      self.sn = sn
      self.r_id = r_id
      self.url = url
      self.payload_type = format
      self.payload_size = payload_size
      self.payload = payload

    def __str__(self):
      return ("Payload Sn:{} r_id:{}".format(self.sn ,self.r_id))

#----------The Stack ---------------

class KNXIOTStack():
    """ ********************************
    Call back handles general task like device 
    discovery. 
    needs to be before _init_
    **********************************"""

    def convertcbor2json(self, payload, payload_len):
       print (type(payload))
       print (" payload len :", payload_len)
       print (" payload size:", len(payload))
       if (len(payload) > payload_len):
        print ("truncating payload")
        payload = payload[:payload_len]
       print ("=========")
       print (payload)
       print ("=========")
            #json_data = loads(response.payload)
            #print(json_data)
            #print ("=========")
       json_string = ""
       try:
            json_data = cbor.loads(payload)
            json_string = json.dumps(json_data, indent=2, sort_keys=True)
            return json_string
       except:
            print("error in cbor..")
            print (json_string)
            print ("===+++===")
       return ""


    def changedCB(self, sn, cb_state, cb_event):
        print("Changed event: Device: {}, State:{} Event:{}".format(sn, cb_state, cb_event))
        name = ""
        if sn != None:
            sn = sn.decode("utf-8")
            #name = self.get_device_name(uuid)
        if cb_state !=  None:
            cb_state = cb_state.decode("utf-8")
        if cb_event !=  None:
            cb_event = cb_event.decode("utf-8")
        if(cb_event=="discovered"):
            print("Discovery Event:{}".format(sn))
            print("Discovery Event:{}".format(cb_state))
            dev = Device(sn , ip_address=cb_state, name=name)
            self.device_array.append(dev)
            discover_event.set()
    
    """ ********************************
    Call back handles client command callbacks.
    Client discovery/state
    **********************************"""
    def clientCB(self, cb_sn, cb_format, cb_id, cb_url, cb_payload_size, cb_payload):
        client_mutex.acquire()

        sn=""
        c_format=""
        r_id = ""
        url = ""
        payload = ""
        if len(cb_sn):
          sn =  cb_sn.decode("utf-8")
        if len(cb_format):
          c_format = cb_format.decode("utf-8")
        if len(cb_id):
          r_id = cb_id.decode("utf-8")
        if len(cb_url):
          url = cb_url.decode("utf-8")
        
        print("ClientCB: SN:{}, format:{}, id:{}, url:{},  size:{} ".format(sn, c_format, r_id, url, cb_payload_size ))
        
        if c_format == "link_format" :
          if cb_payload is not None:
            print("ClientCB: link-format");
            payload = cb_payload.decode("utf-8")
            print("p:", payload)
        if c_format == "cbor" :
          print("ClientCB: cbor")
          payload = self.convertcbor2json(cb_payload, cb_payload_size)
          print("p:",payload)
        if c_format == "json" :
          print("ClientCB: json")
          payload = cb_payload.decode("utf-8")
          print("p:", payload)
        if c_format == "error" :
          print("ClientCB: error")
          print("p:", cb_payload_size)

        resp  = CoAPResponse(sn, c_format, r_id, url, cb_payload_size, payload)
        self.response_array.append(resp)
        client_event.set()
        client_mutex.release()         
    
    """ ********************************
    Call back handles resource call backs tasks.
    Resources is an dictionary with sn of device
    not used...
    **********************************"""
    def resourceCB(self, anchor, uri, rtypes, myjson):
        uuid = str(anchor)[8:-1]
        uuid_new = copy.deepcopy(uuid)
        my_uri = str(uri)[2:-1]

        if self.debug is not None and 'resources' in self.debug:
            print(colored("          Resource Event          \n",'green',attrs=['underline']))
            print(colored("UUID:{}, \nURI:{}",'green').format(uuid_new,my_uri))
        my_str = str(myjson)[2:-1]
        my_str = json.loads(my_str)
        
        duplicate_uri = False

        if self.resourcelist.get(uuid_new) is None:
            mylist = [ my_str ]
            #don't add duplicate resources lists
            if uuid_new not in self.resourcelist:
                self.resourcelist[uuid_new] = mylist
        else:
            mylist = self.resourcelist[uuid_new]
            #Make sure to not add duplicate resources if second discovery
            for resource in mylist:
                if my_uri == resource['uri']:
                    duplicate_uri=True
            if not duplicate_uri:
                mylist.append(my_str)
            #don't add duplicate resources lists
            if uuid_new not in self.resourcelist:
                self.resourcelist[uuid_new] = mylist
        if self.debug is not None and 'resources' in self.debug:
            print(colored(" -----resourcelist {}",'cyan').format(mylist))

        #Look for zero length uri...this means discovery is complete
        if len(my_uri) <=0:
            resource_event.set()
            print("ALL resources gathered");
        if self.debug is not None and 'resources' in self.debug:
            print(colored("Resources {}",'yellow').format(self.resourcelist))


    def __init__(self, debug=True):
        print ("loading ...")
        resource_mutex.acquire()
        if sys.platform == 'linux':
            libname = "libkisCS.so"
        else:
            libname = "kisCS.dll"
        print ("loading :", libname)
        libdir = os.path.dirname(__file__) 
        print ("at :", libdir)
        self.timout = 3
       
        self.lib=ctl.load_library(libname, libdir)
        # python list of copied unowned devices on the local network
        # will be updated from the C layer automatically by the CHANGED_CALLBACK
        self.discovered_devices = []
        # resource list
        self.resourcelist = {}

        self.device_array = []
        self.response_array = []

        print (self.lib)
        print ("...")
        self.debug=debug
        value = self.lib.py_get_max_app_data_size()
        print("py_get_max_app_data_size :", value)
        
        self.counter = 1

        self.changedCB = CHANGED_CALLBACK(self.changedCB)
        self.lib.py_install_changedCB(self.changedCB)


        #ret = self.lib.oc_storage_config("./onboarding_tool_creds");
        #print("oc_storage_config : {}".format(ret))

        self.resourceCB = RESOURCE_CALLBACK(self.resourceCB)
        self.lib.py_install_resourceCB(self.resourceCB)
        
        self.clientCB = CLIENT_CALLBACK(self.clientCB)
        self.lib.py_install_clientCB(self.clientCB)

        print ("...")
        self.threadid = threading.Thread(target=self.thread_function, args=())  
        self.threadid.start()
        print ("...")
        
    def get_r_id(self):
        self.counter = self.counter + 1
        return str(self.counter)


    def thread_function(self):
        """ starts the main function in C.
        this function is threaded in python.
        """
        print ("thread_function: thread started")
        init = self.lib.py_main()
            
    def get_result(self): 
        self.lib.get_cb_result.restype = bool
        return self.lib.get_cb_result()

    def purge_device_array(self, sn):
        for index, device in enumerate(self.device_array):
            if device.sn == sn:
                #print("Remove: {}".format(device.sn))
                self.device_array.pop(index)
        
    def discover_devices(self, scope=2):
        print("Discover Devices: scope", scope)
        # application
        discover_event.clear()
        ret = self.lib.py_discover_devices(c_int(scope))
        time.sleep(2)
        # python callback application
        print("[P] discovery- done")
        nr_discovered = self.lib.py_get_nr_devices()
        discover_event.wait(self.timout)
        print("Discovered DEVICE ARRAY {}".format(self.device_array))
        return self.device_array

    def device_array_contains(self, sn):
        contains = False
        for index, device in enumerate(self.device_array):
            if device.sn == sn:
                contains = True
        return contains 

    def get_device(self,sn):
        ret = None
        for index, device in enumerate(self.device_array):
            if device.sn == sn:
                ret = device
        return ret 

    def return_devices_array(self):
        return self.device_array


    def purge_response_by_id(self, r_id):
        #return 
        for index, resp in enumerate(self.response_array):
          if resp.r_id == r_id:
            #print("purge_response: Remove: {}".format(resp.r_id))
            self.response_array.pop(index)

    def purge_response(self, response):
        if response is not None:
          self.purge_response_by_id(response.r_id)

    def find_response(self, r_id):
        #print("==> find response: ", r_id)
        for item in self.response_array:
           # print("    find_response:", item.r_id)
           if str(item.r_id) == str(r_id):
               return item
        return None

    def issue_cbor_get(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_cbor_get", sn, uri, query, r_id)
        self.lib.py_cbor_get.argtypes = [String, String, String, String]
        #self.lib.py_general_get.restype = c_int
        client_event.clear()
        self.lib.py_cbor_get(sn, uri, query, r_id)
        client_event.wait(self.timout)
        response =  self.find_response(r_id)
        if response is None :
            client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_linkformat_get(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_linkformat_get", sn, uri, query, r_id)
        self.lib.py_linkformat_get.argtypes = [String, String, String, String]
        #self.lib.py_general_get.restype = c_int
        
        client_event.clear()
        self.lib.py_linkformat_get(sn, uri, query, r_id)
        client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_cbor_post(self, sn, uri, content, query=None) :
        r_id = self.get_r_id()
        
        client_event.clear()
        print(" issue_cbor_post", sn, uri, query, r_id)
        try:
          payload = cbor.dumps(content)
          payload_len = len(payload)
          print(" len :", payload_len)
          print(" cbor :", payload)

          self.lib.py_cbor_post.argtypes = [String, String, String, String, c_int, String]
          self.lib.py_cbor_post(sn, uri, query, r_id, payload_len, payload)
        except:
            pass
        # print(" issue_cbor_post - done")
        client_event.wait(self.timout)
        return self.find_response(r_id)

        
    def issue_cbor_put(self, sn, uri, content, query=None) :
        r_id = self.get_r_id()
        
        client_event.clear()
        print(" issue_cbor_put", sn, uri, query, r_id)
        try:
          payload = cbor.dumps(content)
          payload_len = len(payload)
          print(" len :", payload_len)
          print(" cbor :", payload)

          self.lib.py_cbor_put.argtypes = [String, String, String, String, c_int, String]
          self.lib.py_cbor_put(sn, uri, query, r_id, payload_len, payload)
        except:
            pass
        print(" issue_cbor_put - done")
        client_event.wait(self.timout)
        return self.find_response(r_id)

        
    def issue_cbor_delete(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        
        client_event.clear()
        print(" issue_cbor_delete", sn, uri, query, r_id)
        try:
          self.lib.py_cbor_delete.argtypes = [String, String, String, String]
          self.lib.py_cbor_delete(sn, uri, query, r_id)
        except:
            pass
        print(" issue_cbor_delete - done")

    def quit(self):
        self.lib.py_exit(c_int(0))

    def sig_handler(self, signum, frame):
        print ("sig_handler..")
        time.sleep(1)
        self.quit()
        sys.exit()

    def get_nr_devices(self):
        # retrieves the number of discovered devices
        # note that a discovery request has to be executed before this call
        self.lib.py_get_nr_devices.argtypes = []
        self.lib.py_get_nr_devices.restype = c_int
        return self.lib.py_get_nr_devices()

    def my_sleep(self):
        while True:
            time.sleep(3)

if __name__ == "__main__": 
    my_stack = KNXIOTStack()
    signal.signal(signal.SIGINT, my_stack.sig_handler)

    try:
        # need this sleep, because it takes a while to start the stack in C in a Thread
        time.sleep(1)
        devices = my_stack.discover_devices()

        if my_stack.get_nr_devices() > 0:
            # do some calls

            print ("SN :", my_stack.device_array[0].sn)
            response = my_stack.issue_linkformat_get(my_stack.device_array[0].sn, "/dev")
            print ("response:", response)
            my_stack.purge_response(response)
        
            print("Get IA :");
            response = my_stack.issue_cbor_get(my_stack.device_array[0].sn, "/dev/ia")
            print ("response:", response)
            my_stack.purge_response(response)
        
            print("Get HW Type :");
            response = my_stack.issue_cbor_get(my_stack.device_array[0].sn, "/dev/hwt")
            print ("response:", response)
            my_stack.purge_response(response)
        
            content = {"cmd": "startLoading"}
            response = my_stack.issue_cbor_post(my_stack.device_array[0].sn, "/a/lsm", content)
            print ("response:", response)
            my_stack.purge_response(response)

    except:
           traceback.print_exc()

    # close down the stack
    time.sleep(2)
    my_stack.quit()









