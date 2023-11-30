#!/bin/bash

for f in *.c; do gcc -O0 "$f" -o "${f%.c}"; done
