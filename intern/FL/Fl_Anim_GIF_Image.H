#ifndef Fl_Anim_Gif_Image_H
#define Fl_Anim_Gif_Image_H

class Fl_Image;
struct FrameInfo;
class Fl_Widget;

#include <FL/Fl_GIF_Image.H>

/**
 The Fl_Anim_GIF_Image class supports loading, caching,
 and drawing of animated Compuserve GIF<SUP>SM</SUP> images.
 The class loads all images contained in the file and animates
 them by cycling through them as defined by the delay times in
 the image file.

 You must supply an FLTK widget as "container" in order to see
 the animation by specifying it in the constructor or later
 using the canvas() method.
 */
class Fl_Anim_GIF_Image : public Fl_GIF_Image {
  typedef Fl_GIF_Image Inherited;
public:
  enum Flags {
    Start = 1,
    Debug = 2,
    DontResizeCanvas = 4,
    DontSetAsImage = 8,
    /**
     This flag indicates to the loader to try using less memory,
     by storing frame data not as canvas-sized images but as defined
     in the GIF file.
     The drawbacks are higher cpu usage during playback and maybe
     minor artefacts when resized.
     */
    OptimizeMemory = 16
  };
  /**
   The constructor create an new animated gif object from
   the given file.
   Optionally it applies the canvas() method after successful load.
   See canvas().
   If 'Start' is specified in the 'flags' parameter it calls start()
   after successful load.
   */
  Fl_Anim_GIF_Image(const char *name_, Fl_Widget *canvas_ = 0, unsigned short flags_ = 0);
  Fl_Anim_GIF_Image();
  virtual ~Fl_Anim_GIF_Image();
  /**
   The color_average() method applies the specified color_average
   to all frames of the animation.
   */
  virtual void color_average(Fl_Color c_, float i_);
  /**
   The virtual copy method makes a copy of the animated image
   and resizes all of its frame images to W x H using
   the current resize method.
   */
  virtual Fl_Image *copy(int W_, int H_);
  /**
   The desaturate() method applies desaturate() to all frames
   of the animation.
   */
  virtual void desaturate();
  virtual void draw(int x_, int y_, int w_, int h_, int cx_ = 0, int cy_ = 0);
  /**
   Return the width and height of the animation canvas as
   specified in the file header
   */
  int canvas_w() const;
  int canvas_h() const;
  /**
   Return the delay of frame 'frame_' [0-frames() -1] in seconds
   */
  double delay(int frame_) const;
  /**
   Set the delay of frame 'frame_' [0-frames() -1] in seconds
   */
  void delay(int frame_, double delay_);
  /**
   Return the number of frames.
   */
  int frames() const;
  /**
   Set the current frame in the range index [0-frames() -1]
   */
  void frame(int frame_);
  /**
   Return the current frame in the range index [0-frames() -1]
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
   Return the name of the played file as specified in the constructor.
   */
  const char *name() const;
  /**
   The canvas() method sets or gets the current widget, that
   is used for display of the frame images.
   The 'flags' parameter specifies wheather the canvas widget
   is resized to the animation dimensions and/or its image()
   method will be used to set the current frame image
   during animation.
   */
  void canvas(Fl_Widget *canvas_, unsigned short flags_ = 0);
  Fl_Widget *canvas() const;
  /**
   The start() method (re-)starts the playing of the frames.
   */
  bool start();
  /**
   The stop() method stops the playing of the frames.
   */
  bool stop();
  /**
   The resize() method resizes the image to the
   specified size, replacing the current image.
   */
  Fl_Anim_GIF_Image& resize(int W_, int H_);
  Fl_Anim_GIF_Image& resize(double scale_);
  /**
   The speed() method changes the playing speed
   to speed_ x original speed. E.g. to play at half
   speed call it with 0.5, for double speed with 2.
   */
  void speed(double speed_);
  double speed() const;
  /**
   Use uncache() to set or forbid image uncaching
   If uncaching is set, frame images are not offscreen cached for
   re-use and will be re-created every time they are displayed.
   This saves a lot of  memory on the expense of cpu usage and
   should to be carefully considered. Per default caching will
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
protected:
  bool next_frame();
  void clear_frames();
  /**
   The load() is used from the constructor to load
   the image from file.
   */
  bool load(const char *name_);
  void set_frame(int frame_);
private:
  static void cb_animate(void *d_);
private:
  char *_name;
  Fl_Widget *_canvas;
  bool _uncache;
  bool _valid;
  int _frame; // current frame
  double _speed;
  FrameInfo *_fi;
};

#endif // Fl_Anim_Gif_Image_H
