#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjuction with the mmml compiler.
./mmml-compiler -f input.mmml -t wavexe -c 5
gcc wavexe.c -o wavexe
./wavexe
afplay output.wav