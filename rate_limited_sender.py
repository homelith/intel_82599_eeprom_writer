#-------------------------------------------------------------------------------
# rate_limited_sender.py
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# MIT License
#
# Copyright (c) 2021 homelith
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#-------------------------------------------------------------------------------

import os
import sys
import time
import signal
import serial
import threading

handle_serial = None
thread_running = False
transfer_running = False
start_word = "writer_ready"
end_word = "writer_end"
baud_rate = 9600

port_name = ""
file_name = ""
target_bps = 300

def open_serial():

    global handle_serial
    global baud_rate
    global port_name

    try:
        print("open '%s' at %u bps ..." % (port_name, baud_rate))
        handle_serial = serial.Serial(port=port_name, baudrate=baud_rate, timeout=0.1)
        print("done.")
    except:
        print("port '%s' not found." % port_name)
        print("usage : python3 rate_limited_sender.py {port_name (e.g. /dev/ttyACM0)} {filename to send} ( {target_bps (default: 300)} )")
        sys.exit(1)

def read_proc():

    global thread_running
    global transfer_running
    global handle_serial

    start_match_count = 0
    end_match_count = 0

    while thread_running:
        if handle_serial.inWaiting() > 0 :
            c = handle_serial.read().decode('ascii')
            print(c, end="")

            # search start_word and start write process
            if start_word[start_match_count] == c :
                if start_match_count == (len(start_word) - 1) :
                    print("'%s' detected" % start_word)
                    start_match_count = 0
                    transfer_running = True
                else:
                    start_match_count += 1
            else:
                start_match_count = 0

            # search end_word and exit program
            if end_word[end_match_count] == c :
                if end_match_count == (len(end_word) - 1) :
                    print("'%s' detected" % end_word)
                    end_match_count = 0
                    thread_running = False
                else:
                    end_match_count += 1
            else:
                end_match_count = 0
        time.sleep(0.0001)

def write_proc() :

    global thread_running
    global transfer_running
    global handle_serial

    prev_tick = 0.0
    remainder_bucket = 0.0
    target_interval = 1.0 / (target_bps / 8.0)

    bytes_sent = 0

    # open input file
    fp = open(file_name, "rb")

    while thread_running :
        curr_tick = time.time();

        # wait until start_word arrived at RX
        if transfer_running == False :
            prev_tick = curr_tick + 1.0
            continue

        # rate limiting
        if (curr_tick - prev_tick + remainder_bucket) < target_interval :
            continue
        remainder_bucket += (curr_tick - prev_tick) - target_interval
        if remainder_bucket < 0.0 :
            remainder_bucket = 0.0
        if remainder_bucket >= target_interval:
            remainder_bucket = target_interval
        prev_tick = curr_tick

        # write 1 byte to serial
        c = fp.read(1)
        if len(c) != 0 :
            handle_serial.write(c)
            bytes_sent += 1
            if bytes_sent % 400 == 0 :
                print("sender : %u bytes sent" % bytes_sent)

        time.sleep(0.0001)

    fp.close()

if __name__ == "__main__" :

    # evaluate cmd arguments
    if len(sys.argv) <= 2 :
        print("usage : python3 rate_limited_sender.py {port_name (e.g. /dev/ttyACM0)} {filename to send} ( {target_bps (default: 300)} )")
        sys.exit(1)

    port_name = sys.argv[1]
    file_name = sys.argv[2]
    if os.path.isfile(file_name) == False :
        print("input file '%s' not found." % file_name)
        print("usage : python3 rate_limited_sender.py {port_name (e.g. /dev/ttyACM0)} {filename to send} ( {target_bps (default: 300)} )")
        sys.exit(1)
    if len(sys.argv) >= 4 :
        target_bps = int(sys.argv[2])

    # init serial port
    open_serial()

    try :
        # start read/write thread and wait
        read_thread = threading.Thread(target=read_proc)
        write_thread = threading.Thread(target=write_proc)
        thread_running = True
        read_thread.start()
        write_thread.start()
        while thread_running:
            time.sleep(1)

    finally :
        print("waiting for thread join ... ")
        thread_running = False
        read_thread.join()
        write_thread.join()
        handle_serial.close()
        print("done.")
        sys.exit(0)
