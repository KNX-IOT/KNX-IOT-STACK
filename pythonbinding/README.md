# Python bindings 


The python bindings are based on ctypes.
The python code is using the shared library containing an KNX-IOT stack based device that can act as an client.
The python code uses the device to interact with other KNX-IOT devices on the network.

## requirements

- windows based machine (Win10 or Win11)
- visual studio compiler (can be community edition)
- cmake environment
- python (32 bit version)

### install python requirements

python -m pip install -r requirements.txt

## limitations

Only tested on Windows based systems.


## Build Shared Library Windows

run the following command from this folder:

```
start_shell.bat
```
This command opens a windows developer shell that knows all paths of visual studio.
in this shell issue the command:
```
build_so_windows.sh
```

This command builds the windows shared library .
Hence the python code can be used directly from this folder.

```
python knx_stack.py
```

### known issues

Note that the windows dll is build for win32.
Hence the python interpreter MUST be a win32 version.
e.g. install a 32-bit python version. 
