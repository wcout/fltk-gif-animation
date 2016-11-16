#ifndef Fl_Anim_Gif_Image_H
#define Fl_Anim_Gif_Image_H

class Fl_Image;
struct FrameInfo;
class Fl_Widget;

#include <FL/Fl_GIF_Image.H>

/**
 The Fl_Anim_GIF_Image class supports loading, caching,
 and drawing of animated Compuserve GIF<SUP>SM</SUP> images.
 The class loads all images contained in the file aand animates
 them by cycling through them as defined by the delay times in
 the image file.

 You must supply an FLTK widget as "container" in order to see
 the animation by specifying it in the constructor or later
 using the canvas() method.
 */
class Fl_Anim_GIF_Image : public Fl_GIF_Image {
  typedef Fl_GIF_Image Inherited;
public:
  Fl_Anim_GIF_Image(const char *name_, Fl_Widget *canvas_ = 0, bool start_ = false, bool debug_ = false);
  virtual ~Fl_Anim_GIF_Image();
  virtual void draw(int x_, int y_, int w_, int h_, int cx_ = 0, int cy_ = 0);
  int canvas_w() const;
  int canvas_h() const;
  double delay(int frame_) const;
  void delay(int frame_, double delay_);
  int frames() const;
  int frame() const;
  Fl_Image *image() const;
  Fl_Image *image(int frame_) const;
  const char *name() const;
  void canvas(Fl_Widget *canvas_, bool set_as_image = true);
  Fl_Widget *canvas() const;
  bool start();
  double speed() const;
  void speed(double speed_);
  void uncache(bool uncache_);
  bool uncache() const;
  bool valid() const;
protected:
  bool nextFrame();
  void clear_frames();
  bool load(const char *name_);
private:
  static void cb_animate(void *d_);
private:
  char *_name;
  Fl_Widget *_canvas;
  bool _uncache;
  bool _valid;
  int _frame;	// current frame
  double _speed;
  FrameInfo *_fi;
};

#endif // Fl_Anim_Gif_Image_H
