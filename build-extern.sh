#!/bin/sh

# put the path to your FLTK repository here
fltk=../fltk/

targets="animgif animgif-resize animgif-simple"

src=extern
#opt=-pg
# Use shared image scaling feature (which uses fast XRender scaling on X11)
opt=-DUSE_SHIMAGE_SCALING

# build the test programs
fltkconfig=$fltk"fltk-config"
for i in $targets
do
	echo "Building" $i
	g++ -Wall -pipe -pedantic -O3 $opt -o $i `$fltkconfig --use-images --cxxflags` $src/test/$i.cxx `$fltkconfig --use-images --ldflags` -g $opt
done
