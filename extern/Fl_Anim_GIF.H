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
              bool start_ = false, bool debug_ = false);
  ~Fl_Anim_GIF();
  int canvas_w() const;
  int canvas_h() const;
  int frames() const;
  int frame() const;
  Fl_Image *image() const;
  Fl_Image *image(int frame_) const;
  bool start();
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
  bool _valid;
  int _frame;	// current frame
  FrameInfo *_fi;
  bool _uncache;
};

#endif // Fl_Gif_Anim_H
