#!/bin/bash 

echo `pwd`
ls -l

python -m pip install -r ../pythonbinding/requirements.txt

cp ./libkisCS.so ../pythonbinding/

cd ../pythonbinding/tests
python knx_test_sequence.py -sleep 2 &
ps

ls ../../linuxbuild_clientserver/apps

../../linuxbuild_clientserver/apps/simpleserver_all 4

