//
//  Demonstrates how playing an animated GIF file
//  under application control frame by frame if
//  this is needed.
//
//  Call optionally with parameter 'reverse[0,1] speed[0.01-10]'.
//  e.g. 'animgifimage-play <file> 1 0.5' to play file <file>
//       in reverse with half speed.
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>

static int frame = 0;
static double speed_factor = 3.; // slow down/speed up playback by factor
static bool reverse = false;  // true = play animation backwards

static void cb_anim(void *d_) {
  Fl_Anim_GIF_Image *animgif = (Fl_Anim_GIF_Image *)d_;

  // switch to next/previous frame
  if (reverse) {
    frame--;
    if (frame < 0) {
      frame = animgif->frames() - 1;
    }
  }
  else {
    frame++;
    if (frame >= animgif->frames()) {
      frame = 0;
    }
  }
  // set the frame (and update canvas)
  animgif->frame(frame);

  // setup timer for next frame
  if (animgif->delay(frame)) {
    Fl::repeat_timeout(animgif->delay(frame) / speed_factor, cb_anim, d_);
  }
}

int main(int argc_, char *argv_[]) {
  // setup play parameters from args
  reverse = argc_ > 2 ? atoi(argv_[2]) : false;
  speed_factor = argc_ > 3 ? atof(argv_[3]) : 3.;
  if (speed_factor < 0.01 || speed_factor > 100)
    speed_factor = 1.;

  Fl_Double_Window win(800, 600, "animated");

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Fl_Box canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();

  // create/load the animated gif and do *NOT* start it immediately.
  // We use the 'DontResizeCanvas' flag here to tell the
  // animation not to change the canvas size (which is the default).
  Fl_Anim_GIF_Image animgif(  /*name_=*/ argv_[1],
                            /*canvas_=*/ &canvas,
                             /*flags_=*/ Fl_Anim_GIF_Image::DontResizeCanvas);

  // check if loading succeeded
  printf("valid: %d frames: %d\n", animgif.valid(), animgif.frames());
  if (animgif.valid()) {
    printf("play%s with %f x speed\n", (reverse ? " reverse" : ""), speed_factor);
    animgif.frame(reverse ? animgif.frames() - 1 : frame);
    // setup first timeout, but check for zero-delay (normal GIF)!
    if (animgif.delay(frame)) {
      Fl::add_timeout(animgif.delay(frame) / speed_factor, cb_anim, &animgif);
    }
    return Fl::run();
  }
}
