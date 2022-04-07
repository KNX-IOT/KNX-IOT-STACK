#!/usr/bin/env python
#############################
#
#    copyright 2021-2022 Cascoda Ltd.
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
# pylint: disable=C0413
# pylint: disable=R0801
# pylint: disable=R0902
# pylint: disable=R0913
# pylint: disable=R0915
# pylint: disable=R1732
# pylint: disable=W0702
# pylint: disable=W1514

import sys
import time
import traceback
import os
import signal
import argparse

#add parent directory to path so we can import from there
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

import knx_stack

#
#
# {sia: 5678, es: {st: write, ga: 1, value: 100 }}
#
#
if __name__ == '__main__':  # pragma: no cover

    parser = argparse.ArgumentParser()
    # input (files etc.)

    parser.add_argument("-iid", "--iid", default=5,
                    help="receiving installation identifier (default iid=5)", nargs='?',
                    const="", required=False)
    parser.add_argument("-ga_max", "--ga_max", default=20,
                    help="group address range(default [1,20]", nargs='?', const="",
                    required=False)
    parser.add_argument("-wait", "--wait",
                    help="wait after issuing s-mode command", nargs='?',
                    default=200000, const=1, required=False)
    # (args) supports batch scripts providing arguments
    print(sys.argv)
    args = parser.parse_args()

    print("iid        :" + str(args.iid))
    print("ga range   :" + str(args.ga_max))
    print("wait [sec] :" + str(args.wait))
    the_stack = knx_stack.KNXIOTStack()
    the_stack.start_thread()

    signal.signal(signal.SIGINT, the_stack.sig_handler)
    time.sleep(2)

    try:
        the_stack.listen_s_mode(2, args.ga_max, args.iid)
        the_stack.listen_s_mode(5, args.ga_max, args.iid)
    except:
        traceback.print_exc()

    time.sleep(int(args.wait))
    the_stack.quit()
    sys.exit()
