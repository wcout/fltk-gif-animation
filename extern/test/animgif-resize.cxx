//
//  Test program for Fl_Anim_GIF::copy().
//
#include "../Fl_Anim_GIF.cxx"
#include <FL/Fl_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdio>

static Fl_Anim_GIF *orig = 0;
static bool draw_grid = true;

static void dump_shared_images()
{
  int numImages = Fl_Shared_Image::num_images();
  Fl_Shared_Image **images = Fl_Shared_Image::images();
  printf("Shared images: %d\n", numImages);
  for (int i = 0; i < numImages; i++)
  {
    printf("%02u [%d] '%s' %d x %d\n", i+1, images[i]->refcount(),
           images[i]->name(), images[i]->w(), images[i]->h());
  }
}

class Canvas : public Fl_Group {
  typedef Fl_Group Inherited;
public:
  Canvas(int x_, int y_, int w_, int h_) :
    Inherited(x_, y_, w_, h_)
  {
    if (!draw_grid)
      box(FL_FLAT_BOX);
  }
  virtual void draw() {
    if (draw_grid) {
      // draw a transparency grid as background
      static const Fl_Color C1 = fl_rgb_color(0xcc, 0xcc, 0xcc);
      static const Fl_Color C2 = fl_rgb_color(0x88, 0x88, 0x88);
      static const int SZ = 8;
      for (int y = 0; y < h(); y += SZ) {
        for (int x = 0; x < w(); x += SZ) {
          fl_color(x%(SZ * 2) ? y%(SZ * 2) ? C1 : C2 : y%(SZ * 2) ? C2 : C1);
          fl_rectf(x, y, 32, 32);
        }
      }
    }
    // draw the current image frame over the grid
    Inherited::draw();
  }
  void do_resize(int W_, int H_) {
    if (!children()) return;
    Fl_Anim_GIF *animgif = (Fl_Anim_GIF *)child(0);
    if (animgif->canvas_w() != W_ || animgif->canvas_w() != H_) {
#if FL_ABI_VERSION >= 10304 && USE_SHIMAGE_SCALING
      static bool once = false;
      if (!once) {
        printf("Using fast shared image scaling\n");
        if (Fl_RGB_Image::RGB_scaling() == FL_RGB_SCALING_BILINEAR) {
           printf("**NOTE**: This bypasses bilinear scaling option!\n");
        }
      }
      once = true;
      // using Fl_Shared_Image::scale(), so no need to reload from original
      animgif->resize(W_, H_);
      animgif->start();
      printf("resized to %d x %d\n", animgif->w(), animgif->h());
#else
      // reload from original
      animgif->stop();
      remove(0);
      // delete already copied images
      if (animgif != orig) {
        delete animgif;
      }
      Fl_Anim_GIF *copied = orig->copy(W_, H_);
      if (!copied->valid()) { // check success of copy
        Fl::warning("Fl_Anim_GIF::copy() %d x %d failed", W_, H_);
      }
      else {
        printf("copy/resized to %d x %d\n", copied->w(), copied->h());
      }
      insert(*copied, 0);
      copied->start();
#endif
    }
    window()->cursor(FL_CURSOR_DEFAULT);
  }
  static void do_resize_cb(void *d_) {
    Canvas *c = (Canvas *)d_;
    c->do_resize(c->w(), c->h());
  }
  virtual void resize(int x_, int y_, int w_, int h_) {
    Inherited::resize(x_, y_, w_, h_);
    // decouple resize event from actual resize operation
    // to avoid lockups..
    Fl::remove_timeout(do_resize_cb, this);
    Fl::add_timeout(0.1, do_resize_cb, this);
    window()->cursor(FL_CURSOR_WAIT);
  }
};

static int global_shortcut(int event_)
{
  // we only handle shortcuts here
  if (event_ != FL_SHORTCUT) return 0;

  if (Fl::test_shortcut(FL_ALT + 'd')) {
    dump_shared_images(); // list stored shared images (for debugging purposes only)
    return 1;
  }
  return 0;
}

int main(int argc_, char *argv_[]) {
  // setup play parameters from args
  const char *fileName = 0;
  bool bilinear = false;
  bool optimize = false;
  bool uncache = false;
  for (int i = 1; i < argc_; i++) {
    if (!strcmp(argv_[i], "-b")) // turn bilinear scaling on
      bilinear = true;
    else if (!strcmp(argv_[i], "-m")) // turn optimize on
      optimize = true;
    else if (!strcmp(argv_[i], "-g")) // disable grid
      draw_grid = false;
    else if (!strcmp(argv_[i], "-u")) // uncache
      uncache = true;
    else if (argv_[i][0] != '-' && !fileName) {
      fileName = argv_[i];
    }
  }
  if (!fileName) {
    fprintf(stderr, "Test program for animated copy.\n");
    fprintf(stderr, "Usage: %s fileName [-b] [-m] [-g] [-u]\n", argv_[0]);
    exit(0);
  }
  Fl_Anim_GIF::min_delay = 0.1; // set a minumum delay for playback

  Fl_Double_Window win(640, 480);

  // prepare a container for the animation
  Canvas canvas(0, 0, win.w(), win.h());
  win.resizable(win);
  win.size_range(1, 1);

  win.end();
  win.show();

  // create/load the animated gif and start it immediately.
  if (optimize) {
    printf("Using memory optimization (if image supports)\n");
  }
  orig = new Fl_Anim_GIF(canvas.x(), canvas.y(), 0, 0,
                         /*name_=*/ fileName, /*start_=*/true, optimize);
  canvas.insert(*orig, 0);

  // check if loading succeeded
  printf("%s: valid: %d frames: %d uncache: %d\n",
    orig->label(), orig->valid(), orig->frames(), orig->uncache());
  if (orig->valid()) {
    win.copy_label(fileName);

    // print information about image optimization
    int n = 0;
    for (int i = 0; i < orig->frames(); i++) {
      if (orig->frame_x(i) != 0 || orig->frame_y(i) != 0) n++;
    }
    printf("image has %d optimized frames\n", n);

    Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_NEAREST);
    Fl_Shared_Image::scaling_algorithm(FL_RGB_SCALING_NEAREST);
    if (bilinear) {
      Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);
      Fl_Shared_Image::scaling_algorithm(FL_RGB_SCALING_BILINEAR);
      printf("Using bilinear scaling - can be slow!\n");
      // NOTE: this is *really* slow. Scaling the TrueColor test image
      //       to full HD desktop takes about 45 seconds!
      // 2017/10/21: this has been improved by lazy resize (per frame)
      //             and by using shared image scale feature.
    }
    orig->uncache(uncache);
    if (uncache) {
      printf("Caching disabled - watch cpu load!\n");
    }

    // set initial size to fit into window
    double ratio = orig->valid() ? (double)orig->w() / orig->h() : 1;
    int W = win.w() - 40;
    int H = (double)W / ratio;
    printf("original size: %d x %d\n", orig->w(), orig->h());
    orig->size(W, H); // resize animation to fit in window
    win.size(W, H);

    Fl::add_handler(global_shortcut);

    return Fl::run();
  }
  else {
    printf("Invalid GIF file: '%s'\n", fileName);
  }
}
