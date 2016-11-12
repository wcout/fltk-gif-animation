//
//  Test program for displaying animated GIF files using the
//  Fl_Anim_GIF_Image class.
//
#include <FL/Fl_Anim_GIF_Image.H>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>

//#define BACKGROUND FL_RED	// use this to see transparent parts better
#define BACKGROUND FL_GRAY
//#define BACKGROUND FL_BLACK

static bool GtestForcedRedraw = false;

static void quit_cb(Fl_Widget* w_, void*) {
  exit(0);
}

static void set_title(Fl_Window *win_, Fl_Anim_GIF_Image *animgif_) {
  char buf[200];
  sprintf(buf, "%s (%d frames)  %2.2fx", fl_filename_name(animgif_->name()),
          animgif_->frames(), animgif_->speed());
  if (animgif_->uncache())
    strcat(buf, " U");
  win_->copy_label(buf);
  win_->copy_tooltip(strdup(buf));
}

static void cb_forced_redraw(void *d_) {
  Fl_Window *win = Fl::first_window();
  while (win) {
    win->redraw();
    win = Fl::next_window(win);
  }
  if (Fl::first_window())
    Fl::add_timeout(0.02, cb_forced_redraw);
  else
    Fl::remove_timeout(cb_forced_redraw);
}

bool openFile(const char *name_, bool debug_, bool close_ = false,
              bool uncache_ = false) {
  Fl::remove_timeout(cb_forced_redraw);
  Fl_Double_Window *win = new Fl_Double_Window(100, 100);
  win->color(BACKGROUND);
  if (close_)
    win->callback(quit_cb);
  printf("\nLoading '%s'%s\n", name_, uncache_ ? " (uncached)" : "");
  Fl_Box *canvas = new Fl_Box(0, 0, 0, 0); // canvas for animation
  Fl_Anim_GIF_Image *animgif = new Fl_Anim_GIF_Image(name_, canvas, false, debug_);
  animgif->uncache(uncache_);
  int W = animgif->w();
  if (GtestForcedRedraw) {
    // demonstrate a way how to use same animation in another canvas simultaneously:
    // as the current implementation allows only automatic redraw of one canvas..
    if (W < 400) {
      Fl::add_timeout(0.02, cb_forced_redraw); // must force periodic redraw
      canvas = new Fl_Box(W, 0, animgif->w(), animgif->h()); // the other canvas for animation
      canvas->image(animgif); // set to same animation!
      W *= 2;
    }
  }
  win->end();
  if (animgif->frames()) {
    win->size(W, animgif->h());
    set_title(win, animgif);
    win->show();
    win->wait_for_expose();
    animgif->start();
  } else {
    delete win;
    return false;
  }
  if (debug_) {
    for (int i = 0; i < animgif->frames(); i++) {
      char buf[200];
      sprintf(buf, "Frame #%d\n", i + 1);
      Fl_Double_Window *win = new Fl_Double_Window(animgif->w(), animgif->h());
      win->tooltip(strdup(buf));
      win->copy_label(buf);
      win->color(BACKGROUND);
      Fl_Box *b = new Fl_Box(0, 0, win->w(), win->h());
      b->image(animgif->image(i));
      win->end();
      win->show();
    }
  }
  return true;
}

#include <sys/types.h>
#include <dirent.h>
bool openDirectory(const char *dir_, bool uncache_) {
  DIR *dir = opendir(dir_);
  if (!dir)
    return false;
  int cnt = 0;
  struct dirent *entry;
  while ((entry = readdir(dir))) {
    char buf[512];
    const char *name = entry->d_name;
    if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
    if (!strstr(name, ".gif") && !strstr(name, ".GIF")) continue;
    sprintf(buf, "%s/%s", dir_, name);
    bool debug = strstr(name, "debug");	// when name contains 'debug' open single frames
    if (openFile(buf, debug, cnt == 0, uncache_))
      cnt++;
  }
  closedir(dir);
  return cnt != 0;
}

static void change_speed(bool up_) {
  Fl_Widget *below = Fl::belowmouse();
  if (below && below->image()) {
    Fl_Anim_GIF_Image *animgif = static_cast<Fl_Anim_GIF_Image *>(below->image());
    if (animgif) {
      double speed = animgif->speed();
      if (up_) speed += 0.1;
      else speed -= 0.1;
      if (speed < 0.1) speed = 0.1;
      if (speed > 10) speed = 10;
      animgif->speed(speed);
      set_title(below->window(), animgif);
    }
  }
}

static int events(int event_) {
  Fl::first_window()->redraw();
  if (event_ == FL_SHORTCUT) {
    if (Fl::event_key() == '+')
      change_speed(true);
    if (Fl::event_key() == '-')
      change_speed(false);
  }
  return 0;
}

static const char testsuite[] = "testsuite";

int main(int argc_, char *argv_[]) {
  fl_register_images();
  Fl::add_handler(events);
  if (argc_ > 1) {
    if (strstr(argv_[1], "-h")) {
      printf("Usage:\n"
             "   -t [directory] [-u]   open all files in directory (default name: %s) [uncached]\n"
             "   filename [-d]         open single file [in debug mode]\n"
             "   No arguments open fileselector\n"
             "   Use keys '+'/'-' to change speed of the active image.\n", testsuite);
      exit(1);
    }
    if (!strcmp(argv_[1], "-t")) {
      bool uncache = false;
      const char *dir = testsuite;
      for (int i = 2; i < argc_; i++)
        if (!strcmp(argv_[i], "-u"))
          uncache = true;
        else if (argv_[i][0] != '-')
          dir = argv_[i];
      openDirectory(dir, uncache);
    } else {
      bool debug = false;
      for (int i = 1; i < argc_; i++) {
        if (!strcmp(argv_[i], "-d"))
          debug = true;
        if (!strcmp(argv_[i], "-f"))
          GtestForcedRedraw = true;
      }
      for (int i = 1; i < argc_; i++)
        if (argv_[i][0] != '-')
          openFile(argv_[i], debug, debug);
    }
  } else {
    while (1) {
      const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
      if (!filename)
        break;
      openFile(filename, false);
      Fl::run();
    }
  }
  return Fl::run();
}
