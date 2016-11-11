# FLTK GIF Animation

[FLTK](http://www.fltk.org/) does not support loading/displaying animated GIF files
currently. There has been already some third party work on this subject here:

[KleineBre's FLTK stuff](https://ringbreak.dnd.utwente.nl/~mrjb/fltk/)

Unfortunately there are many animated GIF's that don't play well with this solution.
So I tried to fix the bugs, but after much disappointment decided to start from scratch. I found
[The GIFLIB project](http://giflib.sourceforge.net/), an easy to use library to reduce
the burden of decoding the GIF's to get more quickly (and safely) to the real work of designing
the interface to FLTK.

## Solution 1: External Widget

My first solution was an external Widget `Fl_Anim_GIF`, derived from the basic FLTK widget `Fl_Box`.
This solution works well and can be found in the folder `extern` (It will probably be not
developed further - see below).

But Kleine Bre's design approach to expand the `Fl_GIF_Image` class with animation capabilties
seems advantegeous. On the other hand I did not want to have `Fl_GIF_Image` load animated files
whithout being asked to. There was also a technical reason not to extend `Fl_GIF_Image`:
`Fl_GIF_Image` converts the images to `XPM` format and the transparent color is always put
at the first entry of the color table. This leads to problems when other frames of the GIF
have a different transparent color index.

## Solution 2: Replacement widget for current GIF class

So I came up with the idea to derive a class `Fl_Anim_GIF_Image` from `Fl_GIF_Image` that
stores its images directly in RGB-format without an intermittent `XPM`.

This solution can be found in the folder `intern`:

The files there can be copied directly into the FLTK tree to replace the current GIF code.

`Fl_GIF_Image.cxx` contains the new GIF decoding algorithm using `GIFLIB`.

`fl_gif_private.H` 'hides' and holds together the `GIFLIB` code (or the relevant parts of
`GIFLIB`'s code for decoding).

`Fl_Anim_GIF_Image.H` declares the new image class `Fl_Anim_GIF_Image`.

## Current status

The current status is promising - it works well, but needs some more tuning.
I will try to improve some aspects, but it may take some time.

## Test

You can test the different solutions with the test programs in each folder.
For the `internal` approach you must have `FLTK` as source distribution to replace
the mentioned files.

There are some crude scripts in the root directory that help to compile the test
programs under Linux.
(Don't use them unless you understand what you are doing...)

Put images into the `testsuite` folder and display all of them by starting
the test program with `-t`.
