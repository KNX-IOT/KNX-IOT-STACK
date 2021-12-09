# Python bindings 

The python bindings are based on ctypes.
The python code is using the shared library containing an KNX-IOT stack based device that can act as an client.
The python code uses the device to interact with other KNX-IOT devices on the network.

## install python requirements

python -m pip install -r requirements.txt

## limitations

Only tested on Linux based systems.


## Build Shared Library

run the following command from this folder:

```
./build\_so.sh
```

This command builds the shared library and copies the library to this folder.
Hence the python code can be used directly from this folder.



## Build Shared Library Windows

run the following command from this folder:

```
start_shell.bat
```
This command opens a windows developer shell that knows all paths of visual studio.
in this shell issue the commanbd
```
build_so.bat
```

This command builds the windows shared library .
Hence the python code can be used directly from this folder.

Note that the windows dll is build for win32.
Hence the python interpreter MUST be a win32 version.

```
python knx_stack.py
```
