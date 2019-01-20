//
//  Minimal program for displaying an animated GIF file
//  with the Fl_Anim_GIF class.
//
#include "../Fl_Anim_GIF.cxx"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>

int main(int argc_, char *argv_[]) {
  Fl_Double_Window win(800, 600, "animated");

  // create/load the animated gif and start it immediately
  Fl_Anim_GIF animgif(0, 0, win.w(), win.h(), argv_[1]);
//  animgif.autoresize(true); // uncomment to resize image to canvas dimensions

  win.end();
  win.resizable(win);
  win.show();

  // check if loading succeeded
  printf("valid: %d frames: %d, size: %d x %d\n",
    animgif.valid(), animgif.frames(), animgif.canvas_w(), animgif.canvas_h());
  if (animgif.valid()) {
    // print frame information
    for (int i=0; i<animgif.frames(); i++) {
      printf("  #%d: %d/%d+%dx%d, delay: %f\n", i+1,
        animgif.frame_x(i), animgif.frame_y(i), animgif.frame_w(i), animgif.frame_h(i), animgif.delay(i));
    }
    return Fl::run();
  }
}
