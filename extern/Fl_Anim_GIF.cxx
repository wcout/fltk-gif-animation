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

class RGB_Image : public Fl_RGB_Image {
  typedef Fl_RGB_Image Inherited;
public:
  enum Transparency {
    T_NONE = 0xff,
    T_FULL = 0
  };
  struct RGBA_Color {
    uchar r, g, b, alpha;
    RGBA_Color(uchar r_ = 0, uchar g_ = 0, uchar b_ = 0, uchar a_ = T_NONE) :
      r(r_), g(g_), b(b_), alpha(a_) {}
  };
  RGB_Image(Fl_Pixmap *pixmap_) :
    Inherited(pixmap_) {}
  RGB_Image(uchar *bits_, int w_, int h_, int d_) :
    Inherited(bits_, w_, h_, d_) {}
  bool isTransparent(int x_, int y_) const;
  RGBA_Color getPixel(int x_, int y_) const;
  void setPixel(int x_, int y_, const RGBA_Color& color_, bool alpha_ = false) const;
  void setToColor(const RGBA_Color c_, bool alpha_ = false) const;
  void setToColor(int x_, int y_, int w_, int h_, const RGBA_Color &c_, bool alpha_ = false) const;
};

bool RGB_Image::isTransparent(int x_, int y_) const {
  const char *buf = data()[0];
  long index = (y_ * w() * d() + (x_ * d()));
  uchar alpha = d() == 4 ? *(buf + index + 3) : T_NONE;
  return alpha == T_FULL;
}

RGB_Image::RGBA_Color RGB_Image::getPixel(int x_, int y_) const {
  RGBA_Color color;
  const char *buf = data()[0];
  buf += (y_ * w() * d() + (x_ * d()));
  color.r = *buf++;
  color.g = *buf++;
  color.b = *buf++;
  color.alpha = d() == 4 ? *buf : T_NONE;
  return color;
}

void RGB_Image::setPixel(int x_, int y_, const RGBA_Color& color_, bool alpha_/* = false*/) const {
  char *buf = (char *)data()[0];
  buf += (y_ * w() * d() + (x_ * d()));
  *buf++ = color_.r;
  *buf++ = color_.g;
  *buf++ = color_.b;
  if (d() == 4 && alpha_)
    *buf = color_.alpha;
}

void RGB_Image::setToColor(int x_, int y_, int w_, int h_,
                           const RGBA_Color &c_, bool alpha_/* = false*/) const {
  for (int y = y_; y < y_ + h_; y++) {
    if (y >= h()) break;
    for (int x = x_; x < x_ + w_; x++) {
      if (x >= w()) continue;
      setPixel(x, y, c_, alpha_);
    }
  }
}

void RGB_Image::setToColor(const RGBA_Color c_, bool alpha_/* = false*/) const {
  setToColor(0, 0, w(), h(), c_, alpha_);
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
  RGB_Image *rgb;			// full frame image
  int x, y, w, h;			// frame original dimensions
  double delay;				// delay (already converted to ms)
  int dispose;				// disposal method
  bool transparent;       // background color is transparent color
  int transparent_color_index;
  RGB_Image::RGBA_Color transparent_color;
};

struct FrameInfo {
  FrameInfo() :
    frames_size(0),
    frames(0),
    background_color_index(-1),
    canvas_w(0),
    canvas_h(0),
    debug(false) {}
  bool push_back_frame(GifFrame *frame_);
  int frames_size;
  GifFrame *frames;
  int background_color_index;
  RGB_Image::RGBA_Color background_color;
  GifFrame frame;
  int canvas_w;
  int canvas_h;
  bool debug;
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

static void setToBackGround(RGB_Image &img_, FrameInfo *_fi) {
  int bg = _fi->background_color_index;
  int tp = _fi->frames_size ? _fi->frames[ _fi->frames_size - 1].transparent_color_index :
           _fi->frame.transparent_color_index;
  DEBUG(("setToBackGround [%d] tp = %d, bg = %d\n", _fi->frames_size, tp, bg));
  RGB_Image::RGBA_Color color = _fi->background_color;
  if (tp >= 0)
    color = _fi->frames_size ?_fi->frames[ _fi->frames_size - 1].transparent_color :
            _fi->frame.transparent_color;
  if (tp >= 0 && bg >= 0)
    bg = tp;
  color.alpha = tp == bg ? RGB_Image::T_FULL : RGB_Image::T_NONE;
  DEBUG(("  setToColor %d/%d/%d alpha=%d\n", color.r, color.g, color.b, color.alpha));
  img_.setToColor(color, true);
}

static void dispose(RGB_Image &new_data, FrameInfo *_fi) {
  int frame = _fi->frames_size - 1;
  if (frame < 0) {
    setToBackGround(new_data, _fi);
    return;
  }
  switch (_fi->frames[frame].dispose) {
    case DISPOSE_PREVIOUS: {
        while (frame > 0 && _fi->frames[frame].dispose == DISPOSE_PREVIOUS)
          frame--;
        DEBUG(("     dispose frame %d to previous\n", frame + 1));
        if (frame)
          frame--;
        RGB_Image *old_data = _fi->frames[frame].rgb;
        const char *dst = new_data.data()[0];
        const char *src = old_data->data()[0];
        memcpy((char *)dst, (char *)src, _fi->canvas_w * _fi->canvas_h * 4);
        break;
      }
    case DISPOSE_BACKGROUND:
      DEBUG(("     dispose frame %d to background\n", frame + 1));
      setToBackGround(new_data, _fi);
      break;

    default: {
        RGB_Image *old_data = _fi->frames[frame].rgb;
        const char *dst = new_data.data()[0];
        const char *src = old_data->data()[0];
        memcpy((char *)dst, (char *)src, _fi->canvas_w * _fi->canvas_h * 4);
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

bool FrameInfo::push_back_frame(GifFrame *frame_) {
  void *tmp = realloc(frames, sizeof(GifFrame) * (frames_size + 1));
  if (!tmp)
    return false;
  frames = (GifFrame *)tmp;
  memcpy(&frames[ frames_size ], frame_, sizeof(GifFrame));
  frames_size++;
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
  if ((last_frame >= 0 &&  _fi->frames[last_frame].dispose == DISPOSE_BACKGROUND) ||
      _fi->frames[_frame].dispose == DISPOSE_BACKGROUND ||
      (_frame == 0 && _fi->frames[_frame].transparent))
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
  _fi->background_color_index = gifFileIn->SColorMap ? gifFileIn->SBackGroundColor : -1;
  if (_fi->background_color_index >= 0) {
    _fi->background_color = RGB_Image::RGBA_Color(
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
    // we know now everything we need about the frame
    int d = 4;
    uchar *rgb_data = new(std::nothrow) uchar[ canvas_w() * canvas_h() * d ];
    if (!rgb_data) {
      fprintf(stderr, "Fl_Anim_GIF::load(%s): Out of memory", name_);
      DGifCloseFile(gifFileIn, &errorCode);
      return false;
    }
    RGB_Image *rgb = new(std::nothrow) RGB_Image(rgb_data, canvas_w(), canvas_h(), d);
    if (!rgb) {
      delete[] rgb_data;
      fprintf(stderr, "Fl_Anim_GIF::load(): Out of memory");
      DGifCloseFile(gifFileIn, &errorCode);
      return false;
    }
    rgb->alloc_array = 1;
    frame.rgb = rgb;
    frame.transparent_color_index = gcb.TransparentColor;
    if (frame.transparent_color_index >= 0)
      frame.transparent_color = RGB_Image::RGBA_Color(
                                  ColorMap->Colors[frame.transparent_color_index].Red,
                                  ColorMap->Colors[frame.transparent_color_index].Green,
                                  ColorMap->Colors[frame.transparent_color_index].Blue);

    frame.transparent = _fi->background_color_index >= 0 && _fi->background_color_index == gcb.TransparentColor;

    dispose(*rgb, _fi);

    // copy image data to rgb
    uchar *bits = image->RasterBits;
    for (int y = frame.y; y < frame.y + frame.h; y++) {
      for (int x = frame.x; x < frame.x + frame.w; x++) {
        uchar c = *bits++;
        if (c == gcb.TransparentColor)
          continue;
        RGB_Image::RGBA_Color color(ColorMap->Colors[c].Red,
                                    ColorMap->Colors[c].Green,
                                    ColorMap->Colors[c].Blue,
                                    RGB_Image::T_NONE);
        rgb->setPixel(x, y, color, true);
      }
    }

    // coalesce transparency
    if (_fi->frames_size && _fi->frames[_fi->frames_size - 1].dispose != DISPOSE_BACKGROUND) {
      DEBUG(("   coalesce\n"));
      RGB_Image *prev = _fi->frames[_fi->frames_size - 1].rgb;
      for (int y = 0; y < canvas_h(); y++) {
        for (int x = 0; x < canvas_w(); x++) {
          if (rgb->isTransparent(x, y)) {
            rgb->setPixel(x, y, prev->getPixel(x, y), true);
          }
        }
      }
    }

    if (!_fi->push_back_frame(&frame)) {
      fprintf(stderr, "Fl_Anim_GIF::load(%s): Out of memory", name_);
      DGifCloseFile(gifFileIn, &errorCode);
      return false;
    }

    // free compressed data
    free(image->RasterBits);
    image->RasterBits = 0;
  }
  DGifCloseFile(gifFileIn, &errorCode);
  _valid = true;
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
