#!/bin/bash

for f in src/*.c; do gcc -O0 "$f" -o "${f%.c}"; done
