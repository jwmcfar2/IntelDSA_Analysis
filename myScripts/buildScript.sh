#!/bin/bash

# Make bin/ folder if it doesn't exist
mkdir -p bin/

# Flags included for these reasons (order matching): native-architecture, ~pthreads~, DSA, SSE2, SSE4.1, AVX2, AVX-512, AVX-512 Double/Quad Word Support, AMX
featureFlags="-march=native -laccel-config -msse2 -msse4.1 -mavx2 -mavx512f -mavx512dq -mamx-tile"
# -lpthread 
# Flags for compilation: flto = extern inlining support
compilationFlags=""

# Compile all .c files in src/modules to object files
for f in src/modules/*.c; do
    gcc -c "$f" -o "${f%.c}.o" $featureFlags
done

# Now compile DSA-based programs with DSA config libraries
for f in src/main/*.c; do
    outfile=$(basename "${f%.c}")
    if [[ "$f" == *"DSATest"* ]]; then
        objFiles=$(find src/modules/ -name "*.o" | tr '\n' ' ')
        gcc -O0 "$f" $objFiles -o "bin/$outfile" $featureFlags
    elif [[ "$f" == *"DSA"* ]]; then
        gcc -O0 "$f" -o "bin/$outfile" $featureFlags
    else
        gcc -O0 "$f" -o "bin/$outfile" $featureFlags
    fi
done

# DEBUG
gcc -g -O0 src/main/DSATest.c $objFiles $featureFlags -o bin/debug 
#gcc $objFiles -S -fverbose-asm -masm=intel -o src/main/DSATest.asm src/main/DSATest.c $featureFlags
#gcc -S -fverbose-asm -masm=intel -o src/main/exampleDSA.asm src/main/exampleDSA.c $featureFlags
