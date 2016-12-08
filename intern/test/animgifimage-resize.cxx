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
    // draw a transparency grid as background
    static const Fl_Color C1 = fl_rgb_color(0xcc, 0xcc, 0xcc);
    static const Fl_Color C2 = fl_rgb_color(0x88, 0x88, 0x88);
    static const int SZ = 8;
    for (int y = 0; y < h(); y += SZ) {
      for (int x = 0; x < w(); x += SZ) {
        fl_color(x%(SZ * 2) ? y%(SZ * 2) ? C1 : C2 : y%(SZ * 2) ? C2 : C1);
        fl_rectf(x, y, 32, 32);
      }
    }
    // draw the current image frame over the grid
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
      window()->cursor(FL_CURSOR_DEFAULT);
      if (!copied->valid()) { // check success of copy
        Fl::warning("Fl_Anim_GIF_Image::copy() %d x %d failed", W_, H_);
      }
      else {
        printf("resized to %d x %d\n", copied->w(), copied->h());
      }
      copied->canvas(this, Fl_Anim_GIF_Image::Start |
                     Fl_Anim_GIF_Image::DontResizeCanvas);
      copied->start();
    }
  }
  static void do_resize_cb(void *d_) {
    Canvas *c = (Canvas *)d_;
    c->do_resize(c->w(), c->h());
  }
  virtual void resize(int x_, int y_, int w_, int h_) {
    Inherited::resize(x_, y_, w_, h_);
    // decouple resize event from actual resize operation
    // to avoid lockups..
    Fl::remove_timeout(do_resize_cb, this);
    Fl::add_timeout(0.1, do_resize_cb, this);
    window()->cursor(FL_CURSOR_WAIT);
  }
};

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(640, 480, "test animated copy");

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Canvas canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();

  // create/load the animated gif and start it immediately.
  // We use the 'DontResizeCanvas' flag here to tell the
  // animation not to change the canvas size (which is the default).
  int flags = Fl_Anim_GIF_Image::Start | Fl_Anim_GIF_Image::DontResizeCanvas;
  if (argc_ > 3) {
    flags |= Fl_Anim_GIF_Image::OptimizeMemory;
    printf("Using memory optimization (if image supports)\n");
  }
  orig = new Fl_Anim_GIF_Image(/*name_=*/ argv_[1],
                             /*canvas_=*/ &canvas,
                              /*flags_=*/ flags );
  if (argc_ > 2) {
    Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);
    printf("Using bilinear scaling - can be slow!\n");
    // NOTE: this is *really* slow. Scaling the TrueColor test image
    //       to full HD desktop takes about 45 seconds!
  }

  // set initial size to fit into window
  double ratio = orig->valid() ? (double)orig->w() / orig->h() : 1;
  int W = win.w() - 40;
  int H = (double)W / ratio;
  win.size(W, H);

  // check if loading succeeded
  printf("%s: valid: %d frames: %d\n", orig->name(), orig->valid(), orig->frames());
  if (orig->valid()) {
    int n = 0;
    for (int i = 0; i < orig->frames(); i++) {
      if (orig->x(i) != 0 || orig->y(i) != 0) n++;
    }
    printf("image has %d optimized frames\n", n);
    return Fl::run();
  }
  else
    printf("Usage:\n%s filename [scale mode bilinear: any value] [minimal update: any value]\n", argv_[0]);
}
