#!/bin/bash

grep -A1 "^[0-9]" reception.txt | grep -v "^--$" | awk '{print $1" "$6}' > __all.txt
grep -v "^0x" __all.txt | awk '{print $1}' > timestamps.txt
grep "^0x" __all.txt | awk '{print $2}' > numbers.txt
rm __all.txt
