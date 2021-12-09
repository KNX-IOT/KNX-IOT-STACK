#!/bin/bash
echo `pwd`

# C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Visual Studio 2019\Visual Studio Tools\VC


set CXX="C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/cl.exe"

# cmake -G"NMake Makefiles" -A Win32 ..
cmake -G"NMake Makefiles" ..

nmake


