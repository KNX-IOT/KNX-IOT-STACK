#!/bin/sh


match='-pedantic'
insert='-pedantic -ftest-coverage -fprofile-arcs'
file='Makefile'

project="$PWD"

root_dir="${project}/../../.."
linux_dir="${root_dir}/port/linux/"
cd $linux_dir


sed -i "s/$match/$insert/" $file

make clean
make test
mkdir ${project}/coverage_report
lcov -c -d ../../ -o ${project}/coverage_report/new_coverage.info && lcov --remove ${project}/coverage_report/new_coverage.info 'port/unittest/*' '/usr/include/*' 'api/unittest/*' -o ${project}/coverage_report/main_coverage.info

genhtml ${project}/coverage_report/main_coverage.info --output-directory ${project}/coverage_report/

sed -i "s/$insert/$match/" $file


