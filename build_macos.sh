#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

rm wavexe
./mmml-compiler -f input.mmml -t wavexe -c 8
gcc wavexe.c -O3 -o wavexe

#if you want tiny executables, uncomment below (and install upx)
#gcc wavexe.c -o wavexe-big
#upx -9 -o wavexe wavexe-big # compress w/ upx for tiny files - can be commented out
#rm wavexe-big

./wavexe
afplay output.wav