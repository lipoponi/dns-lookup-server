#!/bin/bash

while true; do
  cat ./domains.txt | telnet localhost 1337 >/dev/null 2>&1 &
done
