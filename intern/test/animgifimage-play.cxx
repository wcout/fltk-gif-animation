//
//  Demonstrates how playing an animated GIF file
//  under application control frame by frame if
//  this is needed.
//  Also demonstrates how to use a single animation
//  object to load multiple animations.
//
//  animgifimage <file> [-r] [-s speed_factor]
//
//  Multiple files can be specified e.g. testsuite/*
//
//  Use keys '+'/'-'/Enter to change speed, ' ' to pause.
//  Right key changes to next frame in paused mode.
//  'n' changes to next file, 'r' toggles reverse play.
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <cstdio>
#include <cstring>

static double speed_factor = 1.; // slow down/speed up playback by factor
static bool reverse = false;  // true = play animation backwards
static bool paused = false;  // flag for paused animation
static bool frame_info = false; // flag to update current frame info in title
static Fl_Anim_GIF_Image animgif; // the animation object
static char **argv = 0; // copy of main() argv[]
static int argc = 0;    // copy of main() argc
static int current_arg = 0; // current index in argv[]

static int next_arg() {
  while (1) {
    current_arg++;
    if (current_arg >= argc) {
      current_arg = 1;
    }
    if (argv[current_arg]) break;
  }
  return current_arg;
}

static const char *next_file() {
  while (argv[next_arg()][0] == '-') ;
  return argv[current_arg];
}

static void set_title() {
  char buf[200];
  char fi[50];
  if (frame_info)
    snprintf(fi, sizeof(fi), "frame %d/%d", animgif.frame() + 1, animgif.frames());
  else
    snprintf(fi, sizeof(fi), "%d frames", animgif.frames());
  snprintf(buf, sizeof(buf), "%s (%s) x %3.2f %s%s",
    argv[current_arg], fi,
    speed_factor, reverse ? "reverse" : "",
    paused ? " PAUSED" : "");

  Fl::first_window()->copy_label(buf);
}

static void cb_anim(void *d_) {
  Fl_Anim_GIF_Image *animgif = (Fl_Anim_GIF_Image *)d_;
  int frame(animgif->frame());

  // switch to next/previous frame
  if (reverse) {
    animgif->canvas()->window()->redraw();
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
  if (!paused && animgif->delay(frame)) {
    Fl::repeat_timeout(animgif->delay(frame) / speed_factor, cb_anim, d_);
  }
  if (frame_info)
    set_title();
}

static void next_frame() {
  cb_anim(&animgif);
}

static void toggle_pause() {
  paused = !paused;
  set_title();
  if (paused)
    Fl::remove_timeout(cb_anim, &animgif);
  else
    next_frame();
  set_title();
}

static void toggle_info() {
  frame_info = !frame_info;
  set_title();
}

static void toggle_reverse() {
  reverse = !reverse;
  set_title();
}

static void change_speed(int dir_) {
  if (dir_> 0) {
    speed_factor += (speed_factor < 1) ? 0.01 : 0.1;
    if (speed_factor > 100)
      speed_factor = 100.;
  }
  else if (dir_ < 0) {
    speed_factor -= (speed_factor > 1) ? 0.1 : 0.01;
    if (speed_factor < 0.01)
      speed_factor = 0.01;
  }
  else {
    speed_factor = 1.;
  }
  set_title();
}

static void load_next() {
  Fl::remove_timeout(cb_anim, &animgif);
  paused = false;
  animgif.load(next_file());
  animgif.canvas()->window()->redraw();
  // check if loading succeeded
  printf("valid: %d frames: %d\n", animgif.valid(), animgif.frames());
  if (animgif.valid()) {
    printf("play '%s'%s with %3.2f x speed\n", animgif.name(),
       (reverse ? " reverse" : ""), speed_factor);
    animgif.frame(reverse ? animgif.frames() - 1 : 0);
    // setup first timeout, but check for zero-delay (normal GIF)!
    if (animgif.delay(animgif.frame())) {
      Fl::add_timeout(animgif.delay(animgif.frame()) / speed_factor, cb_anim, &animgif);
    }
  }
  set_title();
}

static int events(int event_) {
  if (event_ == FL_SHORTCUT) {
    if (Fl::event_key() == '+')
      change_speed(1);
    else if (Fl::event_key() == '-')
      change_speed(-1);
    else if (Fl::event_key() == FL_Enter)
      change_speed(0);
    else if (Fl::event_key() == 'n')
      load_next();
    else if (Fl::event_key() == 'i')
      toggle_info(); // Note: this can raise cpu usage considerably!
    else if (Fl::event_key() == 'r')
      toggle_reverse();
    else if (Fl::event_key() == ' ')
      toggle_pause();
    else if (paused && Fl::event_key() == FL_Right)
      next_frame();
    else
      return 0;
  }
  return 1;
}

int main(int argc_, char *argv_[]) {
  // setup play parameters from args
  argv = argv_;
  argc = argc_;
  int n = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-r"))
      reverse = !reverse;
    else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
      i++;
      speed_factor = atof(argv[i]);
      argv[i] = 0;
    }
    else if (argv[i][0] != '-') {
      n++;
      continue;
    }
  }
  if (!n) {
     fprintf(stderr, "You must specify one or more image files!\n");
     exit(0);
  }
  if (speed_factor < 0.01 || speed_factor > 100)
    speed_factor = 1.;

  Fl_Double_Window win(800, 600);

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Fl_Box canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();
  Fl::add_handler(events);

  // use the 'DontResizeCanvas' flag to tell the animation
  // not to change the canvas size (which is the default).
  unsigned short flags = Fl_Anim_GIF_Image::DontResizeCanvas;
//  flags |= Fl_Anim_GIF_Image::Debug|Fl_Anim_GIF_Image::Log;
  animgif.canvas(&canvas, flags);

  load_next();
  return Fl::run();
}
