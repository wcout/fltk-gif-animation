//
//  Test program for displaying animated GIF files using the
//  Fl_Anim_GIF_Image class.
//
#include <FL/Fl_Anim_GIF_Image.H>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>

//#define BACKGROUND FL_RED	// use this to see transparent parts better
#define BACKGROUND FL_GRAY
//#define BACKGROUND FL_BLACK

static const double RedrawDelay = 1./50;

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
  win_->copy_tooltip(buf);
}

static void cb_forced_redraw(void *d_) {
  Fl_Window *win = Fl::first_window();
  while (win) {
    win->redraw();
    win = Fl::next_window(win);
  }
  if (Fl::first_window())
    Fl::repeat_timeout(RedrawDelay, cb_forced_redraw);
}

bool openFile(const char *name_, char *flags_, bool close_ = false) {
  bool uncache = strchr(flags_, 'u');
  bool debug = strchr(flags_, 'd');
  bool desaturate = strchr(flags_, 'D');
  bool average = strchr(flags_, 'A');
  bool test_tiles = strchr(flags_, 'T');
  bool test_forced_redraw = strchr(flags_, 'f');
  Fl::remove_timeout(cb_forced_redraw);
  Fl_Double_Window *win = new Fl_Double_Window(100, 100);
  win->color(BACKGROUND);
  if (close_)
    win->callback(quit_cb);
  printf("\nLoading '%s'%s\n", name_, uncache ? " (uncached)" : "");
  Fl_Box *canvas = test_tiles ? 0 : new Fl_Box(0, 0, 0, 0); // canvas for animation
  Fl_Anim_GIF_Image *animgif = new Fl_Anim_GIF_Image(name_, canvas, false, debug);
  animgif->uncache(uncache);
  if (average)
    animgif->color_average(FL_GREEN, 0.5); // currently hardcoded
  if (desaturate)
    animgif->desaturate();
  int W = animgif->w();
  int H = animgif->h();
  if (animgif->frames()) {
    if (test_tiles) {
      // demonstrate a way how to use the animation with Fl_Tiled_Image
      W *= 2;
      H *= 2;
      Fl_Tiled_Image *tiled_image = new Fl_Tiled_Image(animgif);
      Fl_Group *group = new Fl_Group(0, 0, win->w(), win->h());
      group->image(tiled_image);
      group->align(FL_ALIGN_INSIDE);
      animgif->canvas(group, false);
      win->resizable(group);
    } else {
      // demonstrate a way how to use same animation in another canvas simultaneously:
      // as the current implementation allows only automatic redraw of one canvas..
      if (test_forced_redraw) {
        if (W < 400) {
          canvas = new Fl_Box(W, 0, animgif->w(), animgif->h()); // another canvas for animation
          canvas->image(animgif); // is set to same animation!
          W *= 2;
          Fl::add_timeout(RedrawDelay, cb_forced_redraw); // force periodic redraw
        }
      }
    }
    win->end();
    set_title(win, animgif);
    win->show();
    win->wait_for_expose();
    win->size(W, H);
    animgif->start();
  } else {
    delete win;
    return false;
  }
  if (debug) {
    for (int i = 0; i < animgif->frames(); i++) {
      char buf[200];
      sprintf(buf, "Frame #%d", i + 1);
      Fl_Double_Window *win = new Fl_Double_Window(animgif->w(), animgif->h());
      win->copy_tooltip(buf);
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
bool openDirectory(const char *dir_, char *flags_) {
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
    if (strstr(name, "debug"))	// when name contains 'debug' open single frames
      strcat(flags_, "d");
    if (openFile(buf, flags_, cnt == 0))
      cnt++;
  }
  closedir(dir);
  return cnt != 0;
}

static void change_speed(bool up_) {
  Fl_Widget *below = Fl::belowmouse();
  if (below && below->image()) {
    Fl_Anim_GIF_Image *animgif = 0;
    // is there another way to determine Fl_Tiled_Image?
    Fl_Tiled_Image *tiled = dynamic_cast<Fl_Tiled_Image *>(below->image());
    animgif = tiled ?
              dynamic_cast<Fl_Anim_GIF_Image *>(tiled->image()) :
              dynamic_cast<Fl_Anim_GIF_Image *>(below->image());
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
  char *openFlags = (char *)calloc(1024, 1);
  if (argc_ > 1) {
    if (strstr(argv_[1], "-h")) {
      printf("Usage:\n"
             "   -t [directory] [-u]   open all files in directory (default name: %s) [uncached]\n"
             "   filename [-d][-T][-D] open single file [in debug mode] [as tiled image] [desaturated]\n"
             "   No arguments open fileselector\n"
             "   Use keys '+'/'-' to change speed of the active image.\n", testsuite);
      exit(1);
    }
    for (int i = 1; i < argc_; i++) {
      if (argv_[i][0] == '-')
        strcat(openFlags, &argv_[i][1]);
    }
    if (strchr(openFlags, 't')) {
      const char *dir = testsuite;
      for (int i = 2; i < argc_; i++)
        if (argv_[i][0] != '-')
          dir = argv_[i];
      openDirectory(dir, openFlags);
    } else {
      for (int i = 1; i < argc_; i++)
        if (argv_[i][0] != '-')
          openFile(argv_[i], openFlags, strchr(openFlags, 'd'));
    }
  } else {
    while (1) {
      const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
      if (!filename)
        break;
      openFile(filename, openFlags);
      Fl::run();
    }
  }
  return Fl::run();
}
