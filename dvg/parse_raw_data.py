#!/bin/python3

import sys
import re

if len(sys.argv) < 2:
    print("pass filename")
    exit()

filename = sys.argv[1]

#print("filename {}".format(filename))

of = open("out.bin", "wb")

with open(filename) as f:
    line = f.readline()
    count=0
    while line:
        if re.search('^0[0-9A-F]', line) :
            addrstr = line[0:4]
            #print(addrstr, end=" ")
            addr= int(addrstr, 16)

            if count != addr - 0x800 :
                print("error", count, addr, addrstr)
                break

            if re.search('CUR', line) or re.search(' VEC ', line) :
                # decode 4 bytes
                #print(line[6:17])
                ba = bytearray.fromhex(line[6:17])
                count += 4
            else:
                #decode 2 bytes
                #print(line[6:11])
                ba = bytearray.fromhex(line[6:11])
                count += 2

            #hex_string = "".join("%02X " % b for b in ba)

            #print(addr, addrstr, hex_string)
            of.write(ba)

        line = f.readline()

