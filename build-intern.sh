#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk-1.4

targets="animgifimage animgifimage-simple animgifimage-resize animgifimage-play"
src=intern
#opt=-pg

fltkconfig=$fltk"/fltk-config"
if [ ! -d "$fltk" ]; then
	echo "You must specify a FLTK directory to patch"
#	exit 1
fi

if [ ! -f "$fltkconfig" ]; then
	echo $fltkconfig "not found"
	exit 1
fi

# copy files into FLTK repo & compile FLTK
if [ "$1" = "all" ]; then
	cp -av $src/FL/*.H $fltk/FL/.
	cp -av $src/src/*.cxx $fltk/src/.
	cp -av $src/src/*.H $fltk/src/.

	cwd=$(pwd)
	cd $fltk
	touch FL/Fl_GIF_Image.H
	touch FL/Fl_Anim_GIF_Image.H
	make
	cd $cwd
fi

# build the testprogram
for i in $targets
do
	echo "Building" $i
	g++ -Wall -pipe -pedantic -O3 $opt -o $i `$fltkconfig --use-images --cxxflags` $src/test/$i.cxx `$fltkconfig --use-images --ldflags` -g $opt
done
