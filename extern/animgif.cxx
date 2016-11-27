#include "Fl_Anim_GIF.cxx"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>

//#define BACKGROUND FL_RED	// use this to see transparent parts better
#define BACKGROUND FL_GRAY
//#define BACKGROUND FL_BLACK

static void quit_cb(Fl_Widget* w_, void*) {
  exit(0);
}

Fl_Window *openFile(const char *name_, bool debug_, bool close_ = false) {
  Fl_Double_Window *win = new Fl_Double_Window(100, 100);
  win->color(BACKGROUND);
  if (close_)
    win->callback(quit_cb);
  printf("\nLoading '%s'\n", name_);
  Fl_Anim_GIF *animgif = new Fl_Anim_GIF(0, 0, 0, 0, name_, false, debug_);
  win->end();
  if (animgif->frames()) {
    win->size(animgif->w(), animgif->h());
    char buf[200];
    sprintf(buf, "%s (%d frames)", fl_filename_name(name_), animgif->frames());
    win->tooltip(strdup(buf));
    win->copy_label(buf);
    win->show();
    win->wait_for_expose();
    animgif->start();
  } else {
    delete win;
    return 0;
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
  return win;
}

#include <FL/filename.H>
bool openTestSuite(const char *dir_) {
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
    if (openFile(buf, debug, cnt == 0))
      cnt++;
  }
  return cnt != 0;
}

int main(int argc_, char *argv_[]) {
  fl_register_images();
  if (argc_ > 1) {
    if (!strcmp(argv_[1], "-t"))
      openTestSuite(argc_ > 2 ? argv_[2] : "testsuite");
    else {
      bool debug = false;
      for (int i = 1; i < argc_; i++)
        if (!strcmp(argv_[i], "-d"))
          debug = true;
      for (int i = 1; i < argc_; i++)
        if (argv_[i][0] != '-')
          openFile(argv_[i], debug, debug);
    }
  } else {
    while (1) {
      const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
      if (!filename)
        break;
      Fl_Window *win = openFile(filename, false);
      Fl::run();
      delete win; // delete last window (which is now just hidden) to test destructors
    }
  }
  return Fl::run();
}
