#!/bin/sh

# put the path to your FLTK repository here (relative to subdir 'extern')
fltk=../../fltk-1.3

target=gif_animation
src=extern
#opt=-pg

# build the test program
cwd=$(pwd)
cd $src
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
cd $cwd
mv $src/$target .
