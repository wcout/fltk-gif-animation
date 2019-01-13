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

class AnimButton : public Fl_Button {
public:
  AnimButton(int x_, int y_, int w_, int h_, const char *gif_, const char *l_ = 0 ) :
    Fl_Button(x_, y_, w_, h_, l_) {
    _anim = new Fl_Anim_GIF(x_, y_, 0, 0, gif_);
    fit();
  }
  void fit() {
    _orig_w = _anim->canvas_w();
    _orig_h = _anim->canvas_h();
    if (_anim->w() > w()) {
      int prop = h() / w();
      _anim->resize(w(), h() * prop);
    }
    resize(x(), y(), _anim->w(), _anim->h());
    _anim->autoresize(true);
  }
  int orig_w() const { return _orig_w; }
  int orig_h() const { return _orig_h; }
  Fl_Anim_GIF *anim() const { return _anim; }
  ~AnimButton() {
    delete _anim;
  }
private:
  Fl_Anim_GIF *_anim;
  int _orig_w;
  int _orig_h;
};

static void cb_start_stop(Fl_Widget *w_, void *d_) {
  AnimButton *b = (AnimButton *)w_;
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
  AnimButton *b = (AnimButton *)w_;
  if (Fl::event_button() > 1) {
    const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
    if (!filename) return;
    b->anim()->load(filename);
    b->fit();
    b->copy_label(b->anim()->name());
    return;
  }
  fl_message("%d x %d (original: %d x %d)\n%d frames",
    b->anim()->canvas_w(), b->anim()->canvas_h(),
    b->orig_w(), b->orig_h(),
    b->anim()->frames());
}

int main(int argc_, char *argv_[]) {
  fl_register_images();

  Fl_Double_Window win(300, 300, "animation demo");

  AnimButton but1( 20, 10, 200, 50, "testsuite/filecopy.gif");
  but1.callback(cb_start_stop);
  but1.do_callback();

  AnimButton but2( 100, 100, 100, 100, "testsuite/banana.gif" );
  but2.align(FL_ALIGN_BOTTOM);
  but2.copy_label(but2.anim()->name());
  but2.callback(cb_info);

  win.resizable(win);
  win.end();
  win.show();

  return Fl::run();
}