//
//  Test program for Fl_Anim_GIF_Image::copy().
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>

Fl_Anim_GIF_Image *orig = 0;

class Canvas : public Fl_Box {
  typedef Fl_Box Inherited;
public:
  Canvas(int x_, int y_, int w_, int h_) :
    Inherited(x_, y_, w_, h_) {}
  virtual void resize(int x_, int y_, int w_, int h_) {
    Inherited::resize(x_, y_, w_, h_);
    if (image()) {
      Fl_Anim_GIF_Image *animgif = (Fl_Anim_GIF_Image *)image();
      image(0);
//      Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR); // very slow!!
      Fl_Anim_GIF_Image *copied = (Fl_Anim_GIF_Image *)orig->copy(w_, h_);
      animgif->canvas(0);
      animgif->stop();
      copied->canvas(this, Fl_Anim_GIF_Image::Start |
                     Fl_Anim_GIF_Image::DontResizeCanvas);
      copied->start();
      printf("resized to %d x %d\n", copied->w(), copied->h());
    }
  }
};

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(800, 600, "animated resize");

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

  // set initial size to fit into window
  canvas.resize(0, 0, win.w(), win.h());

  // check if loading succeeded
  printf("%s: valid: %d frames: %d\n", orig->name(), orig->valid(), orig->frames());
  if (orig->valid())
    return Fl::run();
}
