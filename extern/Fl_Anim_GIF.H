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
#ifndef Fl_Gif_Anim_H
#define Fl_Gif_Anim_H

class Fl_Image;
class RGB_Image;
struct FrameInfo;

#include <FL/Fl_Box.H>

class Fl_Anim_GIF : public Fl_Box {
  typedef Fl_Box Inherited;
public:
  Fl_Anim_GIF(int x_, int y_, int w_, int h_, const char *name_ = 0,
              bool start_ = true, bool optimize_mem_ = false, bool debug_ = false);
  ~Fl_Anim_GIF();
  int canvas_w() const;
  int canvas_h() const;
  /**
   The color_average() method applies the specified color_average
   to all frames of the animation.
   */
  virtual void color_average(Fl_Color c_, float i_);
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
   The start() method (re-)starts the playing of the frames.
   */
  bool start();
  /**
   The stop() method stops the playing of the frames.
   */
  bool stop();
  void uncache(bool uncache_);
  bool uncache() const;
  bool valid() const;
protected:
  bool next_frame();
  void clear_frames();
  void set_frame(int frame_);
private:
  static void cb_animate(void *d_);
private:
  virtual void draw();
  bool _valid;
  int _frame;	// current frame
  FrameInfo *_fi;
  bool _uncache;
};

#endif // Fl_Gif_Anim_H
