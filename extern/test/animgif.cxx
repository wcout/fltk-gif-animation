//
// Copyright 2016-2019 Christian Grabner <wcout@gmx.net>
//
// Testprogram for the Fl_Anim_GIF widget (FLTK animated GIF widget).
//
// Fl_Anim_GIF is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation,  either version 3 of the License, or
// (at your option) any later version.
//
// Fl_Anim_GIF is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY;  without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details:
// http://www.gnu.org/licenses/.
//
#include "../Fl_Anim_GIF.cxx" // include the widget code directly

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl.H>

//#define BACKGROUND FL_RED	// use this to see transparent parts better
#define BACKGROUND FL_GRAY
//#define BACKGROUND FL_BLACK

static bool CopyTest = false;
static bool CallbackTest = false;

static void quit_cb(Fl_Widget* w_, void*) {
  exit(0);
}

static void callback(Fl_Widget *o_, void *d_) {
  // this is called each time the frame image changes (to the next frame)
  Fl_Anim_GIF *animgif = (Fl_Anim_GIF *)o_;
  printf("'%s': displaying frame %d/%d, delay %fs\n",
    animgif->label(),
    animgif->frame()+1, animgif->frames(), animgif->delay(animgif->frame()));
  // stop animation after one complete pass
  if (animgif->frame()+1 == animgif->frames())
  {
    animgif->stop();
    printf("'%s': stopped after one pass\n", animgif->label());
  }
}

static int global_key_handler(int e_) {
  // intercept keys '+' and '-'
  if (e_ != FL_SHORTCUT)
    return 0;
  bool faster = Fl::event_key('+');
  bool slower = Fl::event_key('-');
  bool reset = Fl::event_key('0');
  if (!faster && !slower && !reset)
    return 0;

  // change speed of current window's animation
  Fl_Widget *wgt = Fl::focus(); // actually gets the window here in this program!
  if (wgt) {
    Fl_Window *win = (Fl_Window *)wgt;
    Fl_Anim_GIF *animgif = (Fl_Anim_GIF *)win->child(0); // first child is animation
    double speed = animgif->speed();
    if (reset) {
      animgif->stop();
      speed = 1.;
      animgif->frame(0);
    }
    if (faster) {
      if (speed < 10) speed += 0.1;
    }
    else if (slower) {
      if (speed > 0.1) speed -= 0.1; // note: get's down to 0. (rounding?)
    }
    animgif->speed(speed);
    printf("speed '%s': %f\n", animgif->label(), animgif->speed());
    animgif->start(); // call start() for sure, because at 0.0 animation was stopped
  }
  return 1;
}

static Fl_Window *openFile(const char *name_, bool optimize_mem_, int debug_, bool close_ = false) {
  Fl_Double_Window *win = new Fl_Double_Window(100, 100);
  win->color(BACKGROUND);
  if (close_)
    win->callback(quit_cb);
  printf("Loading '%s' ... ", name_);
  Fl_Anim_GIF *animgif = new Fl_Anim_GIF(0, 0, 0, 0, name_, /*start_=*/false, optimize_mem_, debug_);
  win->end();
  printf("%s\n", animgif->valid() ? "OK" : "ERROR");
  char buf[200];
  if (animgif->valid() && animgif->frames()) {
    if (CopyTest) {
      // test the copy() functionality
      Fl_Anim_GIF *copied = animgif->copy(400, 400);
      delete animgif;
      animgif = copied;
      win->insert(*animgif, 0);
      sprintf(buf, "Copy of '%s' (%d frames)", fl_filename_name(name_), animgif->frames());
    }
    else {
      // test the resize() functionality
      double scale = animgif->h() < 100 ? 2 : 1; // test resize() method on small GIF's
      animgif->resize(scale);
      sprintf(buf, "%s (%d frames) scale=%1.1f", fl_filename_name(name_), animgif->frames(), scale);
    }
    win->copy_tooltip(buf);
    win->copy_label(buf);
    win->size(animgif->w(), animgif->h());
    win->show();
    win->wait_for_expose();
    if (CallbackTest)
      animgif->callback(callback);
    animgif->start();
  }
  else {
    printf("Invalid GIF file '%s', %d frames\n", fl_filename_name(name_), animgif->frames());
    delete win;
    return 0;
  }
  if (debug_ >= 3) {
    for (int i = 0; i < animgif->frames(); i++) {
      char buf[200];
      sprintf(buf, "Frame #%d", i + 1);
      Fl_Double_Window *win = new Fl_Double_Window(animgif->w(), animgif->h());
      win->copy_tooltip(buf);
      win->copy_label(buf);
      win->color(BACKGROUND);
      Fl_Box *b = new Fl_Box(0, 0, win->w(), win->h());
      b->image(animgif->image(i));
      b->size(b->image()->w(), b->image()->h());
      win->size(b->image()->w(), b->image()->h());
      win->end();
      win->show();
    }
  }
  return win;
}

#include <FL/filename.H>
static bool openTestSuite(const char *dir_) {
  dirent **list;
  int nbr_of_files = fl_filename_list(dir_, &list, fl_alphasort);
  if (nbr_of_files <= 0)
    return false;
  int cnt = 0;
  for (int i = 0; i < nbr_of_files; i++) {
    char buf[512];
    const char *name = list[i]->d_name;
    if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
    if (!strstr(name, ".gif") && !strstr(name, ".GIF")) continue;
    snprintf(buf, sizeof(buf), "%s/%s", dir_, name);
    bool debug = strstr(name, "debug");	// hack: when name contains 'debug' open single frames
    if (openFile(buf, true, debug, cnt == 0))
      cnt++;
  }
  return cnt != 0;
}

//
// There are 3 ways how to use this test program:
//  - call with no parameters: brings up file chooser to select and display a GIF
//  - call with a GIF file as argument: display just this GIF file
//  - call with -t and a directory path: display *all* GIF's in this directory
//
int main(int argc_, char *argv_[]) {
  fl_register_images();
  Fl::add_handler(global_key_handler);
  int nofile = true;
  if (argc_ > 1) {
    if (!strcmp(argv_[1], "-t")) {
      nofile = false;
      openTestSuite(argc_ > 2 ? argv_[2] : "testsuite");
    }
    else {
      int debug = 0;
      for (int i = 1; i < argc_; i++) {
        if (!strncmp(argv_[i], "-d", 2)) {
          int j = 1;
          while (argv_[i][j++] == 'd')
            debug++;
        }
        if (!strcmp(argv_[i], "-c"))
          CopyTest = true;
        if (!strcmp(argv_[i], "-x"))
          CallbackTest = true;
      }
      for (int i = 1; i < argc_; i++)
        if (argv_[i][0] != '-') {
          nofile = false;
          openFile(argv_[i], false, debug, debug);
      }
    }
  }
  if (nofile) {
    while (1) {
      const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
      if (!filename)
        break;
      Fl_Window *win = openFile(filename, true, false);
      Fl::run();
      delete win; // delete last window (which is now just hidden) to test destructors
    }
  }
  return Fl::run();
}
