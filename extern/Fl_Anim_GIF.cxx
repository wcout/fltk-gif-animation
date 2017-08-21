#ifdef FL_LIBRARY
#include <FL/Fl_Anim_GIF.H>
#else
#include "Fl_Anim_GIF.H"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl.H>
#include <new> // std::nothrow

namespace {
  enum Transparency {
    T_NONE = 0xff,
    T_FULL = 0
  };

struct RGBA_Color {
  uchar r, g, b, alpha;
  RGBA_Color(uchar r_ = 0, uchar g_ = 0, uchar b_ = 0, uchar a_ = T_NONE) :
    r(r_), g(g_), b(b_), alpha(a_) {}
};
}

#include "fl_anim_gif_private.H"

struct GifFrame {
  GifFrame() :
    rgb(0),
    x(0),
    y(0),
    w(0),
    h(0),
    delay(0),
    dispose(0),
    transparent(false),
    transparent_color_index(-1) {}
  Fl_RGB_Image *rgb;		// full frame image
  int x, y, w, h;			// frame original dimensions
  double delay;				// delay (already converted to ms)
  int dispose;				// disposal method
  bool transparent;       // background color is transparent color
  int transparent_color_index;
  RGBA_Color transparent_color;
};

struct FrameInfo {
  FrameInfo() :
    frames_size(0),
    frames(0),
    loop_count(1),
    background_color_index(-1),
    canvas_w(0),
    canvas_h(0),
    desaturate(false),
    average_color(FL_BLACK),
    average_weight(-1),
    debug(false),
    optimize_mem(false),
    offscreen(0) {}
  int frames_size;                         // number of frames stored in 'frames'
  GifFrame *frames;                        // "vector" for frames
  int loop_count;                          // loop count from file
  int background_color_index;              // needed for dispose()
  RGBA_Color background_color;             // needed for dispose()
  GifFrame frame;                          // current processed frame
  int canvas_w;                            // width of GIF from header
  int canvas_h;                            // height of GIF from header
  bool desaturate;                         // flag if frames should be desaturated
  Fl_Color average_color;                  // color for color_average()
  float average_weight;                    // weight for color_average (negative: none)
  bool debug;                              // Flag for debug outputs
  bool optimize_mem;                       // Flag to store frames in original dimensions
  uchar *offscreen;                        // internal "offscreen" buffer to build frames
};


#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>	// for parent()
#include <FL/Fl.H>			// for Fl::add_timeout()

#define DEBUG(x) if ( _fi->debug ) printf x
//#define DEBUG(x)

static double convertDelay(int d_) {
  if (d_ < 1)
    d_ = 10;
  return (double)d_ / 100;
}

// reset offscreen to background color
static void setToBackGround(uchar *offscreen_, int frame_, FrameInfo *_fi) {
  int bg = _fi->background_color_index;
  int tp = frame_ >= 0 ?_fi->frames[frame_].transparent_color_index : bg;
  DEBUG(("setToBackGround [%d] tp = %d, bg = %d\n", frame_, tp, bg));
  RGBA_Color color = _fi->background_color;
  if (tp >= 0)
    color = _fi->frames[frame_].transparent_color;
  if (tp >= 0 && bg >= 0)
    bg = tp;
  color.alpha = tp == bg ? T_FULL : tp < 0 ? T_FULL : T_NONE;
  DEBUG(("  setToColor %d/%d/%d alpha=%d\n", color.r, color.g, color.b, color.alpha));
  for (uchar *p = offscreen_ + _fi->canvas_w * _fi->canvas_h * 4 - 4; p >= offscreen_; p -= 4)
    memcpy(p, &color, 4);
}

// dispose frame with index 'frame_' to offscreen buffer
static void dispose(int frame_, FrameInfo *_fi, uchar *offscreen_) {
  if (frame_ < 0) {
    return;
  }
  switch (_fi->frames[frame_].dispose) {
    case DISPOSE_PREVIOUS: {
        // dispose to previous restores to first not DISPOSE_TO_PREVIOUS frame
        int prev(frame_);
        while (prev > 0 && _fi->frames[prev].dispose == DISPOSE_PREVIOUS)
          prev--;
        if (prev == 0 && _fi->frames[prev].dispose == DISPOSE_PREVIOUS) {
          setToBackGround(offscreen_, -1, _fi);
          return;
        }
        DEBUG(("     dispose frame %d to previous frame %d\n", frame_ + 1, prev + 1));
        // copy the previous image data..
        uchar *dst = offscreen_;
        const char *src = _fi->frames[prev].rgb->data()[0];
        memcpy((char *)dst, (char *)src, _fi->canvas_w * _fi->canvas_h * 4);
        break;
      }
    case DISPOSE_BACKGROUND:
      DEBUG(("     dispose frame %d to background\n", frame_ + 1));
      setToBackGround(offscreen_, frame_, _fi);
      break;

    default: {
        // nothing to do (keep everything as is)
        break;
      }
  }
}

Fl_Anim_GIF::Fl_Anim_GIF(int x_, int y_, int w_, int h_,
                         const char *name_ /* = 0*/, bool start_ /* = false*/,
                         bool debug_/* = false*/) :
  Inherited(x_, y_, 0, 0),
  _valid(false),
  _frame(-1),
  _fi(new(std::nothrow)FrameInfo()),
  _uncache(false) {
  if (!_fi)
    return;
  _fi->debug = debug_;
  load(name_);
  if (canvas_w() && canvas_h()) {
    if (!w() && !h())
      size(canvas_w(), canvas_h());
  }
  if (start_)
    start();
}

Fl_Anim_GIF::~Fl_Anim_GIF() {
  Fl::remove_timeout(cb_animate, this);
  clear_frames();
  delete _fi;
}

bool Fl_Anim_GIF::start() {
  if (_fi->frames_size) {
    nextFrame();
  }
  return _fi->frames_size != 0;
}

void Fl_Anim_GIF::clear_frames() {
  while (_fi->frames_size--) {
    delete _fi->frames[_fi->frames_size].rgb;
  }
  free(_fi->frames);
  _fi->frames = 0;
  _fi->frames_size = 0;
}

// add a frame to the "vector" in FrameInfo
static bool push_back_frame(FrameInfo *fi_, GifFrame *frame_) {
  void *tmp = realloc(fi_->frames, sizeof(GifFrame) * (fi_->frames_size + 1));
  if (!tmp) {
    return false;
  }
  fi_->frames = (GifFrame *)tmp;
  memcpy(&fi_->frames[ fi_->frames_size ], frame_, sizeof(GifFrame));
  fi_->frames_size++;
  return true;
}

bool Fl_Anim_GIF::nextFrame() {
  int last_frame = _frame;
  _frame++;
  if (_frame >= _fi->frames_size)
    _frame = 0;
  if (_frame >= _fi->frames_size)
    return false;

  // NOTE: decreases performance, but saves a lot of memory
  if (_uncache && Inherited::image())
    Inherited::image()->uncache();

  Inherited::image(image());
  if ((last_frame >= 0 && (_fi->frames[last_frame].dispose == DISPOSE_BACKGROUND ||
     _fi->frames[last_frame].dispose == DISPOSE_PREVIOUS)) ||
     (_frame == 0 ))
    parent()->redraw();
  else
    redraw();
  double delay = _fi->frames[_frame].delay;
  if (delay)	// normal GIF has no delay
    Fl::repeat_timeout(delay, cb_animate, this);
  return true;
}

bool Fl_Anim_GIF::load(const char *name_) {
  DEBUG(("Fl_Anim_GIF:::load '%s'\n", name_));
  clear_frames();

  // open gif file for readin
  GifFileType *gifFileIn;
  int errorCode;
  if ((gifFileIn = DGifOpenFileName(name_, &errorCode)) == NULL) {
    fprintf(stderr, "open '%s': %s\n", name_, GifErrorString(errorCode));
    return false;
  }
  DEBUG(("%d x %d  BG=%d aspect %d\n", gifFileIn->SWidth, gifFileIn->SHeight, gifFileIn->SBackGroundColor, gifFileIn->AspectByte));
  _fi->canvas_w = gifFileIn->SWidth;
  _fi->canvas_h = gifFileIn->SHeight;
  w( _fi->canvas_w);
  h( _fi->canvas_h);
  _frame = -1;
  _fi->offscreen = (uchar *)calloc(_fi->canvas_w * _fi->canvas_h * 4, 1);
  _fi->background_color_index = gifFileIn->SColorMap ? gifFileIn->SBackGroundColor : -1;
  if (_fi->background_color_index >= 0) {
    _fi->background_color = RGBA_Color(
                              gifFileIn->SColorMap->Colors[_fi->background_color_index].Red,
                              gifFileIn->SColorMap->Colors[_fi->background_color_index].Green,
                              gifFileIn->SColorMap->Colors[_fi->background_color_index].Blue);
  }

  // read whole file
  if (DGifSlurp(gifFileIn) == GIF_ERROR) {
    fprintf(stderr, "slurp '%s': %s\n", name_, GifErrorString(errorCode));
    DGifCloseFile(gifFileIn, &errorCode);
    return false;
  }
  DEBUG(("images: %d\n", gifFileIn->ImageCount));

  // process all frames
  GraphicsControlBlock gcb = {};
  gcb.TransparentColor = NO_TRANSPARENT_COLOR;
  for (int i = 0; i < gifFileIn->ImageCount; i++) {
    GifFrame &frame = _fi->frame;
    SavedImage *image = &gifFileIn->SavedImages[i];
    GifImageDesc *id = &image->ImageDesc;

    ColorMapObject *ColorMap = id->ColorMap ? id->ColorMap : gifFileIn->SColorMap;
    frame.x = id->Left;
    frame.y = id->Top;
    frame.w = id->Width;
    frame.h = id->Height;
    frame.delay = 0;
    frame.transparent_color_index = -1;
    int e = image->ExtensionBlockCount;
    while (e--) {
      ExtensionBlock *ext = &image->ExtensionBlocks[ e ];
      if (ext->Function == GRAPHICS_EXT_FUNC_CODE) {
        DGifExtensionToGCB(ext->ByteCount, ext->Bytes, &gcb);
        DEBUG(("#%d %d/%d %dx%d delay: %d, dispose: %d transparent_color: %d\n",
               (int)_fi->frames_size + 1,
               frame.x, frame.y, frame.w, frame.h,
               gcb.DelayTime, gcb.DisposalMode, gcb.TransparentColor));
        frame.delay = convertDelay(gcb.DelayTime);
        frame.dispose = gcb.DisposalMode;
        break;
      }
    }
    if (!ColorMap) {
      fprintf(stderr, "Gif Image does not have a colormap\n");
      DGifCloseFile(gifFileIn, &errorCode);
      return false;
    }

    // we know now everything we need about the frame..
    frame.transparent_color_index = gcb.TransparentColor;
    if (frame.transparent_color_index >= 0)
      frame.transparent_color = RGBA_Color(
                                  ColorMap->Colors[frame.transparent_color_index].Red,
                                  ColorMap->Colors[frame.transparent_color_index].Green,
                                  ColorMap->Colors[frame.transparent_color_index].Blue);

    dispose(_frame, _fi, _fi->offscreen);

    // copy image data to rgb
    uchar *bits = image->RasterBits;
    for (int y = frame.y; y < frame.y + frame.h; y++) {
      for (int x = frame.x; x < frame.x + frame.w; x++) {
        uchar c = *bits++;
        if (c == gcb.TransparentColor)
          continue;
        uchar *buf = _fi->offscreen;
        buf += (y * w() * 4 + (x * 4));
        *buf++ = ColorMap->Colors[c].Red;
        *buf++ = ColorMap->Colors[c].Green;
        *buf++ = ColorMap->Colors[c].Blue;
        *buf = T_NONE;
      }
    }

    // create RGB image from offscreen
    if (_fi->optimize_mem) {
      uchar *buf = new uchar[frame.w * frame.h * 4];
      uchar *dest = buf;
      for (int y = frame.y; y < frame.y + frame.h; y++) {
        for (int x = frame.x; x < frame.x + frame.w; x++) {
          memcpy(dest, &_fi->offscreen[y * w() * 4 + x * 4], 4);
          dest += 4;
        }
      }
      frame.rgb = new Fl_RGB_Image(buf, frame.w, frame.h, 4);
    }
    else {
      uchar *buf = new uchar[w() * h() * 4];
      memcpy(buf, _fi->offscreen, w() * h() * 4);
      frame.rgb = new Fl_RGB_Image(buf, w(), h(), 4);
    }
    frame.rgb->alloc_array = 1;

    if (!push_back_frame(_fi, &frame)) {
      DGifCloseFile(gifFileIn, &errorCode);
      return false;
    }

    // free compressed data
    free(image->RasterBits);
    image->RasterBits = 0;
    _frame++;
  }
  DGifCloseFile(gifFileIn, &errorCode);
  _valid = true;
  memset(_fi->offscreen, 0, w() * h() * 4);
  return _valid;
}         // load

int Fl_Anim_GIF::canvas_w() const {
  return _fi->canvas_w;
}

int Fl_Anim_GIF::canvas_h() const {
  return _fi->canvas_h;
}

int Fl_Anim_GIF::frames() const {
  return _fi->frames_size;
}

int Fl_Anim_GIF::frame() const {
  return _frame;
}

Fl_Image *Fl_Anim_GIF::image() const {
  return _fi->frames[_frame].rgb;
}

Fl_Image *Fl_Anim_GIF::image(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].rgb;
  return 0;
}

void Fl_Anim_GIF::uncache(bool uncache_) {
  _uncache = uncache_;
}

bool Fl_Anim_GIF::uncache() const {
  return _uncache;
}

bool Fl_Anim_GIF::valid() const {
  return _valid;
}
/*static*/
void Fl_Anim_GIF::cb_animate(void *d_) {
  Fl_Anim_GIF *b = (Fl_Anim_GIF *)d_;
  b->nextFrame();
}
