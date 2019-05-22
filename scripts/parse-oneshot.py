#!/usr/bin/env python3

# parse the text output of various oneshot to make a scatterplot with all samples

import re
import sys
import argparse

START_TAG = "---BENCHMARK-START---"
END_TAG   = "---BENCHMARK-END---"

parser = argparse.ArgumentParser("parse-oneshot")
parser.add_argument('-c','--columns', nargs='+', help='columns to extract', required=True)

args = parser.parse_args()

print("columns ", args.columns)

name_regex = 'mlp([0-9]*)'

columns = None
state = 'outside'

for line in sys.stdin:
  # print(state, " >>> ", line)
  if state == 'outside':
    if (re.match(START_TAG, line)):
      state = 'first'
  elif state == 'first':
    # print("first line: ", line)
    headings = line.split()
    # print(columns)
    state = 'inside'
  elif state == 'inside':
    if (re.match(END_TAG, line)):
      state = 'outside'
    else:
      data = line.split()
      name = data[0]
      m = re.match(name_regex, name)
      if (m and m.group(1)):
        print(m.group(1), end='', sep='')
      else:
        print("WARNING: name_rexeg didn't match name: ", name, ", line: ", line)
      # extract the desired columns
      for i in args.columns:
        print(",", data[int(i)], end='', sep='')
      print()
  else:
    sys.exit("bad state")


    
    #print(match.group(1), match.group(2), sep=',')
