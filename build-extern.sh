#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk-1.3

target=gif_animation
src=extern
#opt=-pg

# build the test program
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $src/$target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
