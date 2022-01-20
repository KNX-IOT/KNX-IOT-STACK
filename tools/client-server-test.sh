#!/bin/bash 

echo `pwd`
ls -l
mydir=$(mktemp -d "${TMPDIR:-/tmp/}$(basename $0).XXXXXXXXXXXX")
mydir=.
echo $mydir

python -m pip install -r ../pythonbinding/requirements.txt

cp ./libkisCS.so ../pythonbinding/


#cd ../pythonbinding/tests
#python knx_test_sequence.py -sleep > $mydir/python_out.txt 2>&1 &
#pythonPID=$!
#ps

echo "--listing apps----"
ls ../linuxbuild_clientserver/apps

echo "--Starting simpleserver_all----"
../linuxbuild_clientserver/apps/simpleserver_all 2>&1 |tee  $mydir/app_out.txt &
serverPID=$!


echo "--Starting Python----"

cd ../pythonbinding/tests
python knx_test_sequence.py 2>&1 | tee  $mydir/python_out.txt
#pythonPID=$!
ps


echo "---killing applications------"
#kill $pythonPID
kill $serverPID

echo "---python output------"

#cat $mydir/python_out.txt | xargs echo -e
cat -v $mydir/python_out.txt

echo "---app output------"
cat -v $mydir/app_out.txt

echo "---Collecting Failures------"

if grep -q $mydir/python_out.txt; then
    echo Failures found
    exit 1
else
    echo no failures found
    exit 0
fi

