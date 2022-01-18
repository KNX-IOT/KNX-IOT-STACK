#!/bin/bash 

echo `pwd`
ls -l
mydir=$(mktemp -d "${TMPDIR:-/tmp/}$(basename $0).XXXXXXXXXXXX")

python -m pip install -r ../pythonbinding/requirements.txt

cp ./libkisCS.so ../pythonbinding/


echo "--Starting Python----"

cd ../pythonbinding/tests
python knx_test_sequence.py -sleep > $mydir/python_out.txt 2>&1 2&
pythonPID=$!
ps

ls ../../linuxbuild_clientserver/apps


echo "--Starting simpleserver_all----"
../../linuxbuild_clientserver/apps/simpleserver_all &
serverPID=$!

sleep 60

kill $pythonPID
kill $serverPID

echo "---Collecting Failures------"

if grep -q$mydir/python_out.txt; then
    echo Failures found
else
    echo no failures found
fi

