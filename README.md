# wavexe
*Tiny Executable Wavetable Synthesizer*

Really simple little program that musically messes with wavecycles and bakes them into a wave file. **Heavily** (and sporadically) in development right now, so it's pretty limited. The sequencing is currently powered by the mmml language, for which you can find the compiler here --->  https://github.com/protodomemusic/mmml/tree/master/compiler

You can probably work out how to build from the included script but, once you've built the mmml compiler from source, you'll need to do the following with a valid mmml file (example included in repo):

`./mmml-compiler -f input.mmml -t wavexe -c 6` then `gcc wavexe.c -o wavexe` and `./wavexe` to create the wave file.

(Also, keep an eye on the bugs in the header of `wavexe.c`, there's a few.)
