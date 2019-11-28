//
//  A real world example
//  with the Fl_Anim_GIF class.
//
#include "../Fl_Anim_GIF.cxx"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <cstdio>

class Fl_Anim_Button : public Fl_Button {
public:
  Fl_Anim_Button(int x_, int y_, int w_, int h_, const char *gif_, const char *l_ = 0 ) :
    Fl_Button(x_, y_, w_, h_, l_) {
    _anim = new Fl_Anim_GIF(x_, y_, 0, 0, gif_);
    fit();
  }
  void fit() {
    _orig_w = _anim->canvas_w();
    _orig_h = _anim->canvas_h();
    double prop = (double)_orig_h / (double)_orig_w;
    _anim->resize(w(), w() * prop);
    _anim->autoresize(true);
    resize(x(), y(), _anim->canvas_w(), _anim->canvas_h());
  }
  virtual void draw() {
    // redraw both widgets
    Fl_Button::draw();
    _anim->draw();
  }
  int orig_w() const { return _orig_w; }
  int orig_h() const { return _orig_h; }
  Fl_Anim_GIF *anim() const { return _anim; }
  void anim(Fl_Anim_GIF *anim_) {
    if (anim_ == _anim) return;
    parent()->remove(*_anim);
    delete _anim;
    _anim = anim_;
    parent()->insert(*_anim, 0);
    fit();
  }
  ~Fl_Anim_Button() {
    delete _anim;
  }
private:
  Fl_Anim_GIF *_anim;
  int _orig_w;
  int _orig_h;
};

static void cb_start_stop(Fl_Widget *w_, void *d_) {
  Fl_Anim_Button *b = (Fl_Anim_Button *)w_;
  if (b->anim()->playing()) {
    b->anim()->stop();
    b->anim()->frame(0);
    b->copy_label("Click to start");
  }
  else {
    b->anim()->start();
    b->copy_label("Copying..");
  }
}

static void cb_info(Fl_Widget *w_, void *d_) {
  Fl_Anim_Button *b = (Fl_Anim_Button *)w_;
  if (Fl::event_button() > 1) {
    const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
    if (!filename) return;
    b->anim()->load(filename);
    b->fit();
    b->copy_label(b->anim()->name());
    b->anim()->start();
    return;
  }
  // just show off some features..
  Fl_Anim_GIF *orig = b->anim()->copy(); // make a 1:1 backup copy of animation
  orig->stop(); // stop backup animation
  b->anim()->desaturate(); // show original desaturated
  fl_message("%d x %d (original: %d x %d)\n%d frames",
    b->anim()->canvas_w(), b->anim()->canvas_h(),
    b->orig_w(), b->orig_h(),
    b->anim()->frames());
  b->anim(orig); // replace with backup
  b->anim()->start(); // start again
}

static void cb_lighter(Fl_Widget *w_, void *d_) {
  Fl_Anim_GIF *ag = ((Fl_Anim_Button *)d_)->anim();
  ag->color_average(FL_WHITE, -0.9);
}

static void cb_darker(Fl_Widget *w_, void *d_) {
  Fl_Anim_GIF *ag = ((Fl_Anim_Button *)d_)->anim();
  ag->color_average(FL_BLACK, -0.9);
}

int main(int argc_, char *argv_[]) {
  fl_register_images();

  Fl_Double_Window win(300, 300, "animation demo");
  Fl_Anim_GIF bg(0, 0, win.w(), win.h(), "testsuite/worm.gif");
  bg.autoresize(true);

  Fl_Anim_Button but1(20, 10, 260, 50, "testsuite/filecopy.gif");
  but1.callback(cb_start_stop);
  but1.do_callback();

  Fl_Anim_Button but2(100, 100, 100, 100, "testsuite/banana.gif");
  but2.align(FL_ALIGN_BOTTOM);
  but2.copy_label(but2.anim()->name());
  but2.callback(cb_info);
  but2.anim()->copy_tooltip("left click: show info\nright click: load image");

  Fl_Button lighter(60,130,40,40,"@8>");
  lighter.callback(cb_lighter, &but2);
  lighter.copy_tooltip("lighter");
  Fl_Button darker(200,130,40,40,"@2>");
  darker.callback(cb_darker, &but2);
  darker.copy_tooltip("darker");

  win.resizable(win);
  win.end();
  win.show(argc_, argv_);

  return Fl::run();
}
