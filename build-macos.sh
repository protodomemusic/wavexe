#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

rm wavexe

# note this will fail if you've just downloaded from the repository
# you can ignore it though (unless you want to write your own music)
./mmml-compiler -f input.mmml -t wavexe -c 8

gcc wavexe.c -O3 -lm -o wavexe

# if you want tiny executables, uncomment below (and install upx)
# gcc wavexe.c -O3 -lm -o wavexe-big
# upx -9 -o wavexe wavexe-big
# rm wavexe-big

./wavexe

# note no software playback on mac (linux audio libraries are easier)
afplay output.wav