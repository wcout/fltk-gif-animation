//
// "$Id: Fl_GIF_Image.cxx 10751 2015-06-14 17:07:31Z AlbrechtS $"
//
// Fl_GIF_Image routines.
//
// Copyright 1997-2015 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//
// Contents:
//
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/Fl_GIF_Image.H>
#include <stdio.h>
#include <stdlib.h>

#include "fl_gif_private.H"	// GIFLIB decoding functions

//
// This routine is a modified version of DGifImageSlurp()
// that reads only one image per time.
//

static SavedImage *DGifSlurpImage(GifFileType *GifFile) {
  size_t ImageSize;
  GifRecordType RecordType;
  SavedImage *sp;
  GifByteType *ExtData;
  int ExtFunction;

  GifFile->ExtensionBlocks = NULL;
  GifFile->ExtensionBlockCount = 0;

  do {
    if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
      return (0);

    switch (RecordType) {
      case IMAGE_DESC_RECORD_TYPE:
        if (DGifGetImageDesc(GifFile) == GIF_ERROR)
          return (0);

        sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
        /* Allocate memory for the image */
        if (sp->ImageDesc.Width < 0 && sp->ImageDesc.Height < 0 &&
            sp->ImageDesc.Width > (INT_MAX / sp->ImageDesc.Height)) {
          return 0;
        }
        ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;

        if (ImageSize > (SIZE_MAX / sizeof(GifPixelType))) {
          return 0;
        }
        sp->RasterBits = (unsigned char *)reallocarray(NULL, ImageSize,
                         sizeof(GifPixelType));

        if (sp->RasterBits == NULL) {
          return 0;
        }

        if (sp->ImageDesc.Interlace) {
          int i, j;
          /*
           * The way an interlaced image should be read -
           * offsets and jumps...
           */
          int InterlacedOffset[] = { 0, 4, 2, 1 };
          int InterlacedJumps[] = { 8, 8, 4, 2 };
          /* Need to perform 4 passes on the image */
          for (i = 0; i < 4; i++)
            for (j = InterlacedOffset[i];
                 j < sp->ImageDesc.Height;
                 j += InterlacedJumps[i]) {
              if (DGifGetLine(GifFile,
                              sp->RasterBits+j*sp->ImageDesc.Width,
                              sp->ImageDesc.Width) == GIF_ERROR)
                return 0;
            }
        } else {
          if (DGifGetLine(GifFile,sp->RasterBits,ImageSize)==GIF_ERROR)
            return (0);
        }

        if (GifFile->ExtensionBlocks) {
          sp->ExtensionBlocks = GifFile->ExtensionBlocks;
          sp->ExtensionBlockCount = GifFile->ExtensionBlockCount;

          GifFile->ExtensionBlocks = NULL;
          GifFile->ExtensionBlockCount = 0;
        }
        /* return image pointer */
        return sp;

      case EXTENSION_RECORD_TYPE:
        if (DGifGetExtension(GifFile,&ExtFunction,&ExtData) == GIF_ERROR)
          return (GIF_ERROR);
        /* Create an extension block with our data */
        if (ExtData != NULL) {
          if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
                                   &GifFile->ExtensionBlocks,
                                   ExtFunction, ExtData[0], &ExtData[1])
              == GIF_ERROR)
            return (0);
        }
        while (ExtData != NULL) {
          if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR)
            return (0);
          /* Continue the extension block */
          if (ExtData != NULL)
            if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
                                     &GifFile->ExtensionBlocks,
                                     CONTINUE_EXT_FUNC_CODE,
                                     ExtData[0], &ExtData[1]) == GIF_ERROR)
              return (0);
        }
        break;

      case TERMINATE_RECORD_TYPE:
        break;

      default:    /* Should be trapped by DGifGetRecordType */
        break;
    }
  } while (RecordType != TERMINATE_RECORD_TYPE);

  /* Sanity check for corrupted file */
  if (GifFile->ImageCount == 0) {
    GifFile->Error = D_GIF_ERR_NO_IMAG_DSCR;
    return (0);
  }

  return 0;
}


// Read a .gif file and convert it to a "xpm" format.

typedef unsigned char uchar;

/**
 The constructor loads the named GIF image.

 The destructor frees all memory and server resources that are used by
 the image.

 Use Fl_Image::fail() to check if Fl_GIF_Image failed to load. fail() returns
 ERR_FILE_ACCESS if the file could not be opened or read, ERR_FORMAT if the
 GIF format could not be decoded, and ERR_NO_IMAGE if the image could not
 be loaded for another reason.
 */
Fl_GIF_Image::Fl_GIF_Image(const char *infname, bool anim/*=false*/) : Fl_Pixmap((char *const*)0),
  gif_handle(0) {
  load(infname, anim);
}

bool Fl_GIF_Image::load(const char *infname, bool anim/* = false*/) {
  close_gif_file();
  GifFileType *gifFileIn;
  int errorCode;
  if ((gifFileIn = DGifOpenFileName(infname, &errorCode)) == NULL) {
    Fl::error("Fl_GIF_Image: Unable to open %s!", infname);
    ld(ERR_FILE_ACCESS);
    return false;
  }
  // read first image
  SavedImage *image = DGifSlurpImage(gifFileIn);
  if (!image) {
    Fl::error("Fl_GIF_Image %s: %s\n", infname, GifErrorString(errorCode));
    DGifCloseFile(gifFileIn, &errorCode);
    ld(ERR_FORMAT);
    return false;
  }
  int Width = gifFileIn->SWidth;
  int Height = gifFileIn->SHeight;
  GifImageDesc *id = &image->ImageDesc;
  ColorMapObject *ColorMap = id->ColorMap ? id->ColorMap : gifFileIn->SColorMap;
  uchar *Image = image->RasterBits;
  if (anim) {
    // make a copy of the raster data because we modify them
    // (but for animated gif we need to keep the original)
    Image = new uchar[Width * Height];
    memcpy(Image, image->RasterBits, Width * Height);
  }
  int HasColormap = ColorMap != 0;
  int ColorMapSize = ColorMap ? ColorMap->ColorCount : 0;
  // TODO: checks needed for (not) using 'colors' in image above ColorMapSize?
  //       (this was also not done in original FLTK 1.3.4 implementation)

  // Read in colormap:
  uchar transparent_pixel = 0;
  char has_transparent = 0;
  uchar Red[256], Green[256], Blue[256]; /* color map */
  if (HasColormap) {
    for (int i=0; i < ColorMapSize; i++) {
      Red[i] = ColorMap->Colors[i].Red;
      Green[i] = ColorMap->Colors[i].Green;
      Blue[i] = ColorMap->Colors[i].Blue;
    }
  } else {
    Fl::warning("%s does not have a colormap.", infname);
    for (int i = 0; i < ColorMapSize; i++)
      Red[i] = Green[i] = Blue[i] = (uchar)(255 * i / (ColorMapSize-1));
  }
  GraphicsControlBlock gcb = {};
  gcb.TransparentColor = NO_TRANSPARENT_COLOR;
  int e = image->ExtensionBlockCount;
  while (e--) {
    ExtensionBlock *ext = &image->ExtensionBlocks[e];
    if (ext->Function == GRAPHICS_EXT_FUNC_CODE) {
      DGifExtensionToGCB(ext->ByteCount,ext->Bytes,&gcb);
      break;
    }
  }
  if (gcb.TransparentColor != NO_TRANSPARENT_COLOR) {
    has_transparent = 1;
    transparent_pixel = gcb.TransparentColor;
  }

  // We are done reading the file, now convert to xpm:
  w(Width);
  h(Height);
  d(1);

  // allocate line pointer arrays:
  char **new_data = new char*[Height+2]; // Data array
  uchar *p;

  // transparent pixel must be zero, swap if it isn't:
  if (has_transparent && transparent_pixel != 0) {
    // swap transparent pixel with zero
    p = Image+Width*Height;
    while (p-- > Image) {
      if (*p==transparent_pixel) *p = 0;
      else if (!*p) *p = transparent_pixel;
    }
    uchar t;
    t                        = Red[0];
    Red[0]                   = Red[transparent_pixel];
    Red[transparent_pixel]   = t;

    t                        = Green[0];
    Green[0]                 = Green[transparent_pixel];
    Green[transparent_pixel] = t;

    t                        = Blue[0];
    Blue[0]                  = Blue[transparent_pixel];
    Blue[transparent_pixel]  = t;
  }

  // find out what colors are actually used:
  uchar used[256];
  uchar remap[256];
  int i;
  for (i = 0; i < ColorMapSize; i++) used[i] = 0;
  p = Image+Width*Height;
  while (p-- > Image) used[*p] = 1;

  // remap them to start with printing characters:
  int base = has_transparent && used[0] ? ' ' : ' '+1;
  int numcolors = 0;
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
      remap[i] = (uchar)(base++);
      numcolors++;
    }

  // write the first line of xpm data:
  char buf[100];
  int length = snprintf(buf, sizeof(buf),
                        "%d %d %d %d",Width,Height,-numcolors,1);
  new_data[0] = new char[length+1];
  strcpy(new_data[0], buf);

  // write the colormap
  new_data[1] = (char*)(p = new uchar[4*numcolors]);
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
      *p++ = remap[i];
      *p++ = Red[i];
      *p++ = Green[i];
      *p++ = Blue[i];
    }

  // remap the image data:
  p = Image+Width*Height;
  while (p-- > Image) *p = remap[*p];

  // split the image data into lines:
  for (i=0; i<Height; i++) {
    new_data[i+2] = new char[Width+1];
    memcpy(new_data[i + 2], (char*)(Image + i*Width), Width);
    new_data[i + 2][Width] = 0;
  }

  data((const char **)new_data, Height + 2);
  alloc_data = 1;

  if (anim) {
    // free temporary raster data
    delete Image;
    // store file handle
    gif_handle = gifFileIn;
  } else
    DGifCloseFile(gifFileIn, &errorCode);
  return true;
}

/*virtual*/
Fl_GIF_Image::~Fl_GIF_Image() {
  close_gif_file();
}

void Fl_GIF_Image::close_gif_file() {
  if (gif_handle) {
    DGifCloseFile((GifFileType *)gif_handle, 0);
  }
}

void *Fl_GIF_Image::read_next_image() {
  SavedImage *s = 0;
  if (gif_handle) {
    s = DGifSlurpImage((GifFileType *)gif_handle);
  }
  return s;
}

//
// Fl_Anim_GIF_Image implementation as extension of Fl_GIF_Image
//

#include <FL/Fl_Anim_GIF_Image.H>

#include <stdio.h>
#include <stdlib.h>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl.H>

//
//  Internal helper classes/structs
//

namespace {
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
}

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
  RGB_Image *rgb;                          // full frame image
  int x, y, w, h;                          // frame original dimensions
  double delay;                            // delay (already converted to ms)
  int dispose;                             // disposal method
  bool transparent;                        // background color is transparent color
  int transparent_color_index;             // needed for dispose()
  RGB_Image::RGBA_Color transparent_color; // needed for dispose()
};

struct FrameInfo {
  FrameInfo() :
    frames_size(0),
    frames(0),
    background_color_index(-1),
    canvas_w(0),
    canvas_h(0),
    debug(false) {}
  int frames_size;                         // number of frames stored in 'frames'
  GifFrame *frames;                        // "vector" for frames
  int background_color_index;              // needed for dispose()
  RGB_Image::RGBA_Color background_color;  // needed for dispose()
  GifFrame frame;                          // current processed frame
  int canvas_w;                            // width of GIF from header
  int canvas_h;                            // height of GIF from header
  bool debug;                              // Flag for debug outputs
};

#include <FL/Fl.H>			// for Fl::add_timeout()
#include <FL/Fl_Group.H>	// for Fl_Group::parent()

#define DEBUG(x) if ( _fi->debug ) printf x
//#define DEBUG(x)

static double convertDelay(int d_) {
  if (d_ <= 0)
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

Fl_Anim_GIF_Image::Fl_Anim_GIF_Image(const char *name_, Fl_Widget *canvas_ , bool start_ /* = false*/,
                                     bool debug_/* = false*/) :
  Inherited(name_, 1),
  _name(strdup(name_)),
  _canvas(canvas_),
  _uncache(false),
  _valid(false),
  _frame(-1),
  _speed(1),
  _fi(new FrameInfo()) {
  _fi->debug = debug_;
  load(name_);
  if (canvas_w() && canvas_h()) {
    if (!w() && !h()) {
      w(canvas_w());
      h(canvas_h());
    }
  }
  if (canvas()) {
    canvas()->size(w(), h());
    canvas()->image(this); // set animation as image() of canvas
  }
  if (start_)
    start();
}

/*virtual*/
Fl_Anim_GIF_Image::~Fl_Anim_GIF_Image() {
  Fl::remove_timeout(cb_animate, this);
  clear_frames();
  delete _fi;
  free(_name);
}

bool Fl_Anim_GIF_Image::start() {
  if (_fi->frames_size) {
    nextFrame();
  }
  return _fi->frames_size != 0;
}

void Fl_Anim_GIF_Image::clear_frames() {
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
  if (!tmp)
    return false;
  fi_->frames = (GifFrame *)tmp;
  memcpy(&fi_->frames[ fi_->frames_size ], frame_, sizeof(GifFrame));
  fi_->frames_size++;
  return true;
}

bool Fl_Anim_GIF_Image::nextFrame() {
  int last_frame = _frame;
  _frame++;
  if (_frame >= _fi->frames_size)
    _frame = 0;
  if (_frame >= _fi->frames_size)
    return false;

  // NOTE: uncaching decreases performance, but saves a lot of memory
  if (_uncache && this->image())
    this->image()->uncache();

  if (canvas()) {
    if ((last_frame >= 0 && _fi->frames[last_frame].dispose == DISPOSE_BACKGROUND) ||
        _fi->frames[_frame].dispose == DISPOSE_BACKGROUND ||
        (_frame == 0 && _fi->frames[_frame].transparent)) {
      if (canvas()->parent()) {
        canvas()->parent()->redraw();
      } else {
        canvas()->redraw();
      }
    } else {
      canvas()->redraw();
    }
  }
  double delay = _fi->frames[_frame].delay;
  if (delay > 0 && _speed > 0) {	// normal GIF has no delay
    delay /= _speed;
    Fl::repeat_timeout(delay, cb_animate, this);
  }
  return true;
}

/*virtual*/
void Fl_Anim_GIF_Image::draw(int x_, int y_, int w_, int h_, int cx_/* = 0*/, int cy_/* = 0*/) {
  if (this->image()) {
    this->image()->draw(x_, y_, w_, h_, cx_, cy_);
  } else
    Inherited::draw(x_, y_, w_, h_, cx_, cy_);
}

bool Fl_Anim_GIF_Image::load(const char *name_) {
  DEBUG(("Fl_Anim_GIF_Image:::load '%s'\n", name_));
  clear_frames();

  // open gif file for readin
  int errorCode(0);
  GifFileType *gifFileIn = (GifFileType *)gif_handle;
  if (!gifFileIn) {
    Inherited::load(name_);
    gifFileIn = (GifFileType *)gif_handle;
  }
  if (gifFileIn == NULL) {
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

  DEBUG(("images: %d\n", gifFileIn->ImageCount));

  // process all frames
  GraphicsControlBlock gcb = {};
  gcb.TransparentColor = NO_TRANSPARENT_COLOR;
  SavedImage *image = &gifFileIn->SavedImages[0];
  while (image) {
    GifFrame &frame = _fi->frame;
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
      // TODO: use same fix as in first image!?
      fprintf(stderr, "Gif Image does not have a colormap\n");
      close_gif_file();
      return false;
    }
    // we know now everything we need about the frame
    int d = 4;
    uchar *rgb_data = new uchar[ canvas_w() * canvas_h() * d ];
    RGB_Image *rgb = new RGB_Image(rgb_data, canvas_w(), canvas_h(), d);
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

    if (!push_back_frame(_fi, &frame)) {
      fprintf(stderr, "Fl_Anim_GIF_Image::load(%s): Out of memory", name_);
      close_gif_file();
      return false;
    }

    // free compressed data
    free(image->RasterBits);
    image->RasterBits = 0;
    image = (SavedImage *)read_next_image();
  }
  close_gif_file();
  _valid = true;
  return _valid;
}         // load

int Fl_Anim_GIF_Image::canvas_w() const {
  return _fi->canvas_w;
}

int Fl_Anim_GIF_Image::canvas_h() const {
  return _fi->canvas_h;
}

double Fl_Anim_GIF_Image::delay(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].delay;
  return 0.;
}

void Fl_Anim_GIF_Image::delay(int frame_, double delay_) {
  if (frame_ >= 0 && frame_ < frames())
    _fi->frames[frame_].delay = delay_;
}

int Fl_Anim_GIF_Image::frames() const {
  return _fi->frames_size;
}

int Fl_Anim_GIF_Image::frame() const {
  return _frame;
}

Fl_Image *Fl_Anim_GIF_Image::image() const {
  return _fi->frames[_frame].rgb;
}

Fl_Image *Fl_Anim_GIF_Image::image(int frame_) const {
  if (frame_ >= 0 && frame_ < frames())
    return _fi->frames[frame_].rgb;
  return 0;
}

Fl_Widget *Fl_Anim_GIF_Image::canvas() const {
  return _canvas;
}

void Fl_Anim_GIF_Image::canvas(Fl_Widget *canvas_) {
  if (_canvas)
    _canvas->image(0);
  _canvas = canvas_;
  if (_canvas)
    _canvas->image(this);
  _frame = -1;
  nextFrame();
}

const char *Fl_Anim_GIF_Image::name() const {
  return _name;
}

double Fl_Anim_GIF_Image::speed() const {
  return _speed;
}

void Fl_Anim_GIF_Image::speed(double speed_) {
  _speed = speed_;
}

void Fl_Anim_GIF_Image::uncache(bool uncache_) {
  _uncache = uncache_;
}

bool Fl_Anim_GIF_Image::uncache() const {
  return _uncache;
}

bool Fl_Anim_GIF_Image::valid() const {
  return _valid;
}
/*static*/
void Fl_Anim_GIF_Image::cb_animate(void *d_) {
  Fl_Anim_GIF_Image *b = (Fl_Anim_GIF_Image *)d_;
  b->nextFrame();
}

// TODO: implement Fl_Anim_GIF_Image::desaturate() / color_average() / copy()?

//
// End of "$Id: Fl_GIF_Image.cxx 10751 2015-06-14 17:07:31Z AlbrechtS $".
//
