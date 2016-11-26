//
//  Test program for Fl_Anim_GIF_Image::copy().
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdio>

Fl_Anim_GIF_Image *orig = 0;

class Canvas : public Fl_Box {
  typedef Fl_Box Inherited;
public:
  Canvas(int x_, int y_, int w_, int h_) :
    Inherited(x_, y_, w_, h_) {}
  virtual void draw() {
    static const Fl_Color C1 = FL_WHITE;
    static const Fl_Color C2 = FL_GRAY;
    for (int y = 0; y < h(); y += 32) {
      for (int x = 0; x < w(); x += 32) {
        fl_color(x%64 ? y%64 ? C1 : C2 : y%64 ? C2 : C1);
        fl_rectf(x, y, 32, 32);
      }
    }
    Inherited::draw();
  }
  void do_resize(int W_, int H_) {
    if (image() && (image()->w() != W_ || image()->h() != H_)) {
      Fl_Anim_GIF_Image *animgif = (Fl_Anim_GIF_Image *)image();
      animgif->stop();
      image(0);
      // delete already copied images
      if (animgif != orig ) {
        delete animgif;
      }
      Fl_Anim_GIF_Image *copied = (Fl_Anim_GIF_Image *)orig->copy(W_, H_);
      copied->canvas(this, Fl_Anim_GIF_Image::Start |
                     Fl_Anim_GIF_Image::DontResizeCanvas);
      copied->start();
      printf("resized to %d x %d\n", copied->w(), copied->h());
    }
  }
  static void do_resize_cb(void *d_) {
    Canvas *c = (Canvas *)d_;
    c->do_resize(c->w(), c->h());
  }
  virtual void resize(int x_, int y_, int w_, int h_) {
    Inherited::resize(x_, y_, w_, h_);
    Fl::remove_timeout(do_resize_cb, this);
    Fl::add_timeout(0.1, do_resize_cb, this);
  }
};

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(800, 600, "test animated copy");

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Canvas canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();

  // create/load the animated gif and start it immediately.
  // We use the 'DontResizeCanvas' flag here to tell the
  // animation not to change the canvas size (which is the default).
  orig = new Fl_Anim_GIF_Image(/*name_=*/ argv_[1],
                             /*canvas_=*/ &canvas,
                              /*flags_=*/ Fl_Anim_GIF_Image::Start |
                                          Fl_Anim_GIF_Image::DontResizeCanvas);
  if (argc_ > 2) {
    Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);
    printf("Using bilinear scaling - can be slow!\n");
    // NOTE: this is *really* slow. Scaling the TrueColor test image
    //       to full HD desktop takes about 45 seconds!
  }

  // set initial size to fit into window
  canvas.resize(0, 0, win.w(), win.h());

  // check if loading succeeded
  printf("%s: valid: %d frames: %d\n", orig->name(), orig->valid(), orig->frames());
  if (orig->valid())
    return Fl::run();
}
