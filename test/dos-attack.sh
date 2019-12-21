#!/bin/bash

while true; do
  printf "yandex.ru\ngoogle.com\n" | telnet localhost 1337 >/dev/null 2>&1 &
done
