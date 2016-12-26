//
// "$Id: Fl_GIF_Image.H 10732 2015-05-23 23:42:26Z matt $"
//
// GIF image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

/* \file
   Fl_GIF_Image widget . */

#ifndef Fl_GIF_Image_H
#define Fl_GIF_Image_H
#  include "Fl_Pixmap.H"

/**
 The Fl_GIF_Image class supports loading, caching,
 and drawing of Compuserve GIF<SUP>SM</SUP> images. The class
 loads the first image and supports transparency.
 */
class FL_EXPORT Fl_GIF_Image : public Fl_Pixmap {

public:

  Fl_GIF_Image(const char* filename);
  Fl_GIF_Image();
  virtual ~Fl_GIF_Image();
  /**
   The static method is_animated() tests if a GIF file
   contains more than one image. As there is no information
   about the number of images a GIF file contains available
   in the GIF header, this is achieved by first reading one
   image of the file and if that succeeds trying to read a
   second image and returning the result of this operation.
  */
  static bool is_animated(const char *name_);

  /**
   This variable can be set/unset from the application
   to allow/disallow automatic creation of animated GIF
   objects from the shared image module. If this variable
   is on and a file with GIF extension is loaded through
   the shared image interface, an Fl_Anim_GIF_Image is
   created, otherwise a normal Fl_GIF_Image.
   The default is off.
  */
  static bool animate;
protected:

  /**
   The protected constructor is used by Fl_Anim_GIF_Image().
   It's sole purpose is to pass a flag to the load() method,
   which signals the loading of an animated GIF. This makes
   load() leave the file descriptor open, in order to be
   usable for the derived class.
  */
  Fl_GIF_Image(const char* filename, bool anim_);
  bool load(const char* filename);
  void close_gif_file();
  void *read_next_image();
  bool _anim;
  void *gif_handle;
};

#endif

//
// End of "$Id: Fl_GIF_Image.H 10732 2015-05-23 23:42:26Z matt $".
//
