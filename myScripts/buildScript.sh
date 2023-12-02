#!/bin/bash

# Make bin/ folder if it doesn't exist
mkdir -p bin/

# Make sure to build any DSA-based programs with DSA config libraries
for f in src/*.c; do
    outfile=$(basename "${f%.c}")
    if [[ "$f" == *"DSA"* ]]; then
        gcc -O0 "$f" -o "bin/$outfile" -laccel-config
    else
        gcc -O0 "$f" -o "bin/$outfile"
    fi
done