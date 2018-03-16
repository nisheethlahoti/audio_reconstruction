#!/bin/bash
# Gets the dynamic range of a 16-bit wav or pcm file, input as stdin

xxd |
tail -n +20 |
sed -Ee "s/^[^:]*: //" -e "s/\w\w(\w)\w /\1/g" -e "s/ .*$//g" |
sed -Ee "s/./&\n/g" |
grep -ve '^$' |
sort |
uniq -c
