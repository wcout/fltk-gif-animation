#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk-1.4

target=animgif
target1=animgif-resize
src=extern
#opt=-pg

# build the test programs
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $src/test/$target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
g++ -Wall -pipe -pedantic -O3 $opt -o $target1 `$fltk/fltk-config --use-images --cxxflags` $src/test/$target1.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
