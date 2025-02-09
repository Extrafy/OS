#!/bin/bash
grep -n $2 $1 > temp
awk -F: '{print $1}' temp > $3
rm temp
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
