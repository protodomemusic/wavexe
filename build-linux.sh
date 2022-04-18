#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

rm wavexe

# note this will fail if you've just downloaded from the repository
# you can ignore it though (unless you want to write your own music)
./mmml-compiler -t wavexe -f input.mmml -c 8

# heavily optimise for speed, link math and alsa sound libraries
gcc wavexe.c -O3 -o wavexe -lm -lasound

# if you want tiny executables, uncomment below (and install upx)
# gcc wavexe.c -O3 -o wavexe-big -lm -lasound
# upx -9 -o wavexe wavexe-big
# rm wavexe-big

./wavexe