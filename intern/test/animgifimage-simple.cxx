//
//  Minimal program for displaying an animated GIF file
//  with the Fl_Anim_GIF_Image class.
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(800, 600, "animated");

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Fl_Box canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();

  // create/load the animated gif and start it immediately.
  // We use the 'DontResizeCanvas' flag here to tell the
  // animation not to change the canvas size (which is the default).
  Fl_Anim_GIF_Image animgif(  /*name_=*/ argv_[1],
                            /*canvas_=*/ &canvas,
                             /*flags_=*/ Fl_Anim_GIF_Image::Start |
                                         Fl_Anim_GIF_Image::DontResizeCanvas);

  // check if loading succeeded
  printf("valid: %d frames: %d\n", animgif.valid(), animgif.frames());
  if (animgif.valid())
    return Fl::run();
}
