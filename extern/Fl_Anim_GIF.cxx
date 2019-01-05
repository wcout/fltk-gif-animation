//
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

#include "gif_load.h"

//
//  Helper classes/definitions
//

enum Transparency {
  T_NONE = 0xff,
  T_FULL = 0
};

struct RGBA_Color {
  uchar r, g, b, alpha;
  RGBA_Color(uchar r_ = 0, uchar g_ = 0, uchar b_ = 0, uchar a_ = T_NONE) :
    r(r_), g(g_), b(b_), alpha(a_) {}
};

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
  Fl_RGB_Image *rgb;                // full frame image
  Fl_Shared_Image *scalable;        // used for hardware-accelerated scaling
  Fl_Color average_color;           // last average color
  float average_weight;             // last average weight
  bool desaturated;                 // flag if frame is desaturated
  int x, y, w, h;                   // frame original dimensions
  double delay;                     // delay (already converted to ms)
  Dispose dispose;                  // disposal method
  int transparent_color_index;      // needed for dispose()
  RGBA_Color transparent_color;     // needed for dispose()
};

class FrameInfo {
friend class Fl_Anim_GIF;
  FrameInfo() :
    valid(false),
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
    _debug(0),
    optimize_mem(false),
    offscreen(0) {}
  ~FrameInfo();
  void clear();
  void copy(const FrameInfo& fi_);
  double convertDelay(int d_) const;
  int debug() const { return _debug; }
  bool load(char *buf_, long len_);
  bool push_back_frame(const GifFrame &frame_);
  void resize(int W_, int H_);
  void scale_frame(int frame_);
  void set_frame(int frame_);
private:
  bool valid;
  int frames_size;                  // number of frames stored in 'frames'
  GifFrame *frames;                 // "vector" for frames
  int loop_count;                   // loop count from file
  int loop;                         // current loop count
  int background_color_index;       // needed for dispose()
  RGBA_Color background_color;      // needed for dispose()
  GifFrame frame;                   // current processed frame
  int canvas_w;                     // width of GIF from header
  int canvas_h;                     // height of GIF from header
  bool desaturate;                  // flag if frames should be desaturated
  Fl_Color average_color;           // color for color_average()
  float average_weight;             // weight for color_average (negative: none)
  Fl_RGB_Scaling scaling;           // saved scaling method for scale_frame()
  int _debug;                       // Flag for debug outputs
  bool optimize_mem;                // Flag to store frames in original dimensions
  uchar *offscreen;                 // internal "offscreen" buffer
private:
  static void cb_gl_frame(void *ctx_, GIF_WHDR *whdr_);
  static void cb_gl_extension(void *ctx_, GIF_WHDR *whdr_);
private:
  void dispose(int frame_);
  void onFrameLoaded(GIF_WHDR &whdr_);
  void onExtensionLoaded(GIF_WHDR &whdr_);
  void setToBackGround(int frame_);
};

#define DEBUG(x) if (debug()) printf x
#define LOG(x) if (debug() >= 2) printf x
#ifndef DEBUG
  #define DEBUG(x)
#endif
#ifndef LOG
  #define LOG(x)
#endif

//
// class FrameInfo implemention
//

FrameInfo::~FrameInfo() {
  clear();
}

void FrameInfo::clear() {
  // release all allocated memory
  while (frames_size-- > 0) {
    if (frames[frames_size].scalable)
      frames[frames_size].scalable->release();
    delete frames[frames_size].rgb;
  }
  delete[] offscreen;
  offscreen = 0;
  free(frames);
  frames = 0;
  frames_size = 0;
}

double FrameInfo::convertDelay(int d_) const {
  if (d_ <= 0)
    d_ = loop_count != 1 ? 10 : 0;
  return (double)d_ / 100;
}

/*static*/
void FrameInfo::cb_gl_frame(void *ctx_, GIF_WHDR *whdr_) {
  // called from GIF_Load() when image block loaded
  FrameInfo *fi = (FrameInfo *)ctx_;
  fi->onFrameLoaded(*whdr_);
}

/*static*/
void FrameInfo::cb_gl_extension(void *ctx_, GIF_WHDR *whdr_) {
  // called from GIF_Load() when extension block loaded
  FrameInfo *fi = (FrameInfo *)ctx_;
  fi->onExtensionLoaded(*whdr_);
}

void FrameInfo::copy(const FrameInfo& fi_) {
  // copy from source
  for (int i = 0; i < fi_.frames_size; i++) {
    if (!push_back_frame(fi_.frames[i])) {
      break;
    }
    double scale_factor_x = (double)canvas_w / (double)fi_.canvas_w;
    double scale_factor_y = (double)canvas_h / (double)fi_.canvas_h;
    if (fi_.optimize_mem) {
      frames[i].x = lround(fi_.frames[i].x * scale_factor_x);
      frames[i].y = lround(fi_.frames[i].y * scale_factor_y);
      int new_w = lround(fi_.frames[i].w * scale_factor_x);
      int new_h = lround(fi_.frames[i].h * scale_factor_y);
      frames[i].w = new_w;
      frames[i].h = new_h;
    }
    // just copy data 1:1 now - scaling will be done adhoc when frame is displayed
    frames[i].rgb = (Fl_RGB_Image *)fi_.frames[i].rgb->copy();
  }
  optimize_mem = fi_.optimize_mem;
  scaling = Fl_Image::RGB_scaling(); // save current scaling mode
  loop_count = fi_.loop_count; // .. and the loop_count!
}

static void deinterlace(GIF_WHDR &whdr_) {
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

void FrameInfo::dispose(int frame_) {
  if (frame_ < 0) {
    return;
  }
  // dispose frame with index 'frame_' to offscreen buffer
  switch (frames[frame_].dispose) {
    case DISPOSE_PREVIOUS: {
        // dispose to previous restores to first not DISPOSE_TO_PREVIOUS frame
        int prev(frame_);
        while (prev > 0 && frames[prev].dispose == DISPOSE_PREVIOUS)
          prev--;
        if (prev == 0 && frames[prev].dispose == DISPOSE_PREVIOUS) {
          setToBackGround(-1);
          return;
        }
        DEBUG(("  dispose frame %d to previous frame %d\n", frame_ + 1, prev + 1));
        // copy the previous image data..
        uchar *dst = offscreen;
        const char *src = frames[prev].rgb->data()[0];
        memcpy((char *)dst, (char *)src, canvas_w * canvas_h * 4);
        break;
      }
    case DISPOSE_BACKGROUND:
      DEBUG(("  dispose frame %d to background\n", frame_ + 1));
      setToBackGround(frame_);
      break;

    default: {
        // nothing to do (keep everything as is)
        break;
      }
  }
}

bool FrameInfo::load(char *buf_, long len_) {
  // decode GIF using gif_load.h
  valid = false;
  GIF_Load(buf_, len_, cb_gl_frame, cb_gl_extension, this, 0);

  delete[] offscreen;
  offscreen = 0;
  return valid;
}

void FrameInfo::onFrameLoaded(GIF_WHDR &whdr_) {
  if (whdr_.ifrm && !valid) return; // if already invalid, just ignore rest
  int delay = whdr_.time;
  if ( delay < 0 )
    delay = -(delay + 1);

  LOG(("onFrameLoaded: frame #%d/%d, %dx%d, delay: %d, intr=%d, bkgd=%d/%d, dispose=%d\n",
        whdr_.ifrm, whdr_.nfrm, whdr_.frxd, whdr_.fryd,
        delay, whdr_.intr, whdr_.bkgd, whdr_.clrs, whdr_.mode));

  deinterlace(whdr_);

  if (!whdr_.ifrm) {
    // first frame, get width/height
    valid = true; // may be reset later from loading callback
    canvas_w = whdr_.xdim;
    canvas_h = whdr_.ydim;
    offscreen = new uchar[canvas_w * canvas_h * 4];
    memset(offscreen, 0, canvas_w * canvas_h * 4);
    background_color_index = whdr_.clrs ? whdr_.bkgd : -1;
  }

  if (!whdr_.clrs) {
    // Note: unfortunately we do not have a filename here..
    Fl::warning("GIF does not have a colormap\n");
    valid = false;
    return;
  }

  if (background_color_index >= 0) {
      background_color = RGBA_Color(whdr_.cpal[background_color_index].R,
                                    whdr_.cpal[background_color_index].G,
                                    whdr_.cpal[background_color_index].B);
  }
  // process frame
  frame.x = whdr_.frxo;
  frame.y = whdr_.fryo;
  frame.w = whdr_.frxd;
  frame.h = whdr_.fryd;
  frame.delay = convertDelay(delay);
  frame.transparent_color_index = whdr_.tran;
  frame.dispose = (Dispose)whdr_.mode;
  if (frame.transparent_color_index >= 0) {
    frame.transparent_color = RGBA_Color(whdr_.cpal[frame.transparent_color_index].R,
                                         whdr_.cpal[frame.transparent_color_index].G,
                                         whdr_.cpal[frame.transparent_color_index].B);
  }
  DEBUG(("#%d %d/%d %dx%d delay: %d, dispose: %d transparent_color: %d\n",
    (int)frames_size + 1,
    frame.x, frame.y, frame.w, frame.h,
    delay, whdr_.mode, whdr_.tran));

  // we know now everything we need about the frame..
  dispose(frames_size-1);

  // copy image data to offscreen
  uchar *bits = whdr_.bptr;
  for (int y = frame.y; y < frame.y + frame.h; y++) {
    for (int x = frame.x; x < frame.x + frame.w; x++) {
      uchar c = *bits++;
      if (c == whdr_.tran)
        continue;
      uchar *buf = offscreen;
      buf += (y * canvas_w * 4 + (x * 4));
      *buf++ = whdr_.cpal[c].R;
      *buf++ = whdr_.cpal[c].G;
      *buf++ = whdr_.cpal[c].B;
      *buf = T_NONE;
    }
  }

  // create RGB image from offscreen
  if (optimize_mem) {
    uchar *buf = new uchar[frame.w * frame.h * 4];
    uchar *dest = buf;
    for (int y = frame.y; y < frame.y + frame.h; y++) {
      for (int x = frame.x; x < frame.x + frame.w; x++) {
        memcpy(dest, &offscreen[y * canvas_w * 4 + x * 4], 4);
        dest += 4;
      }
    }
    frame.rgb = new Fl_RGB_Image(buf, frame.w, frame.h, 4);
  }
  else {
    uchar *buf = new uchar[canvas_w * canvas_h * 4];
    memcpy(buf, offscreen, canvas_w * canvas_h * 4);
    frame.rgb = new Fl_RGB_Image(buf, canvas_w, canvas_h, 4);
  }
  frame.rgb->alloc_array = 1;

  if (!push_back_frame(frame)) {
    valid = false;
  }
}

void FrameInfo::onExtensionLoaded(GIF_WHDR &whdr_) {
  uchar *ext = whdr_.bptr;
  if (memcmp(ext, "NETSCAPE2.0", 11) == 0 && ext[11] >= 3) {
    uchar *params = &ext[12];
    loop_count = params[1] | (params[2] << 8);
    DEBUG(("netscape loop count: %u\n", loop_count));
  }
}

bool FrameInfo::push_back_frame(const GifFrame &frame_) {
  void *tmp = realloc(frames, sizeof(GifFrame) * (frames_size + 1));
  if (!tmp) {
    return false;
  }
  frames = (GifFrame *)tmp;
  memcpy(&frames[frames_size], &frame_, sizeof(GifFrame));
  frames_size++;
  return true;
}

void FrameInfo::resize(int W_, int H_) {
  double scale_factor_x = (double)W_ / (double)canvas_w;
  double scale_factor_y = (double)H_ / (double)canvas_h;
  for (int i=0; i < frames_size; i++) {
    if (optimize_mem) {
      frames[i].x = lround(frames[i].x * scale_factor_x);
      frames[i].y = lround(frames[i].y * scale_factor_y);
      int new_w = lround(frames[i].w * scale_factor_x);
      int new_h = lround(frames[i].h * scale_factor_y);
      frames[i].w = new_w;
      frames[i].h = new_h;
    }
  }
  canvas_w = W_;
  canvas_h = H_;
}

void FrameInfo::scale_frame(int frame_) {
  // Do the actual scaling after a resize if neccessary
  int new_w = optimize_mem ? frames[frame_].w : canvas_w;
  int new_h = optimize_mem ? frames[frame_].h : canvas_h;
  if (frames[frame_].scalable &&
      frames[frame_].scalable->w() == new_w &&
      frames[frame_].scalable->h() == new_h)
    return;
  else if (frames[frame_].rgb->w() == new_w && frames[frame_].rgb->h() == new_h)
    return;

  Fl_RGB_Scaling old_scaling = Fl_Image::RGB_scaling(); // save current scaling method
  Fl_Image::RGB_scaling(scaling);
#if FL_ABI_VERSION >= 10304 && USE_SHIMAGE_SCALING
  if (!frames[frame_].scalable) {
    frames[frame_].scalable = Fl_Shared_Image::get(frames[frame_].rgb, 0);
  }
  frames[frame_].scalable->scale(new_w, new_h, 0, 1);
#else
  Fl_RGB_Image *copied = (Fl_RGB_Image *)frames[frame_].rgb->copy(new_w, new_h);
  delete frames[frame_].rgb;
  frames[frame_].rgb = copied;
#endif
  Fl_Image::RGB_scaling(old_scaling); // restore scaling method
}

void FrameInfo::setToBackGround(int frame_) {
  // reset offscreen to background color
  int bg = background_color_index;
  int tp = frame_ >= 0 ? frames[frame_].transparent_color_index : bg;
  DEBUG(("  setToBackGround [%d] tp = %d, bg = %d\n", frame_, tp, bg));
  RGBA_Color color = background_color;
  if (tp >= 0)
    color = frames[frame_].transparent_color;
  if (tp >= 0 && bg >= 0)
    bg = tp;
  color.alpha = tp == bg ? T_FULL : tp < 0 ? T_FULL : T_NONE;
  DEBUG(("  setToColor %d/%d/%d alpha=%d\n", color.r, color.g, color.b, color.alpha));
  for (uchar *p = offscreen + canvas_w * canvas_h * 4 - 4; p >= offscreen; p -= 4)
    memcpy(p, &color, 4);
}

void FrameInfo::set_frame(int frame_) {
  // scaling pending?
  scale_frame(frame_);

  // color average pending?
  if (average_weight >= 0 && average_weight < 1 &&
      ((average_color != frames[frame_].average_color) ||
       (average_weight != frames[frame_].average_weight))) {
    frames[frame_].rgb->color_average(average_color, average_weight);
    frames[frame_].average_color = average_color;
    frames[frame_].average_weight = average_weight;
  }

  // desaturate pending?
  if (desaturate && !frames[frame_].desaturated) {
    frames[frame_].rgb->desaturate();
    frames[frame_].desaturated = true;
  }
}


//
// Fl_Anim_GIF global variables
//

/*static*/
double Fl_Anim_GIF::min_delay = 0.;
/*static*/
bool Fl_Anim_GIF::loop = true;

//
// class Fl_Anim_GIF implementation
//

#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>	// for parent()
#include <FL/Fl.H>			// for Fl::add_timeout()
#include <FL/fl_draw.H>

void Fl_Anim_GIF::_init(const char*name_, bool start_,
                        bool optimize_mem_, int debug_) {
  _fi->_debug = debug_;
  _fi->optimize_mem = optimize_mem_;
  _valid = load(name_);
  if (canvas_w() && canvas_h()) {
    if (w() <= 0 && h() <= 0)
      size(canvas_w(), canvas_h());
  }
  if (_valid && start_)
    start();
}

Fl_Anim_GIF::Fl_Anim_GIF(int x_, int y_, int w_, int h_,
                         const char *name_ /* = 0*/,
                         bool start_ /* = true*/,
                         bool optimize_mem_/* = false*/,
                         int debug_/* = 0*/) :
  Inherited(x_, y_, w_, h_),
  _valid(false),
  _uncache(false),
  _stopped(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
    _init(name_, start_, optimize_mem_, debug_);
}

Fl_Anim_GIF::Fl_Anim_GIF(int x_, int y_,
                         const char *name_ /* = 0*/,
                         bool start_ /* = true*/,
                         bool optimize_mem_/* = false*/,
                         int debug_/* = 0*/) :
  Inherited(x_, y_, 0, 0),
  _valid(false),
  _uncache(false),
  _stopped(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
    _init(name_, start_, optimize_mem_, debug_);
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
  delete _fi;
}

void Fl_Anim_GIF::clear_frames() {
  _fi->clear();
}

int Fl_Anim_GIF::canvas_w() const {
  return _fi->canvas_w;
}

int Fl_Anim_GIF::canvas_h() const {
  return _fi->canvas_h;
}

/*static*/
void Fl_Anim_GIF::cb_animate(void *d_) {
  Fl_Anim_GIF *b = (Fl_Anim_GIF *)d_;
  b->next_frame();
}

/*virtual*/
void Fl_Anim_GIF::color_average(Fl_Color c_, float i_) {
  _fi->average_color = c_;
  _fi->average_weight = i_;
}

/*virtual*/
Fl_Anim_GIF * Fl_Anim_GIF::copy(int W_, int H_) {
  Fl_Anim_GIF *copied = new Fl_Anim_GIF();
  // copy/resize the animated gif frames (Fl_RGB_Image array)
  copied->w(W_);
  copied->h(H_);
  copied->_fi->canvas_w = W_;
  copied->_fi->canvas_h = H_;
  copied->_fi->copy(*_fi); // copy the meta data

  copied->copy_label(label());
  copied->_uncache = _uncache; // copy 'inherits' frame uncache status
  copied->_valid = _valid && copied->_fi->frames_size == _fi->frames_size;
  scale_frame(); // scale current frame now
  if (copied->_valid && _frame >= 0 && !Fl::has_timeout(cb_animate, copied))
    copied->start(); // start if original also was started
  return copied;
}

int Fl_Anim_GIF::debug() const {
  return _fi->debug();
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

void Fl_Anim_GIF::frame(int frame_) {
  if (Fl::has_timeout(cb_animate, this)) {
    Fl::warning("Fl_Anim_GIF::frame(%d): not idle!\n", frame_);
    return;
  }
  if (frame_ >= 0 && frame_ < frames()) {
    set_frame(frame_);
  }
  else {
    Fl::warning("Fl_Anim_GIF::frame(%d): out of range!\n", frame_);
  }
}

int Fl_Anim_GIF::frames() const {
  return _fi->frames_size;
}

int Fl_Anim_GIF::frame() const {
  return _frame;
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

Fl_Image *Fl_Anim_GIF::image() const {
  return _fi->frames[_frame].rgb;
}

Fl_Image *Fl_Anim_GIF::image(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].rgb;
  return 0;
}

static char *readin(const char *name_, long &sz_) {
  FILE *gif = fopen(name_, "r");
  sz_ = 0;
  char *buf = 0;
  if (!(gif && fseek(gif, 0, SEEK_END) >= 0 &&
       (sz_ = ftell( gif )) >= 0            &&
       (buf = (char *)malloc( (size_t)sz_)) &&
       fseek(gif, 0, SEEK_SET) >= 0         &&
       fread(buf, 1, (size_t)sz_, gif) == (size_t)sz_)) {
    free(buf);
    buf = 0;
  }
  if (gif) fclose(gif);
  return buf;
}

bool Fl_Anim_GIF::load(const char *name_) {
  DEBUG(("Fl_Anim_GIF:::load '%s'\n", name_));
  clear_frames();
  copy_label(name_); // TODO: store name as label() or use own field for it?

  // read gif file into memory
  long len = 0;
  char *buf = readin(name_, len);
  if (!buf) {
    Fl::error("Fl_Anim_GIF: Unable to open '%s': %s\n", name_, strerror(errno));
    return false;
  }

  // decode GIF using gif_load.h
  _fi->load(buf, len);
  free(buf);

  if (!_fi->valid) {
    Fl::error("Fl_Anim_GIF: %s has invalid format.\n", name_);
  }
  return _fi->valid;
} // load

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

Fl_Anim_GIF& Fl_Anim_GIF::resize(int W_, int H_) {
  int W(W_);
  int H(H_);
  if (!W || !H || ((W == canvas_w() && H == canvas_h()))) {
    return *this;
  }
  _fi->resize(W, H);
  scale_frame(); // scale current frame now
  size(W, H);
  return *this;
}

Fl_Anim_GIF& Fl_Anim_GIF::resize(double scale_) {
  return resize(lround((double)canvas_w() * scale_), lround((double)canvas_h() * scale_));
}

void Fl_Anim_GIF::scale_frame(int frame_/* = -1*/) {
  int i(frame_ >= 0 ? frame_ : _frame);
  if (i < 0 || i >= _fi->frames_size)
    return;
  _fi->scale_frame(i);
}

void Fl_Anim_GIF::set_frame(int frame_) {
  int last_frame = _frame;
  _frame = frame_;
  // NOTE: decreases performance, but saves a lot of memory
  if (_uncache && Inherited::image())
    Inherited::image()->uncache();

  _fi->set_frame(_frame);

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

double Fl_Anim_GIF::speed() const {
  return _speed;
}

void Fl_Anim_GIF::speed(double speed_) {
  _speed = speed_;
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

void Fl_Anim_GIF::uncache(bool uncache_) {
  _uncache = uncache_;
}

bool Fl_Anim_GIF::uncache() const {
  return _uncache;
}

bool Fl_Anim_GIF::valid() const {
  return _valid;
}
