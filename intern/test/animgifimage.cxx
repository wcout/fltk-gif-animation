//
//  Test program for displaying animated GIF files using the
//  Fl_Anim_GIF_Image class.
//
#include <FL/Fl_Anim_GIF_Image.H>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>

static const Fl_Color BackGroundColor = FL_GRAY; // use e.g. FL_RED to see
                                                 // transparent parts better
static const double RedrawDelay = 1./50;         // interval [sec] for forced redraw

static void quit_cb(Fl_Widget* w_, void*) {
  exit(0);
}

static void set_title(Fl_Window *win_, Fl_Anim_GIF_Image *animgif_) {
  char buf[200];
  snprintf(buf, sizeof(buf), "%s (%d frames)  %2.2fx", fl_filename_name(animgif_->name()),
          animgif_->frames(), animgif_->speed());
  if (animgif_->frame_uncache())
    strcat(buf, " U");
  win_->copy_label(buf);
  win_->copy_tooltip(buf);
}

static void cb_forced_redraw(void *d_) {
  Fl_Window *win = Fl::first_window();
  while (win) {
    if (!win->menu_window())
      win->redraw();
    win = Fl::next_window(win);
  }
  if (Fl::first_window())
    Fl::repeat_timeout(RedrawDelay, cb_forced_redraw);
}

Fl_Window *openFile(const char *name_, char *flags_, bool close_ = false) {
  // determine test options from 'flags_'
  bool uncache = strchr(flags_, 'u');
  char *d = flags_ - 1;
  int debug = 0;
  while ((d = strchr(++d, 'd'))) debug++;
  bool optimize_mem = strchr(flags_, 'm');
  bool desaturate = strchr(flags_, 'D');
  bool average = strchr(flags_, 'A');
  bool test_tiles = strchr(flags_, 'T');
  bool test_forced_redraw = strchr(flags_, 'f');
  bool resizable = !test_tiles && strchr(flags_, 'r');

  // setup window
  Fl::remove_timeout(cb_forced_redraw);
  Fl_Double_Window *win = new Fl_Double_Window(100, 100);
  win->color(BackGroundColor);
  if (close_)
    win->callback(quit_cb);
  printf("Loading '%s'%s%s ... ", name_,
    uncache ? " (uncached)" : "",
    optimize_mem ? " (optimized)" : "");

  // create a canvas for the animation
  Fl_Box *canvas = test_tiles ? 0 : new Fl_Box(0, 0, 0, 0); // canvas will be resized by animation
  unsigned short flags = debug ? Fl_Anim_GIF_Image::Log : 0;
  if (debug > 1)
    flags |= Fl_Anim_GIF_Image::Debug;
  if (optimize_mem) {
    flags |= Fl_Anim_GIF_Image::OptimizeMemory;
  }
  // create animation, specifying this canvas as display widget
  Fl_Anim_GIF_Image *animgif = new Fl_Anim_GIF_Image(name_, canvas, flags);
  printf("%s\n", animgif->valid() ? "OK" : "ERROR");
  win->user_data(animgif); // store address of image (see note in main())

  // exercise the optional tests on the animation
  animgif->frame_uncache(uncache);
  if (resizable) // note: bug in FLTK (STR 3352) - test resize functionality here
    animgif->resize(0.7); // hardcoded for now!
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
      animgif->canvas(group, Fl_Anim_GIF_Image::DontResizeCanvas | Fl_Anim_GIF_Image::DontSetAsImage );
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
#if 0
    // make window resizable
    // NOTE: can't do this currently (FLTK bug STR 3352)
    if (resizable && canvas) {
      win->resizable(canvas);
    }
#endif

    // start the animation
    win->end();
    set_title(win, animgif);
    win->show();
    win->wait_for_expose();
    win->size(W, H);
    win->init_sizes(); // IMPORTANT: otherwise weird things happen at Ctrl+/- scaling
    animgif->start();
  } else {
    delete win;
    return 0;
  }
  if (debug >=3) {
    // open each frame in a separate window
    for (int i = 0; i < animgif->frames(); i++) {
      char buf[200];
      snprintf(buf, sizeof(buf), "Frame #%d", i + 1);
      Fl_Double_Window *win = new Fl_Double_Window(animgif->w(), animgif->h());
      win->copy_tooltip(buf);
      win->copy_label(buf);
      win->color(BackGroundColor);
      int w = animgif->image(i)->w();
      int h = animgif->image(i)->h();
      // in 'optimize_mem' mode frames must be offsetted to canvas
      int x = (w == animgif->w() && h == animgif->h()) ? 0 : animgif->frame_x(i);
      int y = (w == animgif->w() && h == animgif->h()) ? 0 : animgif->frame_y(i);
      Fl_Box *b = new Fl_Box(x, y, w, h);
      // get the frame image
      b->image(animgif->image(i));
      win->end();
      win->show();
    }
  }
  return win;
}

#include <FL/filename.H>
bool openDirectory(const char *dir_, char *flags_) {
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
    if (strstr(name, "debug"))	// hack: when name contains 'debug' open single frames
      strcat(flags_, "d");
    if (openFile(buf, flags_, cnt == 0))
      cnt++;
  }
  return cnt != 0;
}

static void change_speed(bool up_) {
  Fl_Widget *below = Fl::belowmouse();
  if (below && below->image()) {
    Fl_Anim_GIF_Image *animgif = 0;
    // Q: is there a way to determine Fl_Tiled_Image without using dynamic cast?
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
    else if (Fl::event_key() == '-')
      change_speed(false);
    else
      return 0;
  }
  return 1;
}

static const char testsuite[] = "testsuite";

int main(int argc_, char *argv_[]) {
  fl_register_images();
  Fl::add_handler(events);
  char *openFlags = (char *)calloc(1024, 1);
  if (argc_ > 1) {
    if (strstr(argv_[1], "-h")) {
      printf("Usage:\n"
             "   -t [directory] [-{flags}] open all files in directory (default name: %s) [with options]\n"
             "   filename [-{flags}] open single file [with options] \n"
             "   No arguments open a fileselector\n"
             "   {flags} can be: d=debug mode, u=uncached, D=desaturated, A=color averaged, T=tiled\n"
             "                   m=minimal update, r=resized\n"
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
    Fl_GIF_Image::animate = true; // create animated shared .GIF images (e.g. file chooser)
    while (1) {
      Fl::add_timeout(0.1, cb_forced_redraw); // animate images in chooser
      const char *filename = fl_file_chooser("Select a GIF image file","*.{gif,GIF}", NULL);
      Fl::remove_timeout(cb_forced_redraw);
      if (!filename)
        break;
      Fl_Window *win = openFile(filename, openFlags);
      Fl::run();
      // delete last window (which is now just hidden) to test destructors
      // NOTE: it is essential that *before* doing this also the
      //       animated image is destroyed, otherwise it will crash
      //       because it's canvas will be gone.
      //       In order to keep this demo simple, the adress of the
      //       Fl_Anim_GIF_Image has been stored in the window's user_data.
      //       In a real-life application you will probably store
      //       it somewhere in the window's or canvas' object and destroy
      //       the image in the window's or canvas' destructor.
      if (win && win->user_data())
        delete ((Fl_Anim_GIF_Image *)win->user_data());
      delete win;
    }
  }
  return Fl::run();
}
