#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

rm wavexe
./mmml-compiler -f input.mmml -t wavexe -c 6
gcc wavexe.c -o wavexe
#gcc wavexe.c -o wavexe-big
#upx -9 -o wavexe wavexe-big # compress w/ upx for tiny files - can be commented out
#rm wavexe-big
./wavexe
afplay output.wav
#./wavexe | play -t raw -e unsigned-integer -b 8 -c 1 -r 44100 - # play with sox (MacOS)
#./wavexe | aplay -r 44100 # play with aplay (linux)