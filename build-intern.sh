#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk-1.4

target=animgifimage
src=intern
#opt=-pg

# copy files into FLTK repo & compile FLTK
if [ "$1" = "all" ]; then
	cp -av $src/fl_*.H $fltk/src/.
	cp -av $src/*GIF*.H $fltk/FL/.
	cp -av $src/*GIF*.cxx $fltk/src/.

	cwd=$(pwd)
	cd $fltk
	make
	cd $cwd
fi

# build the testprogram
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $src/$target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
