#!/bin/sh

# put the path to your FLTK repository here (relative to subdir 'intern')
fltk=../../fltk-1.3

target=animgifimage
src=intern
#opt=-pg

# copy files into FLTK repo & compile FLTK
if [ "$1" = "all" ]; then
	cp -a $src/fl_*.H $fltk/src/.
	cp -a $src/*GIF*.H $fltk/FL/.
	cp -a $src/*GIF*.cxx $fltk/src/.

	cwd=$(pwd)
	cd $fltk
	make
	cd $cwd
fi

# build the testprogram
cwd=$(pwd)
cd $src
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
cd $cwd
mv $src/$target .
