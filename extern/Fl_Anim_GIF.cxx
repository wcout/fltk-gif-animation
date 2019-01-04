//#
// Copyright 2016-2019 Christian Grabner <wcout@gmx.net>
//
// Fl_Anim_GIF widget - FLTK animated GIF widget.
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
#ifdef FL_LIBRARY
#include <FL/Fl_Anim_GIF.H>
#else
#include "Fl_Anim_GIF.H"
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath> // lround()
#include <cstring> // strerror
#include <cerrno> // errno
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>

enum Transparency {
  T_NONE = 0xff,
  T_FULL = 0
};

struct RGBA_Color {
  uchar r, g, b, alpha;
  RGBA_Color(uchar r_ = 0, uchar g_ = 0, uchar b_ = 0, uchar a_ = T_NONE) :
    r(r_), g(g_), b(b_), alpha(a_) {}
};

#include "gif_load.h"
enum Dispose {
  DISPOSE_UNDEF = GIF_NONE,
  DISPOSE_NOT = GIF_CURR,
  DISPOSE_BACKGROUND = GIF_BKGD,
  DISPOSE_PREVIOUS = GIF_PREV
};

struct GifFrame {
  GifFrame() :
    rgb(0),
    scalable(0),
    average_color(FL_BLACK),
    average_weight(-1),
    desaturated(false),
    x(0),
    y(0),
    w(0),
    h(0),
    delay(0),
    dispose(DISPOSE_UNDEF),
    transparent_color_index(-1) {}
  Fl_RGB_Image *rgb;                       // full frame image
  Fl_Shared_Image *scalable;               // used for hardware-accelerated scaling
  Fl_Color average_color;                  // last average color
  float average_weight;                    // last average weight
  bool desaturated;                        // flag if frame is desaturated
  int x, y, w, h;                          // frame original dimensions
  double delay;                            // delay (already converted to ms)
  Dispose dispose;                         // disposal method
  int transparent_color_index;             // needed for dispose()
  RGBA_Color transparent_color;            // needed for dispose()
};

struct FrameInfo {
  FrameInfo() :
    frames_size(0),
    frames(0),
    loop_count(1),
    loop(0),
    background_color_index(-1),
    canvas_w(0),
    canvas_h(0),
    desaturate(false),
    average_color(FL_BLACK),
    average_weight(-1),
    scaling((Fl_RGB_Scaling)0),
    debug(0),
    optimize_mem(false),
    offscreen(0) {}
  int frames_size;                         // number of frames stored in 'frames'
  GifFrame *frames;                        // "vector" for frames
  int loop_count;                          // loop count from file
  int loop;                                // current loop count
  int background_color_index;              // needed for dispose()
  RGBA_Color background_color;             // needed for dispose()
  GifFrame frame;                          // current processed frame
  int canvas_w;                            // width of GIF from header
  int canvas_h;                            // height of GIF from header
  bool desaturate;                         // flag if frames should be desaturated
  Fl_Color average_color;                  // color for color_average()
  float average_weight;                    // weight for color_average (negative: none)
  Fl_RGB_Scaling scaling;                  // saved scaling method for scale_frame()
  int debug;                               // Flag for debug outputs
  bool optimize_mem;                       // Flag to store frames in original dimensions
  uchar *offscreen;                        // internal "offscreen" buffer to build frames
};

#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>	// for parent()
#include <FL/Fl.H>			// for Fl::add_timeout()

#define DEBUG(x) if ( _fi->debug ) printf x
#define LOG(x) if ( _fi->debug >= 2) printf x
//#define DEBUG(x)
//#define LOG(x)

/*static*/
double Fl_Anim_GIF::min_delay = 0.;
/*static*/
bool Fl_Anim_GIF::loop = true;


static double convertDelay(FrameInfo *fi_, int d_) {
  if (d_ <= 0)
    d_ = fi_->loop_count != 1 ? 10 : 0;
  return (double)d_ / 100;
}

// reset offscreen to background color
static void setToBackGround(uchar *offscreen_, int frame_, FrameInfo *_fi) {
  int bg = _fi->background_color_index;
  int tp = frame_ >= 0 ?_fi->frames[frame_].transparent_color_index : bg;
  DEBUG(("  setToBackGround [%d] tp = %d, bg = %d\n", frame_, tp, bg));
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
        DEBUG(("  dispose frame %d to previous frame %d\n", frame_ + 1, prev + 1));
        // copy the previous image data..
        uchar *dst = offscreen_;
        const char *src = _fi->frames[prev].rgb->data()[0];
        memcpy((char *)dst, (char *)src, _fi->canvas_w * _fi->canvas_h * 4);
        break;
      }
    case DISPOSE_BACKGROUND:
      DEBUG(("  dispose frame %d to background\n", frame_ + 1));
      setToBackGround(offscreen_, frame_, _fi);
      break;

    default: {
        // nothing to do (keep everything as is)
        break;
      }
  }
}

void Fl_Anim_GIF::init(const char*name_, bool start_, bool optimize_mem_, int debug_) {
  _fi->debug = debug_;
  _fi->optimize_mem = optimize_mem_;
  load(name_);
  if (canvas_w() && canvas_h()) {
    if (w() <= 0 && h() <= 0)
      size(canvas_w(), canvas_h());
  }
  if (_valid && start_)
    start();
}

Fl_Anim_GIF::Fl_Anim_GIF(int x_, int y_, int w_, int h_,
                         const char *name_ /* = 0*/, bool start_ /* = true*/,
                         bool optimize_mem_/* = false*/,
                         int debug_/* = 0*/) :
  Inherited(x_, y_, w_, h_),
  _valid(false),
  _uncache(false),
  _stopped(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
    init(name_, start_, optimize_mem_, debug_);
}

Fl_Anim_GIF::Fl_Anim_GIF(int x_, int y_,
                         const char *name_ /* = 0*/, bool start_ /* = true*/,
                         bool optimize_mem_/* = false*/,
                         int debug_/* = 0*/) :
  Inherited(x_, y_, 0, 0),
  _valid(false),
  _uncache(false),
  _stopped(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
    init(name_, start_, optimize_mem_, debug_);
}

Fl_Anim_GIF::Fl_Anim_GIF() :
  Inherited(0, 0, 0, 0),
  _valid(false),
  _uncache(false),
  _stopped(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
}

Fl_Anim_GIF::~Fl_Anim_GIF() {
  Fl::remove_timeout(cb_animate, this);
  clear_frames();
  delete _fi->offscreen;
  delete _fi;
}

bool Fl_Anim_GIF::start() {
  _stopped = false;
  _fi->loop = 0;
  Fl::remove_timeout(cb_animate, this);
  if (_fi->frames_size) {
    next_frame();
  }
  return _fi->frames_size != 0;
}

bool Fl_Anim_GIF::stop() {
  Fl::remove_timeout(cb_animate, this);
  _stopped = true;
  return _fi->frames_size != 0;
}

void Fl_Anim_GIF::clear_frames() {
  while (_fi->frames_size--) {
    if (_fi->frames[_fi->frames_size].scalable)
      _fi->frames[_fi->frames_size].scalable->release();
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

void Fl_Anim_GIF::scale_frame(int frame_/* = -1*/) {
  int i(frame_ >= 0 ? frame_ : _frame);
  if (i < 0 || i >= _fi->frames_size)
    return;
  Fl_RGB_Scaling scaling = Fl_Image::RGB_scaling();
  int new_w = _fi->optimize_mem ? _fi->frames[i].w : _fi->canvas_w;
  int new_h = _fi->optimize_mem ? _fi->frames[i].h : _fi->canvas_h;
  if (_fi->frames[i].scalable &&
      _fi->frames[i].scalable->w() == new_w &&
      _fi->frames[i].scalable->h() == new_h)
    return;
  else if (_fi->frames[i].rgb->w() == new_w && _fi->frames[i].rgb->h() == new_h)
    return;
  Fl_Image::RGB_scaling(_fi->scaling);
#if FL_ABI_VERSION >= 10304 && USE_SHIMAGE_SCALING
  if (!_fi->frames[i].scalable) {
    _fi->frames[i].scalable = Fl_Shared_Image::get(_fi->frames[i].rgb, 0);
  }
  _fi->frames[i].scalable->scale(new_w, new_h, 0, 1);
#else
  Fl_RGB_Image *copied = (Fl_RGB_Image *)_fi->frames[i].rgb->copy(new_w, new_h);
  delete _fi->frames[i].rgb;
  _fi->frames[i].rgb = copied;
#endif
  Fl_Image::RGB_scaling(scaling);
}

void Fl_Anim_GIF::set_frame(int frame_) {
  int last_frame = _frame;
  _frame = frame_;
  // NOTE: decreases performance, but saves a lot of memory
  if (_uncache && Inherited::image())
    Inherited::image()->uncache();

  // scaling pending?
  scale_frame();

  // color average pending?
  if (_fi->average_weight >= 0 && _fi->average_weight < 1 &&
      ((_fi->average_color != _fi->frames[_frame].average_color) ||
       (_fi->average_weight != _fi->frames[_frame].average_weight))) {
    _fi->frames[_frame].rgb->color_average(_fi->average_color, _fi->average_weight);
    _fi->frames[_frame].average_color = _fi->average_color;
    _fi->frames[_frame].average_weight = _fi->average_weight;
  }

  // desaturate pending?
  if (_fi->desaturate && !_fi->frames[_frame].desaturated) {
    _fi->frames[_frame].rgb->desaturate();
    _fi->frames[_frame].desaturated = true;
  }

  Inherited::image(image());
  if (parent() && ((last_frame >= 0 && (_fi->frames[last_frame].dispose == DISPOSE_BACKGROUND ||
     _fi->frames[last_frame].dispose == DISPOSE_PREVIOUS)) ||
     (_frame == 0 )))
    parent()->redraw();
  else
    redraw();
  static bool recurs = false;
  if (!recurs) {
    recurs = true;
    do_callback();
    recurs = false;
  }
}

bool Fl_Anim_GIF::next_frame() {
  int frame(_frame);
  frame++;
  if (frame >= _fi->frames_size) {
    _fi->loop++;
    if (Fl_Anim_GIF::loop && _fi->loop_count > 0 && _fi->loop > _fi->loop_count) {
      DEBUG(("loop count %d reached - stopped!\n", _fi->loop_count));
      stop();
    }
    else
      frame = 0;
  }
  if (frame >= _fi->frames_size)
    return false;
  set_frame(frame);
  double delay = _fi->frames[_frame].delay;
  if (_fi->loop_count != 1 && min_delay && delay < min_delay) {
    DEBUG(("#%d: correct delay %f => %f\n", frame, delay, min_delay));
    delay = min_delay;
  }
  if (!_stopped && delay > 0 && _speed > 0) {	// normal GIF has no delay
    delay /= _speed;
    Fl::add_timeout(delay, cb_animate, this);
  }
  return true;
}

static void deinterlace(_GIF_WHDR &whdr_) {
  if (!whdr_.intr) return;
  // this code is from 'gif_load's example program (with adaptions)
  int iter = 0;
  int ddst = 0;
  int ifin = 4;
  size_t sz = whdr_.frxd * whdr_.fryd;
  uchar *buf = new uchar[sz];
  memset(buf, whdr_.tran, sz);
  for (int dsrc = -1; iter < ifin; iter++)
    for (int yoff = 16 >> ((iter > 1) ? iter : 1), y = (8 >> iter) & 7;
      y < whdr_.fryd; y += yoff)
        for (int x = 0; x < whdr_.frxd; x++)
          if (whdr_.tran != (int)whdr_.bptr[++dsrc])
            buf[whdr_.frxd * y + x + ddst] = whdr_.bptr[dsrc];
  memcpy(whdr_.bptr, buf, sz);
  delete[] buf;
}

void Fl_Anim_GIF::onFrameLoaded(_GIF_WHDR &whdr_) {
  if (whdr_.ifrm && !_valid) return; // if already invalid, just ignore rest
  int delay = whdr_.time;
  if ( delay < 0 )
    delay = -(delay + 1);

  LOG(("onFrameLoaded: frame #%d/%d, %dx%d, delay: %d, intr=%d, bkgd=%d/%d, dispose=%d\n",
        whdr_.ifrm, whdr_.nfrm, whdr_.frxd, whdr_.fryd,
        delay, whdr_.intr, whdr_.bkgd, whdr_.clrs, whdr_.mode));

  deinterlace(whdr_);

  if (!whdr_.ifrm) {
    // first frame, get width/height
    _valid = true; // may be reset later from loading callback
    _fi->canvas_w = whdr_.xdim;
    _fi->canvas_h = whdr_.ydim;
    _frame = -1;
    delete _fi->offscreen;
    _fi->offscreen = (uchar *)calloc(_fi->canvas_w * _fi->canvas_h * 4, 1);
    _fi->background_color_index = whdr_.clrs ? whdr_.bkgd : -1;
  }

  if (!whdr_.clrs) {
    Fl::error("Fl_Anim_GIF '%s' does not have a colormap", label());
    _valid = false;
    return;
  }

  if (_fi->background_color_index >= 0) {
    _fi->background_color = RGBA_Color(whdr_.cpal[_fi->background_color_index].R,
                                       whdr_.cpal[_fi->background_color_index].G,
                                       whdr_.cpal[_fi->background_color_index].B);
  }
  // process frame
  GifFrame &frame = _fi->frame;
  frame.x = whdr_.frxo;
  frame.y = whdr_.fryo;
  frame.w = whdr_.frxd;
  frame.h = whdr_.fryd;
  frame.delay = convertDelay(_fi, delay);
  frame.transparent_color_index = whdr_.tran;
  frame.dispose = (Dispose)whdr_.mode;
  if (frame.transparent_color_index >= 0) {
    frame.transparent_color = RGBA_Color(whdr_.cpal[frame.transparent_color_index].R,
                                         whdr_.cpal[frame.transparent_color_index].G,
                                         whdr_.cpal[frame.transparent_color_index].B);
  }
  DEBUG(("#%d %d/%d %dx%d delay: %d, dispose: %d transparent_color: %d\n",
    (int)_fi->frames_size + 1,
    frame.x, frame.y, frame.w, frame.h,
    delay, whdr_.mode, whdr_.tran));

  // we know now everything we need about the frame..
  dispose(_frame, _fi, _fi->offscreen);

  // copy image data to offscreen
  uchar *bits = whdr_.bptr;
  for (int y = frame.y; y < frame.y + frame.h; y++) {
    for (int x = frame.x; x < frame.x + frame.w; x++) {
      uchar c = *bits++;
      if (c == whdr_.tran)
        continue;
      uchar *buf = _fi->offscreen;
      buf += (y * canvas_w() * 4 + (x * 4));
      *buf++ = whdr_.cpal[c].R;
      *buf++ = whdr_.cpal[c].G;
      *buf++ = whdr_.cpal[c].B;
      *buf = T_NONE;
    }
  }

  // create RGB image from offscreen
  if (_fi->optimize_mem) {
    uchar *buf = new uchar[frame.w * frame.h * 4];
    uchar *dest = buf;
    for (int y = frame.y; y < frame.y + frame.h; y++) {
      for (int x = frame.x; x < frame.x + frame.w; x++) {
        memcpy(dest, &_fi->offscreen[y * canvas_w() * 4 + x * 4], 4);
        dest += 4;
      }
    }
    frame.rgb = new Fl_RGB_Image(buf, frame.w, frame.h, 4);
  }
  else {
    uchar *buf = new uchar[canvas_w() * canvas_h() * 4];
    memcpy(buf, _fi->offscreen, canvas_w() * canvas_h() * 4);
    frame.rgb = new Fl_RGB_Image(buf, canvas_w(), canvas_h(), 4);
  }
  frame.rgb->alloc_array = 1;

  if (!push_back_frame(_fi, &frame)) {
    _valid = false;
    return;
  }
  _frame++;
}

/*static*/
void Fl_Anim_GIF::cb_gl_frame(void *ctx_, _GIF_WHDR *whdr_) {
  Fl_Anim_GIF *anim_gif = (Fl_Anim_GIF *)ctx_;
  anim_gif->onFrameLoaded(*whdr_);
}

void Fl_Anim_GIF::onExtensionLoaded(_GIF_WHDR &whdr_) {
  uchar *ext = whdr_.bptr;
  if (memcmp(ext, "NETSCAPE2.0", 11) == 0 && ext[11] >= 3) {
    uchar *params = &ext[12];
    _fi->loop_count = params[1] | (params[2] << 8);
     DEBUG(("netscape loop count: %u\n", _fi->loop_count));
  }
}

/*static*/
void Fl_Anim_GIF::cb_gl_extension(void *ctx_, _GIF_WHDR *whdr_) {
  Fl_Anim_GIF *anim_gif = (Fl_Anim_GIF *)ctx_;
  anim_gif->onExtensionLoaded(*whdr_);
}

bool Fl_Anim_GIF::load(const char *name_) {
  DEBUG(("Fl_Anim_GIF:::load '%s'\n", name_));
  clear_frames();
  copy_label(name_); // TODO: store name as label() or use own field for it?

  // read gif file into memory
  FILE *gif = fopen(name_, "r");
  long len = 0;
  char *buf = 0;
  if ( !(gif && fseek(gif, 0, SEEK_END) >= 0 &&
        (len = ftell( gif )) >= 0            &&
        (buf = (char *)malloc( (size_t)len)) &&
        fseek(gif, 0, SEEK_SET) >= 0         &&
        fread(buf, 1, (size_t)len, gif) == (size_t)len)) {
    Fl::error("Fl_Anim_GIF open '%s': %s", name_, strerror(errno));
    free(buf);
    if (gif) fclose(gif);
    return false;
  }
  fclose(gif);

  // decode GIF using gif_load.h
  GIF_Load(buf, len, cb_gl_frame, cb_gl_extension, this, 0);
  free(buf);

  memset(_fi->offscreen, 0, canvas_w() * canvas_h() * 4);
  return _valid;
} // load

int Fl_Anim_GIF::canvas_w() const {
  return _fi->canvas_w;
}

int Fl_Anim_GIF::canvas_h() const {
  return _fi->canvas_h;
}

double Fl_Anim_GIF::delay(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].delay;
  return 0.;
}

void Fl_Anim_GIF::delay(int frame_, double delay_) {
  if (frame_ >= 0 && frame_ < frames())
    _fi->frames[frame_].delay = delay_;
}

void Fl_Anim_GIF::frame(int frame_) {
  if (Fl::has_timeout(cb_animate, this)) {
    Fl::warning("Fl_Anim_GIF::frame(%d): not idle!", frame_);
    return;
  }
  if (frame_ >= 0 && frame_ < frames()) {
    set_frame(frame_);
  }
  else {
    Fl::warning("Fl_Anim_GIF::frame(%d): out of range!", frame_);
  }
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

double Fl_Anim_GIF::speed() const {
  return _speed;
}

void Fl_Anim_GIF::speed(double speed_) {
  _speed = speed_;
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

int Fl_Anim_GIF::frame_x(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].x;
  return -1;
}

int Fl_Anim_GIF::frame_y(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].y;
  return -1;
}

int Fl_Anim_GIF::frame_w(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].w;
  return -1;
}

int Fl_Anim_GIF::frame_h(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].h;
  return -1;
}

int Fl_Anim_GIF::debug() const {
  return _fi->debug;
}

/*static*/
void Fl_Anim_GIF::cb_animate(void *d_) {
  Fl_Anim_GIF *b = (Fl_Anim_GIF *)d_;
  b->next_frame();
}

/*virtual*/
Fl_Anim_GIF * Fl_Anim_GIF::copy(int W_, int H_) {
  Fl_Anim_GIF *copied = new Fl_Anim_GIF();
  // copy/resize the animated gif frames (Fl_RGB_Image array)
  for (int i = 0; i < _fi->frames_size; i++) {
    if (!push_back_frame(copied->_fi, &_fi->frames[i])) {
      break;
    }
    double scale_factor_x = (double)W_ / (double)canvas_w();
    double scale_factor_y = (double)H_ / (double)canvas_h();
    if (_fi->optimize_mem) {
      copied->_fi->frames[i].x = (int)((double)_fi->frames[i].x * scale_factor_x + .5);
      copied->_fi->frames[i].y = (int)((double)_fi->frames[i].y * scale_factor_y + .5);
      int new_w = (int)((double)_fi->frames[i].w * scale_factor_x + .5);
      int new_h = (int)((double)_fi->frames[i].h * scale_factor_y + .5);
      copied->_fi->frames[i].w = new_w;
      copied->_fi->frames[i].h = new_h;
    }
    // just copy data 1:1 now - scaling will be done adhoc when frame is displayed
    copied->_fi->frames[i].rgb = (Fl_RGB_Image *)_fi->frames[i].rgb->copy();
  }
  copied->w(W_);
  copied->h(H_);
  copied->_fi->canvas_w = W_;
  copied->_fi->canvas_h = H_;
  copied->_fi->optimize_mem = _fi->optimize_mem;
  copied->_fi->scaling = Fl_Image::RGB_scaling(); // save current scaling mode
  copied->_uncache = _uncache; // copy 'inherits' frame uncache status
  copied->_fi->loop_count = _fi->loop_count; // .. and the loop_count!
  copied->copy_label(label());
  copied->_valid = _valid && copied->_fi->frames_size == _fi->frames_size;
  scale_frame(); // scale current frame now
  if (copied->_valid && _frame >= 0 && !Fl::has_timeout(cb_animate, copied))
    copied->start(); // start if original also was started
  return copied;
}

Fl_Anim_GIF& Fl_Anim_GIF::resize(int W_, int H_) {
  int W(W_);
  int H(H_);
  if (!W || !H || ((W == canvas_w() && H == canvas_h()))) {
    return *this;
  }
  double scale_factor_x = (double)W / (double)canvas_w();
  double scale_factor_y = (double)H / (double)canvas_h();
  for (int i=0; i < _fi->frames_size; i++) {
    if (_fi->optimize_mem) {
      _fi->frames[i].x = (int)((double)_fi->frames[i].x * scale_factor_x + .5);
      _fi->frames[i].y = (int)((double)_fi->frames[i].y * scale_factor_y + .5);
      int new_w = (int)((double)_fi->frames[i].w * scale_factor_x + .5);
      int new_h = (int)((double)_fi->frames[i].h * scale_factor_y + .5);
      _fi->frames[i].w = new_w;
      _fi->frames[i].h = new_h;
    }
  }
  _fi->canvas_w = W;
  _fi->canvas_h = H;
  _fi->scaling = Fl_Image::RGB_scaling(); // save current scaling mode
  scale_frame(); // scale current frame now
  size(W, H);
  return *this;
}

Fl_Anim_GIF& Fl_Anim_GIF::resize(double scale_) {
  return resize(lround((double)canvas_w() * scale_), lround((double)canvas_h() * scale_));
}

/*virtual*/
void Fl_Anim_GIF::color_average(Fl_Color c_, float i_) {
  _fi->average_color = c_;
  _fi->average_weight = i_;
}

/*virtual*/
void Fl_Anim_GIF::desaturate() {
  _fi->desaturate = true;
}

/*virtual*/
void Fl_Anim_GIF::draw() {
  if (this->image()) {
    int X = x() + (w() - canvas_w()) / 2;
    int Y = y() + (h() - canvas_h()) / 2;
    if (_fi->optimize_mem) {
      int f0 = _frame;
      while (f0 > 0 && !(_fi->frames[f0].x == 0 && _fi->frames[f0].y == 0 &&
                         _fi->frames[f0].w == canvas_w() && _fi->frames[f0].h == canvas_h()))
        --f0;
      for (int f = f0; f <= _frame; f++) {
        if (f < _frame && _fi->frames[f].dispose == DISPOSE_PREVIOUS) continue;
        if (f < _frame && _fi->frames[f].dispose == DISPOSE_BACKGROUND) continue;
        scale_frame(f);
        if (_fi->frames[f].scalable) {
          _fi->frames[f].scalable->draw(X + _fi->frames[f].x, Y + _fi->frames[f].y);
        }
        else if (_fi->frames[f].rgb) {
          _fi->frames[f].rgb->draw(X + _fi->frames[f].x, Y +_fi->frames[f].y);
        }
      }
    }
    else {
      if (_fi->frames[_frame].scalable) {
        _fi->frames[_frame].scalable->draw(X, Y);
        return;
      }
      this->image()->draw(X, Y);
    }
  }
}
