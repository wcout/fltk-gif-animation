#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk

targets="animgifimage animgifimage-simple animgifimage-resize animgifimage-play buttons pixmap"
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
# uncomment for newer FLTK version using image reader class
#	cp -av $src/FL/v2/*.H $fltk/FL/.
#	cp -av $src/src/v2/*.cxx $fltk/src/.
	cp -av gif_load/gif_load.h $fltk/src/.

	cwd=$(pwd)
	cd $fltk
	touch FL/Fl_GIF_Image.H
	touch FL/Fl_Anim_GIF_Image.H
	make
	cd $cwd
else
	echo "Building only the test programs - use '$0 all' to compile FLTK widget"
fi

# build the testprogram
for i in $targets
do
	echo "Building" $i
	g++ -I gif_load -Wall -pipe -pedantic -O3 $opt -o $i `$fltkconfig --use-images --cxxflags` $src/test/$i.cxx `$fltkconfig --use-images --ldflags` -g $opt
done
