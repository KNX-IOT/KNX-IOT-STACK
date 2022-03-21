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
# pylint: disable=C0302
# pylint: disable=C0413
# pylint: disable=R0201
# pylint: disable=R0202
# pylint: disable=R0801
# pylint: disable=R0902
# pylint: disable=R0904
# pylint: disable=R0911
# pylint: disable=R0913
# pylint: disable=R0915
# pylint: disable=R1705
# pylint: disable=R1706
# pylint: disable=R1732
# pylint: disable=W0212
# pylint: disable=W0401
# pylint: disable=W0702
# pylint: disable=W1514

# disable this for testing
# unused imports, init method not callded
# pylint: disable=W0231
# pylint: disable=W0614

#import argparse
import binascii
import copy
import ctypes
import json
import os
import os.path
import signal
import sys
import threading
import time
import traceback
from ctypes import *

import cbor
import numpy.ctypeslib as ctl
from termcolor import colored

unowned_return_list=[]

discover_event = threading.Event()
discover_data_event = threading.Event()
spake_event = threading.Event()
client_event = threading.Event()
client_mutex = threading.Lock()
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

    #def __cmp__(self, string):
    #    if isinstance(string, UserString):
    #        return cmp(self.data, string.data)
    #    return cmp(self.data, string)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
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
        return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
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

    def decode(self, encoding=None, errors=None):
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            return self.__class__(self.data.decode(encoding))
        return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            return self.__class__(self.data.encode(encoding))
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

def ReturnString(obj, _func=None, _arguments=None):
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
CLIENT_CALLBACK = CFUNCTYPE(None, c_char_p, c_int, c_char_p, c_char_p, c_char_p, c_int, c_char_p)
DISCOVERY_CALLBACK = CFUNCTYPE(None, c_int, c_char_p)
SPAKE_CALLBACK = CFUNCTYPE(None, c_char_p, c_int, c_char_p, c_int)

#----------LinkFormat parsing ---------------

class LinkFormat():

    def __init__(self, my_response):
        self.response = my_response
        self.lines = self.response.splitlines()

    def get_nr_lines(self):
        # make sure that the string is zero, e.g. remove leading/trailing white space
        if len(self.response.strip()) == 0:
            return 0
        return len(self.lines)

    def get_line(self, index):
        return self.lines[index]

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
        my_url = url.replace("coap://","")
        my_url = my_url.replace("coaps://","")
        mybase = my_url.split("/")
        return mybase[0]

#----------Devices ---------------

class Device():

    def __init__(self, sn, ip_address=None, name="", _resources=None,
                 _resource_array=None, credentials=None, last_event=None):
        self.sn = sn
        self.ip_address = ip_address
        #self.owned_state = owned_state
        self.name = name
        self.credentials = credentials
        self.resource_array = []
        self.last_event = last_event

    def __str__(self):
        return "Device Sn:{}".format(self.sn)

    def get_sn(self):
        return self.sn

#----------Responses ---------------
class CoAPResponse():

    def __init__(self, sn, status, payload_format, r_id, url, payload_size, payload):
        self.sn = sn
        self.r_id = r_id
        self.url = url
        self.status = status
        self.payload_type = payload_format
        self.payload_size = payload_size
        self.payload = payload

    def __str__(self):
        return "Payload Sn:{} r_id:{}".format(self.sn ,self.r_id)

    def get_sn(self):
        return self.sn

    def print_payload(self):
        if self.payload_type == "json":
            print("::",self.get_payload_dict())
            return
        print(";:",self.payload)

    def get_payload(self):
        if self.payload_type == "json":
            return self.payload
        return self.payload

    def get_payload_string(self):
        if self.payload_type == "json":
            return self.payload
        return self.payload

    def get_payload_boolean(self):
        if self.payload_type == "json":
            my_string = self.get_payload()
            if my_string == "true":
                return True
            if my_string == "false":
                return False
            return self.payload
        return self.payload

    def get_payload_int(self):
        if self.payload_type == "json":
            my_string = self.get_payload()
            print("get_payload_int:", int(my_string))
            return int(my_string)
        return self.payload

    def get_payload_dict(self):
        """
          note that this is a bit of a fudge...
          the json is wrapped in the python layer above the stack
          this is not complete json due to that there is no good way of
          using cbor with and without {}
        """
        if self.payload_type == "json":
            my_string = str(self.payload)
            #print ("get_payload_dict", my_string)
            try:
                return json.loads(my_string)
            except:
                #print("get_payload_dict: replacing {x with {:")
                my_str = my_string.replace("{x","{")
                try:
                    return json.loads(my_str)
                except:
                    #print("get_payload_dict: replacing {x with {'0':")
                    my_str = my_string.replace("{x",'{"0":')
                    try:
                        return json.loads(my_str)
                    except:
                        pass
        return self.payload

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
        if len(payload) > payload_len:
            print ("truncating payload")
            payload = payload[:payload_len]
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
        if sn is not None:
            sn = sn.decode("utf-8")
        if cb_state is not  None:
            cb_state = cb_state.decode("utf-8")
        if cb_event is not  None:
            cb_event = cb_event.decode("utf-8")
        if cb_event == "discovered":
            print("Discovery Event:{} state:{}".format(sn, cb_state))
            dev = Device(sn , ip_address=cb_state, name=name)
            self.device_array.append(dev)
            discover_event.set()

    def clientCB(self, cb_sn, cb_status, cb_format, cb_id, cb_url, cb_payload_size, cb_payload):
        """ ********************************
        Call back handles client command callbacks.
        Client discovery/state
        **********************************"""
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
        print("ClientCB: SN:{}, status:{}, format:{}, id:{}, url:{},  size:{} ".
               format(sn, cb_status, c_format, r_id, url, cb_payload_size ))
        if c_format == "link_format" :
            if cb_payload is not None:
                print("ClientCB: link-format")
                payload = cb_payload.decode("utf-8")
        if c_format == "cbor" :
            print("ClientCB: cbor")
            payload = self.convertcbor2json(cb_payload, cb_payload_size)
        if c_format == "json" :
            print("ClientCB: json")
            try:
                #temp_payload = cb_payload[:cb_payload_size]
                payload = cb_payload.decode("utf-8")
            except:
                payload = cb_payload
        if c_format == "error" :
            print("ClientCB: error")
            print("p:", cb_payload_size)
        resp  = CoAPResponse(sn, cb_status, c_format, r_id, url, cb_payload_size, payload)
        self.response_array.append(resp)
        client_event.set()
        client_mutex.release()

    def resourceCB(self, anchor, uri, _rtypes, myjson):
        """ ********************************
        Call back handles resource call backs tasks.
        Resources is an dictionary with sn of device
        not used...
        **********************************"""
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
        #if len(my_uri) <=0:
        #    resource_event.set()
        #    print("ALL resources gathered")
        if self.debug is not None and 'resources' in self.debug:
            print(colored("Resources {}",'yellow').format(self.resourcelist))

    def discoveryCB(self, cb_payload_size, cb_payload):
        """ ********************************
        Call back handles discovery callbacks.
        Client discovery/state
        **********************************"""
        print("discoveryCB", cb_payload_size)
        data = cb_payload[:cb_payload_size]
        self.discovery_data = data.decode("utf-8")
        discover_data_event.set()

    def spakeCB(self, cb_sn, cb_state, cb_secret, cb_secret_size):
        """ ********************************
        Call back handles spake callbacks.
        Client spake/state
        **********************************"""
        print("spakeCB", cb_sn, cb_state, cb_secret_size) #, hex(cb_secret))
        new_secret = cb_secret[:cb_secret_size]
        self.spake = { "state": cb_state, "sec_size": cb_secret_size, "secret" : new_secret}
        secret_in_hex = binascii.hexlify(new_secret)
        print ("spakeCB: secret (in hex)",secret_in_hex)
        #print (len(new_secret))
        spake_event.set()

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
        self.resourcelist = {}
        self.device_array = []
        self.response_array = []
        self.discovery_data = None
        self.spake = {}
        print (self.lib)
        print ("...")
        self.debug=debug
        value = self.lib.py_get_max_app_data_size()
        print("py_get_max_app_data_size :", value)
        self.counter = 1
        self.changedCBFunc = CHANGED_CALLBACK(self.changedCB)
        self.lib.py_install_changedCB(self.changedCBFunc)
        #ret = self.lib.oc_storage_config("./onboarding_tool_creds");
        #print("oc_storage_config : {}".format(ret))
        self.resourceCBFunc = RESOURCE_CALLBACK(self.resourceCB)
        self.lib.py_install_resourceCB(self.resourceCBFunc)
        self.clientCBFunc = CLIENT_CALLBACK(self.clientCB)
        self.lib.py_install_clientCB(self.clientCBFunc)
        self.discoveryCBFunc = DISCOVERY_CALLBACK(self.discoveryCB)
        self.lib.py_install_discoveryCB(self.discoveryCBFunc)
        self.spakeCBFunc = SPAKE_CALLBACK(self.spakeCB)
        self.lib.py_install_spakeCB(self.spakeCBFunc)
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
        self.lib.py_main()

    def get_result(self):
        self.lib.get_cb_result.restype = bool
        return self.lib.get_cb_result()

    def purge_device_array(self, sn):
        for index, device in enumerate(self.device_array):
            if device.sn == sn:
                self.device_array.pop(index)

    def discover_devices(self, scope=2):
        print("Discover Devices: scope", scope)
        # application
        discover_event.clear()
        self.lib.py_discover_devices(c_int(scope))
        time.sleep(self.timout)
        # python callback application
        print("[P] discovery- done")
        self.lib.py_get_nr_devices()
        discover_event.wait(self.timout)
        print("Discovered DEVICE ARRAY {}".format(self.device_array))
        return self.device_array

    def discover_devices_with_query(self, query, scope=2):
        print("Discover Devices with Query: scope", scope, query)
        # application
        discover_event.clear()
        self.lib.py_discover_devices_with_query.argtypes = [c_int, String ]
        self.lib.py_discover_devices_with_query(scope, query)
        time.sleep(self.timout)
        # python callback application
        print("[P] discovery- done")
        self.lib.py_get_nr_devices()
        discover_event.wait(self.timout)
        print("Discovered DEVICE ARRAY {}".format(self.device_array))
        return self.device_array

    def discover_devices_with_query_data(self, query, scope=2):
        print("Discover Devices with Query: scope", scope, query)
        # application
        discover_data_event.clear()
        self.discovery_data = None
        self.lib.py_discover_devices_with_query.argtypes = [c_int, String ]
        self.lib.py_discover_devices_with_query(int(scope), query)
        time.sleep(self.timout)
        # python callback application
        print("[P] discovery- done")
        self.lib.py_get_nr_devices()
        discover_data_event.wait(self.timout)
        print("Discovered DEVICE ARRAY {}".format(self.device_array))
        return self.discovery_data

    def initiate_spake(self, sn, password):
        print("initiate spake: ", sn, password)
        # application
        spake_event.clear()
        self.discovery_data = None
        self.lib.py_initiate_spake.argtypes = [ String, String ]
        self.lib.py_initiate_spake(sn, password)
        #time.sleep(self.timout)
        # python callback application
        #print("[P] discovery- done")
        #self.lib.py_get_nr_devices()
        spake_event.wait(self.timout)
        print ("initate_spake data: ")
        return self.spake
        #return self.discovery_data

    def device_array_contains(self, sn):
        contains = False
        for device in self.device_array:
            if device.sn == sn:
                contains = True
        return contains

    def get_device(self, sn):
        ret = None
        for device in self.device_array:
            if device.sn == sn:
                ret = device
        return ret

    def return_devices_array(self):
        return self.device_array

    def purge_response_by_id(self, r_id):
        for index, resp in enumerate(self.response_array):
            if resp.r_id == r_id:
                self.response_array.pop(index)

    def purge_response(self, my_response):
        if my_response is not None:
            self.purge_response_by_id(my_response.r_id)

    def find_response(self, r_id):
        for item in self.response_array:
            if str(item.r_id) == str(r_id):
                return item
        return None

    def issue_cbor_get(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_cbor_get", sn, uri, query, r_id)
        self.lib.py_cbor_get.argtypes = [String, String, String, String]
        client_event.clear()
        self.lib.py_cbor_get(sn, uri, query, r_id)
        client_event.wait(self.timout)
        my_response =  self.find_response(r_id)
        if my_response is None :
            client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_cbor_get_unsecured(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_cbor_get_unsecured", sn, uri, query, r_id)
        self.lib.py_cbor_get_unsecured.argtypes = [String, String, String, String]
        client_event.clear()
        self.lib.py_cbor_get_unsecured(sn, uri, query, r_id)
        client_event.wait(self.timout)
        my_response =  self.find_response(r_id)
        if my_response is None :
            client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_linkformat_get(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_linkformat_get", sn, uri, query, r_id)
        self.lib.py_linkformat_get.argtypes = [String, String, String, String]
        client_event.clear()
        self.lib.py_linkformat_get(sn, uri, query, r_id)
        client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_linkformat_get_unsecured(self, sn, uri, query=None) :
        r_id = self.get_r_id()
        print("issue_linkformat_get_unsecured", sn, uri, query, r_id)
        self.lib.py_linkformat_get_unsecured.argtypes = [String, String, String, String]
        client_event.clear()
        self.lib.py_linkformat_get_unsecured(sn, uri, query, r_id)
        client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_cbor_post(self, sn, uri, my_content, query=None) :
        r_id = self.get_r_id()
        client_event.clear()
        print(" issue_cbor_post", sn, uri, query, r_id)
        try:
            payload = cbor.dumps(my_content)
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

    def issue_cbor_put(self, sn, uri, my_content, query=None) :
        r_id = self.get_r_id()
        client_event.clear()
        print(" issue_cbor_put", sn, uri, query, r_id)
        try:
            payload = cbor.dumps(my_content)
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
        client_event.wait(self.timout)
        return self.find_response(r_id)

    def issue_s_mode(self, scope, sia, ga, iid, st, value_type, value) :
        print(" issue_s_mode: scope:{} sia:{} ga:{} iid:{} value_type:{} value:{}".
               format(scope, sia, ga, iid, value_type, value))
        try:
            self.lib.py_issue_requests_s_mode.argtypes = [c_int,
                       c_int, c_int, c_int, String, c_int, String]
            self.lib.py_issue_requests_s_mode(int(scope), int(sia), int(ga), int(iid),
                                              str(st), int(value_type), str(value))
        except:
            traceback.print_exc()
        print(" issue_s_mode - done")

    def quit(self):
        self.lib.py_exit(c_int(0))

    def sig_handler(self, _signum, _frame):
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
            print("Get IA :")
            response = my_stack.issue_cbor_get(my_stack.device_array[0].sn, "/dev/ia")
    except:
        traceback.print_exc()

    # close down the stack
    time.sleep(2)
    my_stack.quit()
