//
// Copyright 2016-2019 Christian Grabner <wcout@gmx.net>
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
struct GIF_WHDR;

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

  class FrameInfo;

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
              bool start_ = true, bool optimize_mem_ = false, int debug_ = 0);
  Fl_Anim_GIF(int x_, int y_, const char *name_ = 0,
              bool start_ = true, bool optimize_mem_ = false, int debug_ = 0);
  /**
   The destructor stops a running animation and releases all
   resources.
   */
  ~Fl_Anim_GIF();
  /**
   The autoresize flag sets the behaviour, when the underlying Fl_Box widget is resized
   by FLTK (e.g. when the window is resizable and is resized by the user). If the value
   is on, the image is auto-resized according the new Fl_Box dimensions.
   The default is off i.e. the image keeps it's dimensions.
   */
  void autoresize(bool autoresize);
  bool autoresize() const { return _autoresize; }
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
  Fl_Anim_GIF *copy();
  virtual Fl_Anim_GIF *copy(int W_, int H_);
  /**
   The debug() method returns if the class has
   set the debug option in it's constructor.
   */
  int debug() const;
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
   Return the frame position of frame 'frame_'
   Usefull only if loaded with 'optimize_mem' and
   the animation also has size optimized frames.
   */
  int frame_x(int frame_) const;
  int frame_y(int frame_) const;
  /**
   Return the frame dimensions of frame 'frame_'.
   Usefull only if loaded with 'optimize_mem' and
   the animation also has size optimized frames.
   */
  int frame_w(int frame_) const;
  int frame_h(int frame_) const;
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
   The loop flag can be used to (dis-)allow loop count.
   If set (which is the default), the animation will be
   stopped after the number of repeats specified in the
   GIF file (typically this count is set to 'forever' anyway).
   If cleared the animation will always be 'forever',
   regardless of what is specified in the GIF file.
   */
  static bool loop;
  /**
   The min_delay value can be used to set a minimum value
   for the frame delay for playback. This is to prevent
   cpu hogs caused by images with very low delay rates.
   This is a global value for all Fl_Anim_GIF objects.
   */
  static double min_delay;
  /**
   Return the name of the played file as specified in the constructor.
   */
  const char *name() const;
  bool playing() const { return _valid && !_stopped; }
  /**
   The speed() method changes the playing speed
   to speed_ x original speed. E.g. to play at half
   speed call it with 0.5, for double speed with 2.
   */
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
public:
  /**
   draw() and resize() are public, in order to allow
   composition.
  */
  virtual void draw();
  virtual void resize(int x_, int y_, int w_, int h_);
protected:
  bool next_frame();
  void clear_frames();
  void set_frame(int frame_);
private:
  static void cb_animate(void *d_);
  static void cb_gl_frame(void *ctx_, GIF_WHDR *whdr_);
  static void cb_gl_extension(void *ctx_, GIF_WHDR *whdr_);
  void scale_frame(int frame_ = -1);
private:
  Fl_Anim_GIF();
  void onFrameLoaded(GIF_WHDR &whdr_);
  void onExtensionLoaded(GIF_WHDR &whdr_);
  void _init(const char*name_, bool start_, bool optimize_mem_, int debug_);
private:
  char *_name;
  bool _valid;
  bool _uncache;
  bool _stopped;
  int _frame; // current frame
  double _speed;
  bool _autoresize;
  FrameInfo *_fi;
};

#endif // Fl_Anim_GIF_H
