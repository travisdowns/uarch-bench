#!/usr/bin/env python3

import re
import sys

for line in sys.stdin:
  match = re.search('([0-9]*)-KiB serial loads *([0-9\.]*)', line)
  if match:
    print(match.group(1), match.group(2), sep=',')
