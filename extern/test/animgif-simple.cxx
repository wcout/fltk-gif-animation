//
//  Mininal program for displaying an animated GIF file
//  with the Fl_Anim_GIF class.
//
#include "../Fl_Anim_GIF.cxx"
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(800, 600, "animated");

  // create/load the animated gif and start it immediately
  Fl_Anim_GIF animgif( 10, 10, argv_[1] );

  win.end();
  win.resizable(win);
  win.show();

  // check if loading succeeded
  printf("valid: %d frames: %d, size: %d x %d\n",
    animgif.valid(), animgif.frames(), animgif.w(), animgif.h());
  if (animgif.valid()) {
    return Fl::run();
  }
}
