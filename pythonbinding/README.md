# Python bindings

The python bindings are based on python ctypes.
The python code is using the shared library (dll) containing an KNX-IOT stack based device that can act as an client.
The python code uses the device to interact with other KNX-IOT devices on the network.

![python architecture}](../images/python-architecture.png)

## requirements

- windows based machine (Win10 or Win11)
- visual studio compiler (can be community edition)
- cmake environment
- python

### install python requirements

python -m pip install -r requirements.txt

## Build Shared Library on Windows for debugging

run the following command from the build directory in a git bash shell:

```bash
cmake .
```

Assuming the build directory has been configured correctly, this command ensures that the latest
version of the scripts are copied into the build dir. Since the scripts are copied at configure time,
you need to use this command whenever you want to test a change to the scripts, or you will see the
old behaviour instead.

```bash
start_shell.bat
```

This command opens a windows developer shell that knows all paths of visual studio.
in this shell issue the commands to build in this folder.

```bash
cmake -G"NMake Makefiles" ..
nmake
```

This command builds the windows shared library.
Therefore the python code can be used directly from this folder.

```bash
python knx_stack.py
```

## Build Shared Library on Linux
Within the build directory:

```bash
cmake .
make
python knx_stack.py
```

### known issues

Note that the windows dll is build for win32 or x64, depending on the default installation of visual studio
Hence the python interpreter MUST match that.

Both combinations (win32 and x64) are known to work.

For more information: https://stackoverflow.com/questions/48000185/python-ctypes-dll-is-not-a-valid-win32-application
