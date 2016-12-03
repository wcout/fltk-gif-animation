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

The current status is promising - it works well, but needs some more tests.
I will stay with `Solution 2` for now and try to improve a few aspects, but
it may take some time.

*Update 2016/11/13:*

A few bugs have been fixed in `Solution 2` that have not been reflected in
`Solution 1` yet.

Issues remaining in `Solution 2`:

- How to make it usable as `Fl_Shared_Image`? One problem is, that `Fl_Shared_Image`
  works through file extension (or file header actually), so how to decide if to
  create an normal GIF or an animated GIF?

- Can the API for `Fl_Image()` derived classes be completed? E.g. how to implement
  `Fl_Animated_GIF_Image::color_average()`/`desaturate()` and `copy()`?

*Update 2016/11/19:*

  - `color_average()` and `desaturate()` work now
  - `copy()` is hard to implement - currently I have implemented a `resize()` method
    that can be used to _replace_ the current image with the resized one (useful
    to be called only once therefore)
  - also it works to run several instances of the _same_ animation simultaneously
    and to use it as tiles in `Fl_Tiled_Image`

Issues remaining:

- Round up the API
  - allow application controlled playback
  - specify repeat count
  - ...

*Update 2016/11/26:*

  - Complete code refactoring. Got rid of helper class `RGB_Image`.

  - `copy()` has also been implemented now. See usage with test program
    `animgifimage-resize`. Scaling large GIF's can be pretty slow though...

*Update 2016/12/03:*

  Finished *all* tasks. Solution 2 is complete now.

## Test

You can test the different solutions with the test programs in each folder.
For the `internal` approach you must have `FLTK` as source distribution to replace
the mentioned files. As said before `Solution 1` is not as advanced as `Solution 2`
because I did not develop it further after starting with `Solution 2`.

There are some crude scripts in the root directory that help to compile the test
programs under Linux.
(Don't use them unless you understand what you are doing...)

Put images into the `testsuite` folder and display all of them by starting
the test program with `-t`.

Or try the command line options for creating tiles, desaturate and color average.

Change the speed live using `+` and `-` keys.

Look at the minimal example program `animgifimage-simple.cxx` to get a quick idea how to
use the class.

## Resume

I am not entirely convinced by the concept of `Solution 2` after all, but it is
not bad either. On the one hand is _cute_ to use it as a `Fl_Image`, on the other
hand exactly therefore it seems inappropriate to put more and more playing
functionality into it.

Perhaps a generic `Fl_Animation` class (or whatever it may be called)
would make sense - and _one_ constructor for such a class could be an
`Fl_Animated_GIF_Image` (which would then not be needed to run the animation,
but only to supply the data). Or something like that.
