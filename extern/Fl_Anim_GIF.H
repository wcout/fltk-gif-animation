//
// Copyright 2016-2017 Christian Grabner <wcout@gmx.net>
//
// Fl_Anim_GIF widget - FLTK animated GIF widget.
//
// Fl_Anim_GIF is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 3 of the License, or
// (at your option) any later version.
//
// Fl_Anim_GIF is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY;  without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details:
// http://www.gnu.org/licenses/.
//
#ifndef Fl_Anim_GIF_H
#define Fl_Anim_GIF_H

class Fl_Image;
class RGB_Image;
struct FrameInfo;

#include <FL/Fl_Box.H>

/**
 The Fl_Anim_GIF class supports loading, caching,
 and drawing of animated Compuserve GIF<SUP>SM</SUP> images.
 The class loads all images contained in the file and animates
 them by cycling through them as defined by the delay times in
 the image file.
 */
class Fl_Anim_GIF : public Fl_Box {
  typedef Fl_Box Inherited;
public:
  /**
   The constructor creates an new animated gif object from
   the given file.
   If 'start_=true' is specified as parameter (which is the
   default), it calls start() after successful load.
   If 'optimize_mem_=true' is specified as parameter (default: false)
   the loader tries using less memory, by storing frame data
   not as canvas-sized images but using the sizes defined in
   the GIF file.
   The drawbacks are higher cpu usage during playback and maybe
   minor artefacts when resized.
   The 'debug_' flag has been used during devolpment of this class,
   and kept afterwards, as it may still be useful. It is off by
   default.
   */
  Fl_Anim_GIF(int x_, int y_, int w_, int h_, const char *name_ = 0,
              bool start_ = true, bool optimize_mem_ = false, bool debug_ = false);
  /**
   The destructor stops a running animation and releases all
   resources.
   */
  ~Fl_Anim_GIF();
  /**
   Return the width and height of the animation canvas as
   specified in the file header
   */
  int canvas_w() const;
  /**
   Return the width and height of the animation canvas as
   specified in the file header
   */
  int canvas_h() const;
  /**
   The color_average() method applies the specified color_average
   to all frames of the animation.
   */
  virtual void color_average(Fl_Color c_, float i_);
  /**
   The virtual copy() method makes a copy of the animated image
   and resizes all of its frame images to W x H using
   the current resize method.
   */
  virtual Fl_Anim_GIF *copy(int W_, int H_);
  /**
   The desaturate() method applies desaturate() to all frames
   of the animation.
   */
  virtual void desaturate();
  /**
   Return the delay of frame 'frame_' `[0-frames() -1]` in seconds
   */
  double delay(int frame_) const;
  /**
   Set the delay of frame 'frame_' `[0-frames() -1]` in seconds
   */
  void delay(int frame_, double delay_);
  /**
   Return the number of frames.
   */
  int frames() const;
  /**
   Set the current frame in the range index `[0-frames() -1]`
   */
  void frame(int frame_);
  /**
   Return the current frame in the range index `[0-frames() -1]`
   or -1 if the image has no frames.
   */
  int frame() const;
  /**
   Return the current frame image.
   */
  Fl_Image *image() const;
  /**
   Return the frame image of frame 'frame_'
   */
  Fl_Image *image(int frame_) const;
  /**
   The resize() method resizes the image to the
   specified size, replacing the current image.
   */
  Fl_Anim_GIF& resize(int W_, int H_);
  Fl_Anim_GIF& resize(double scale_);
  /**
   The load() method is either used from the constructor to load
   the image from the given file, or to re-load an existing
   animation from another file.
   */
  bool load(const char *name_);
  /**
   The speed() method changes the playing speed
   to speed_ x original speed. E.g. to play at half
   speed call it with 0.5, for double speed with 2.
   */
  void speed(double speed_);
  double speed() const;
  /**
   The start() method (re-)starts the playing of the frames.
   */
  bool start();
  /**
   The stop() method stops the playing of the frames.
   */
  bool stop();
  /**
   Use uncache() to set or forbid frame image uncaching.
   If frame uncaching is set, frame images are not offscreen cached
   for re-use and will be re-created every time they are displayed.
   This saves a lot of  memory on the expense of cpu usage and
   should to be carefully considered. Per default frame caching will
   be done.
   */
  void uncache(bool uncache_);
  /**
   Return the active uncache() setting.
   */
  bool uncache() const;
  /**
   The valid() method returns if the class has
   successfully loaded and the image has at least
   one frame.
   */
  bool valid() const;
  /**
   The debug() method returns if the class has
   set the debug option in it's constructor.
   */
  bool debug() const;
protected:
  bool next_frame();
  void clear_frames();
  void set_frame(int frame_);
private:
  static void cb_animate(void *d_);
private:
  Fl_Anim_GIF();
  virtual void draw();
  bool _valid;
  bool _uncache;
  bool _stopped;
  int _frame; // current frame
  double _speed;
  FrameInfo *_fi;
};

#endif // Fl_Anim_GIF_H
