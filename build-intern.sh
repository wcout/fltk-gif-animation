#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk-1.3

target=animgifimage
target1=animgifimage-simple
target2=animgifimage-resize
src=intern
#opt=-pg

# copy files into FLTK repo & compile FLTK
if [ "$1" = "all" ]; then
	cp -av $src/FL/*.H $fltk/FL/.
	cp -av $src/src/*.cxx $fltk/src/.
	cp -av $src/src/*.H $fltk/src/.

	cwd=$(pwd)
	cd $fltk
	make
	cd $cwd
fi

# build the testprogram
g++ -Wall -pipe -pedantic -O3 $opt -o $target `$fltk/fltk-config --use-images --cxxflags` $src/test/$target.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt

g++ -Wall -pipe -pedantic -O3 $opt -o $target1 `$fltk/fltk-config --use-images --cxxflags` $src/test/$target1.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt

g++ -Wall -pipe -pedantic -O3 $opt -o $target2 `$fltk/fltk-config --use-images --cxxflags` $src/test/$target2.cxx `$fltk/fltk-config --use-images --ldflags` -g $opt
