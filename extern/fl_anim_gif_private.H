#ifndef fl_anim_gif_private_H
#define fl_anim_gif_private_H

/******************************************************************************

dgif_lib.c - GIF decoding

The functions here and in egif_lib.c are partitioned carefully so that
if you only require one of read and write capability, only one of these
two modules will be linked.  Preserve this property!

*****************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */

//#include "gif_lib.h"
/******************************************************************************
 
gif_lib.h - service library for decoding and encoding GIF images
                                                                             
*****************************************************************************/

#ifndef _GIF_LIB_H_
#define _GIF_LIB_H_ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIFLIB_MAJOR 5
#define GIFLIB_MINOR 1
#define GIFLIB_RELEASE 4

#define GIF_ERROR   0
#define GIF_OK      1

#include <stddef.h>
#include <stdbool.h>

#define GIF_STAMP "GIFVER"          /* First chars in file - GIF stamp.  */
#define GIF_STAMP_LEN sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS 3           /* Version first character in stamp. */
#define GIF87_STAMP "GIF87a"        /* First chars in file - GIF stamp.  */
#define GIF89_STAMP "GIF89a"        /* First chars in file - GIF stamp.  */

typedef unsigned char GifPixelType;
typedef unsigned char *GifRowType;
typedef unsigned char GifByteType;
typedef unsigned int GifPrefixType;
typedef int GifWord;

typedef struct GifColorType {
    GifByteType Red, Green, Blue;
} GifColorType;

typedef struct ColorMapObject {
    int ColorCount;
    int BitsPerPixel;
    bool SortFlag;
    GifColorType *Colors;    /* on malloc(3) heap */
} ColorMapObject;

typedef struct GifImageDesc {
    GifWord Left, Top, Width, Height;   /* Current image dimensions. */
    bool Interlace;                     /* Sequential/Interlaced lines. */
    ColorMapObject *ColorMap;           /* The local color map */
} GifImageDesc;

typedef struct ExtensionBlock {
    int ByteCount;
    GifByteType *Bytes; /* on malloc(3) heap */
    int Function;       /* The block function code */
#define CONTINUE_EXT_FUNC_CODE    0x00    /* continuation subblock */
#define COMMENT_EXT_FUNC_CODE     0xfe    /* comment */
#define GRAPHICS_EXT_FUNC_CODE    0xf9    /* graphics control (GIF89) */
#define PLAINTEXT_EXT_FUNC_CODE   0x01    /* plaintext */
#define APPLICATION_EXT_FUNC_CODE 0xff    /* application block */
} ExtensionBlock;

typedef struct SavedImage {
    GifImageDesc ImageDesc;
    GifByteType *RasterBits;         /* on malloc(3) heap */
    int ExtensionBlockCount;         /* Count of extensions before image */    
    ExtensionBlock *ExtensionBlocks; /* Extensions before image */    
} SavedImage;

typedef struct GifFileType {
    GifWord SWidth, SHeight;         /* Size of virtual canvas */
    GifWord SColorResolution;        /* How many colors can we generate? */
    GifWord SBackGroundColor;        /* Background color for virtual canvas */
    GifByteType AspectByte;	     /* Used to compute pixel aspect ratio */
    ColorMapObject *SColorMap;       /* Global colormap, NULL if nonexistent. */
    int ImageCount;                  /* Number of current image (both APIs) */
    GifImageDesc Image;              /* Current image (low-level API) */
    SavedImage *SavedImages;         /* Image sequence (high-level API) */
    int ExtensionBlockCount;         /* Count extensions past last image */
    ExtensionBlock *ExtensionBlocks; /* Extensions past last image */    
    int Error;			     /* Last error condition reported */
    void *UserData;                  /* hook to attach user data (TVT) */
    void *Private;                   /* Don't mess with this! */
} GifFileType;

#define GIF_ASPECT_RATIO(n)	((n)+15.0/64.0)

typedef enum {
    UNDEFINED_RECORD_TYPE,
    SCREEN_DESC_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE, /* Begin with ',' */
    EXTENSION_RECORD_TYPE,  /* Begin with '!' */
    TERMINATE_RECORD_TYPE   /* Begin with ';' */
} GifRecordType;

/* func type to read gif data from arbitrary sources (TVT) */
typedef int (*InputFunc) (GifFileType *, GifByteType *, int);

/* func type to write gif data to arbitrary targets.
 * Returns count of bytes written. (MRB)
 */
typedef int (*OutputFunc) (GifFileType *, const GifByteType *, int);

/******************************************************************************
 GIF89 structures
******************************************************************************/

typedef struct GraphicsControlBlock {
    int DisposalMode;
#define DISPOSAL_UNSPECIFIED      0       /* No disposal specified. */
#define DISPOSE_DO_NOT            1       /* Leave image in place */
#define DISPOSE_BACKGROUND        2       /* Set area too background color */
#define DISPOSE_PREVIOUS          3       /* Restore to previous content */
    bool UserInputFlag;      /* User confirmation required before disposal */
    int DelayTime;           /* pre-display delay in 0.01sec units */
    int TransparentColor;    /* Palette index for transparency, -1 if none */
#define NO_TRANSPARENT_COLOR	-1
} GraphicsControlBlock;

/******************************************************************************
 GIF encoding routines
******************************************************************************/

/* Main entry points */
GifFileType *EGifOpenFileName(const char *GifFileName,
                              const bool GifTestExistence, int *Error);
GifFileType *EGifOpenFileHandle(const int GifFileHandle, int *Error);
GifFileType *EGifOpen(void *userPtr, OutputFunc writeFunc, int *Error);
int EGifSpew(GifFileType * GifFile);
const char *EGifGetGifVersion(GifFileType *GifFile); /* new in 5.x */
int EGifCloseFile(GifFileType *GifFile, int *ErrorCode);

#define E_GIF_SUCCEEDED          0
#define E_GIF_ERR_OPEN_FAILED    1    /* And EGif possible errors. */
#define E_GIF_ERR_WRITE_FAILED   2
#define E_GIF_ERR_HAS_SCRN_DSCR  3
#define E_GIF_ERR_HAS_IMAG_DSCR  4
#define E_GIF_ERR_NO_COLOR_MAP   5
#define E_GIF_ERR_DATA_TOO_BIG   6
#define E_GIF_ERR_NOT_ENOUGH_MEM 7
#define E_GIF_ERR_DISK_IS_FULL   8
#define E_GIF_ERR_CLOSE_FAILED   9
#define E_GIF_ERR_NOT_WRITEABLE  10

/* These are legacy.  You probably do not want to call them directly */
int EGifPutScreenDesc(GifFileType *GifFile,
                      const int GifWidth, const int GifHeight, 
		      const int GifColorRes,
                      const int GifBackGround,
                      const ColorMapObject *GifColorMap);
int EGifPutImageDesc(GifFileType *GifFile, 
		     const int GifLeft, const int GifTop,
                     const int GifWidth, const int GifHeight, 
		     const bool GifInterlace,
                     const ColorMapObject *GifColorMap);
void EGifSetGifVersion(GifFileType *GifFile, const bool gif89);
int EGifPutLine(GifFileType *GifFile, GifPixelType *GifLine,
                int GifLineLen);
int EGifPutPixel(GifFileType *GifFile, const GifPixelType GifPixel);
int EGifPutComment(GifFileType *GifFile, const char *GifComment);
int EGifPutExtensionLeader(GifFileType *GifFile, const int GifExtCode);
int EGifPutExtensionBlock(GifFileType *GifFile,
                         const int GifExtLen, const void *GifExtension);
int EGifPutExtensionTrailer(GifFileType *GifFile);
int EGifPutExtension(GifFileType *GifFile, const int GifExtCode, 
		     const int GifExtLen,
                     const void *GifExtension);
int EGifPutCode(GifFileType *GifFile, int GifCodeSize,
                const GifByteType *GifCodeBlock);
int EGifPutCodeNext(GifFileType *GifFile,
                    const GifByteType *GifCodeBlock);

/******************************************************************************
 GIF decoding routines
******************************************************************************/

/* Main entry points */
GifFileType *DGifOpenFileName(const char *GifFileName, int *Error);
GifFileType *DGifOpenFileHandle(int GifFileHandle, int *Error);
int DGifSlurp(GifFileType * GifFile);
GifFileType *DGifOpen(void *userPtr, InputFunc readFunc, int *Error);    /* new one (TVT) */
    int DGifCloseFile(GifFileType * GifFile, int *ErrorCode);

#define D_GIF_SUCCEEDED          0
#define D_GIF_ERR_OPEN_FAILED    101    /* And DGif possible errors. */
#define D_GIF_ERR_READ_FAILED    102
#define D_GIF_ERR_NOT_GIF_FILE   103
#define D_GIF_ERR_NO_SCRN_DSCR   104
#define D_GIF_ERR_NO_IMAG_DSCR   105
#define D_GIF_ERR_NO_COLOR_MAP   106
#define D_GIF_ERR_WRONG_RECORD   107
#define D_GIF_ERR_DATA_TOO_BIG   108
#define D_GIF_ERR_NOT_ENOUGH_MEM 109
#define D_GIF_ERR_CLOSE_FAILED   110
#define D_GIF_ERR_NOT_READABLE   111
#define D_GIF_ERR_IMAGE_DEFECT   112
#define D_GIF_ERR_EOF_TOO_SOON   113

/* These are legacy.  You probably do not want to call them directly */
int DGifGetScreenDesc(GifFileType *GifFile);
int DGifGetRecordType(GifFileType *GifFile, GifRecordType *GifType);
int DGifGetImageDesc(GifFileType *GifFile);
int DGifGetLine(GifFileType *GifFile, GifPixelType *GifLine, int GifLineLen);
int DGifGetPixel(GifFileType *GifFile, GifPixelType GifPixel);
int DGifGetComment(GifFileType *GifFile, char *GifComment);
int DGifGetExtension(GifFileType *GifFile, int *GifExtCode,
                     GifByteType **GifExtension);
int DGifGetExtensionNext(GifFileType *GifFile, GifByteType **GifExtension);
int DGifGetCode(GifFileType *GifFile, int *GifCodeSize,
                GifByteType **GifCodeBlock);
int DGifGetCodeNext(GifFileType *GifFile, GifByteType **GifCodeBlock);
int DGifGetLZCodes(GifFileType *GifFile, int *GifCode);


/******************************************************************************
 Color table quantization (deprecated)
******************************************************************************/
int GifQuantizeBuffer(unsigned int Width, unsigned int Height,
                   int *ColorMapSize, GifByteType * RedInput,
                   GifByteType * GreenInput, GifByteType * BlueInput,
                   GifByteType * OutputBuffer,
                   GifColorType * OutputColorMap);

/******************************************************************************
 Error handling and reporting.
******************************************************************************/
extern const char *GifErrorString(int ErrorCode);     /* new in 2012 - ESR */

/*****************************************************************************
 Everything below this point is new after version 1.2, supporting `slurp
 mode' for doing I/O in two big belts with all the image-bashing in core.
******************************************************************************/

/******************************************************************************
 Color map handling from gif_alloc.c
******************************************************************************/

extern ColorMapObject *GifMakeMapObject(int ColorCount,
                                     const GifColorType *ColorMap);
extern void GifFreeMapObject(ColorMapObject *Object);
extern ColorMapObject *GifUnionColorMap(const ColorMapObject *ColorIn1,
                                     const ColorMapObject *ColorIn2,
                                     GifPixelType ColorTransIn2[]);
extern int GifBitSize(int n);

extern void *
reallocarray(void *optr, size_t nmemb, size_t size);

/******************************************************************************
 Support for the in-core structures allocation (slurp mode).              
******************************************************************************/

extern void GifApplyTranslation(SavedImage *Image, GifPixelType Translation[]);
extern int GifAddExtensionBlock(int *ExtensionBlock_Count,
				ExtensionBlock **ExtensionBlocks, 
				int Function, 
				unsigned int Len, unsigned char ExtData[]);
extern void GifFreeExtensions(int *ExtensionBlock_Count,
			      ExtensionBlock **ExtensionBlocks);
extern SavedImage *GifMakeSavedImage(GifFileType *GifFile,
                                  const SavedImage *CopyFrom);
extern void GifFreeSavedImages(GifFileType *GifFile);

/******************************************************************************
 5.x functions for GIF89 graphics control blocks
******************************************************************************/

int DGifExtensionToGCB(const size_t GifExtensionLength,
		       const GifByteType *GifExtension,
		       GraphicsControlBlock *GCB);
size_t EGifGCBToExtension(const GraphicsControlBlock *GCB,
		       GifByteType *GifExtension);

int DGifSavedExtensionToGCB(GifFileType *GifFile, 
			    int ImageIndex, 
			    GraphicsControlBlock *GCB);
int EGifGCBToSavedExtension(const GraphicsControlBlock *GCB, 
			    GifFileType *GifFile, 
			    int ImageIndex);

/******************************************************************************
 The library's internal utility font                          
******************************************************************************/

#define GIF_FONT_WIDTH  8
#define GIF_FONT_HEIGHT 8
extern const unsigned char GifAsciiTable8x8[][GIF_FONT_WIDTH];

extern void GifDrawText8x8(SavedImage *Image,
                     const int x, const int y,
                     const char *legend, const int color);

extern void GifDrawBox(SavedImage *Image,
                    const int x, const int y,
                    const int w, const int d, const int color);

extern void GifDrawRectangle(SavedImage *Image,
                   const int x, const int y,
                   const int w, const int d, const int color);

extern void GifDrawBoxedText8x8(SavedImage *Image,
                          const int x, const int y,
                          const char *legend,
                          const int border, const int bg, const int fg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _GIF_LIB_H */

/* end */

//#include "gif_lib_private.h"
/****************************************************************************

gif_lib_private.h - internal giflib routines and structures

****************************************************************************/

#ifndef _GIF_LIB_PRIVATE_H
#define _GIF_LIB_PRIVATE_H

//#include "gif_lib.h"
//#include "gif_hash.h"

/*****************************************************************************

gif_err.c - handle error reporting for the GIF library.

****************************************************************************/

#include <stdio.h>

//#include "gif_lib.h"
//#include "gif_lib_private.h"

/*****************************************************************************
 Return a string description of  the last GIF error
*****************************************************************************/
const char *
GifErrorString(int ErrorCode)
{
    const char *Err;

    switch (ErrorCode) {
      case E_GIF_ERR_OPEN_FAILED:
        Err = "Failed to open given file";
        break;
      case E_GIF_ERR_WRITE_FAILED:
        Err = "Failed to write to given file";
        break;
      case E_GIF_ERR_HAS_SCRN_DSCR:
        Err = "Screen descriptor has already been set";
        break;
      case E_GIF_ERR_HAS_IMAG_DSCR:
        Err = "Image descriptor is still active";
        break;
      case E_GIF_ERR_NO_COLOR_MAP:
        Err = "Neither global nor local color map";
        break;
      case E_GIF_ERR_DATA_TOO_BIG:
        Err = "Number of pixels bigger than width * height";
        break;
      case E_GIF_ERR_NOT_ENOUGH_MEM:
        Err = "Failed to allocate required memory";
        break;
      case E_GIF_ERR_DISK_IS_FULL:
        Err = "Write failed (disk full?)";
        break;
      case E_GIF_ERR_CLOSE_FAILED:
        Err = "Failed to close given file";
        break;
      case E_GIF_ERR_NOT_WRITEABLE:
        Err = "Given file was not opened for write";
        break;
      case D_GIF_ERR_OPEN_FAILED:
        Err = "Failed to open given file";
        break;
      case D_GIF_ERR_READ_FAILED:
        Err = "Failed to read from given file";
        break;
      case D_GIF_ERR_NOT_GIF_FILE:
        Err = "Data is not in GIF format";
        break;
      case D_GIF_ERR_NO_SCRN_DSCR:
        Err = "No screen descriptor detected";
        break;
      case D_GIF_ERR_NO_IMAG_DSCR:
        Err = "No Image Descriptor detected";
        break;
      case D_GIF_ERR_NO_COLOR_MAP:
        Err = "Neither global nor local color map";
        break;
      case D_GIF_ERR_WRONG_RECORD:
        Err = "Wrong record type detected";
        break;
      case D_GIF_ERR_DATA_TOO_BIG:
        Err = "Number of pixels bigger than width * height";
        break;
      case D_GIF_ERR_NOT_ENOUGH_MEM:
        Err = "Failed to allocate required memory";
        break;
      case D_GIF_ERR_CLOSE_FAILED:
        Err = "Failed to close given file";
        break;
      case D_GIF_ERR_NOT_READABLE:
        Err = "Given file was not opened for read";
        break;
      case D_GIF_ERR_IMAGE_DEFECT:
        Err = "Image is defective, decoding aborted";
        break;
      case D_GIF_ERR_EOF_TOO_SOON:
        Err = "Image EOF detected before image complete";
        break;
      default:
        Err = NULL;
        break;
    }
    return Err;
}

/* end */


/******************************************************************************

gif_hash.h - magfic constants and declarations for GIF LZW

******************************************************************************/

#ifndef _GIF_HASH_H_
#define _GIF_HASH_H_

#include <unistd.h>
#include <stdint.h>

#define HT_SIZE			8192	   /* 12bits = 4096 or twice as big! */
#define HT_KEY_MASK		0x1FFF			      /* 13bits keys */
#define HT_KEY_NUM_BITS		13			      /* 13bits keys */
#define HT_MAX_KEY		8191	/* 13bits - 1, maximal code possible */
#define HT_MAX_CODE		4095	/* Biggest code possible in 12 bits. */

/* The 32 bits of the long are divided into two parts for the key & code:   */
/* 1. The code is 12 bits as our compression algorithm is limited to 12bits */
/* 2. The key is 12 bits Prefix code + 8 bit new char or 20 bits.	    */
/* The key is the upper 20 bits.  The code is the lower 12. */
#define HT_GET_KEY(l)	(l >> 12)
#define HT_GET_CODE(l)	(l & 0x0FFF)
#define HT_PUT_KEY(l)	(l << 12)
#define HT_PUT_CODE(l)	(l & 0x0FFF)

typedef struct GifHashTableType {
    uint32_t HTable[HT_SIZE];
} GifHashTableType;

GifHashTableType *_InitHashTable(void);
void _ClearHashTable(GifHashTableType *HashTable);
void _InsertHashTable(GifHashTableType *HashTable, uint32_t Key, int Code);
int _ExistsHashTable(GifHashTableType *HashTable, uint32_t Key);

#endif /* _GIF_HASH_H_ */

/* end */

#ifndef SIZE_MAX
    #define SIZE_MAX     UINTPTR_MAX
#endif

#define EXTENSION_INTRODUCER      0x21
#define DESCRIPTOR_INTRODUCER     0x2c
#define TERMINATOR_INTRODUCER     0x3b

#define LZ_MAX_CODE         4095    /* Biggest code possible in 12 bits. */
#define LZ_BITS             12

#define FLUSH_OUTPUT        4096    /* Impossible code, to signal flush. */
#define FIRST_CODE          4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE        4098    /* Impossible code, to signal empty. */

#define FILE_STATE_WRITE    0x01
#define FILE_STATE_SCREEN   0x02
#define FILE_STATE_IMAGE    0x04
#define FILE_STATE_READ     0x08

#define IS_READABLE(Private)    (Private->FileState & FILE_STATE_READ)
#define IS_WRITEABLE(Private)   (Private->FileState & FILE_STATE_WRITE)

typedef struct GifFilePrivateType {
    GifWord FileState, FileHandle,  /* Where all this data goes to! */
      BitsPerPixel,     /* Bits per pixel (Codes uses at least this + 1). */
      ClearCode,   /* The CLEAR LZ code. */
      EOFCode,     /* The EOF LZ code. */
      RunningCode, /* The next code algorithm can generate. */
      RunningBits, /* The number of bits required to represent RunningCode. */
      MaxCode1,    /* 1 bigger than max. possible code, in RunningBits bits. */
      LastCode,    /* The code before the current code. */
      CrntCode,    /* Current algorithm code. */
      StackPtr,    /* For character stack (see below). */
      CrntShiftState;    /* Number of bits in CrntShiftDWord. */
    unsigned long CrntShiftDWord;   /* For bytes decomposition into codes. */
    unsigned long PixelCount;   /* Number of pixels in image. */
    FILE *File;    /* File as stream. */
    InputFunc Read;     /* function to read gif input (TVT) */
    OutputFunc Write;   /* function to write gif output (MRB) */
    GifByteType Buf[256];   /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE]; /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE + 1];    /* So we can trace the codes. */
    GifPrefixType Prefix[LZ_MAX_CODE + 1];
    GifHashTableType *HashTable;
    bool gif89;
} GifFilePrivateType;

#endif /* _GIF_LIB_PRIVATE_H */

/* end */

/* compose unsigned little endian value */
#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))

/* avoid extra function call in case we use fread (TVT) */
#define READ(_gif,_buf,_len)                                     \
  (((GifFilePrivateType*)_gif->Private)->Read ?                   \
    ((GifFilePrivateType*)_gif->Private)->Read(_gif,_buf,_len) : \
    fread(_buf,1,_len,((GifFilePrivateType*)_gif->Private)->File))

static int DGifGetWord(GifFileType *GifFile, GifWord *Word);
static int DGifSetupDecompress(GifFileType *GifFile);
static int DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line,
                              int LineLen);
static int DGifGetPrefixChar(GifPrefixType *Prefix, int Code, int ClearCode);
static int DGifDecompressInput(GifFileType *GifFile, int *Code);
static int DGifBufferedInput(GifFileType *GifFile, GifByteType *Buf,
                             GifByteType *NextByte);

/******************************************************************************
 Open a new GIF file for read, given by its name.
 Returns dynamically allocated GifFileType pointer which serves as the GIF
 info record.
******************************************************************************/
GifFileType *
DGifOpenFileName(const char *FileName, int *Error)
{
    int FileHandle;
    GifFileType *GifFile;

    if ((FileHandle = open(FileName, O_RDONLY)) == -1) {
	if (Error != NULL)
	    *Error = D_GIF_ERR_OPEN_FAILED;
        return NULL;
    }

    GifFile = DGifOpenFileHandle(FileHandle, Error);
    return GifFile;
}

/******************************************************************************
 Update a new GIF file, given its file handle.
 Returns dynamically allocated GifFileType pointer which serves as the GIF
 info record.
******************************************************************************/
GifFileType *
DGifOpenFileHandle(int FileHandle, int *Error)
{
    char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;
    FILE *f;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_ENOUGH_MEM;
        (void)close(FileHandle);
        return NULL;
    }

    /*@i1@*/memset(GifFile, '\0', sizeof(GifFileType));

    /* Belt and suspenders, in case the null pointer isn't zero */
    GifFile->SavedImages = NULL;
    GifFile->SColorMap = NULL;

    Private = (GifFilePrivateType *)calloc(1, sizeof(GifFilePrivateType));
    if (Private == NULL) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_ENOUGH_MEM;
        (void)close(FileHandle);
        free((char *)GifFile);
        return NULL;
    }

    /*@i1@*/memset(Private, '\0', sizeof(GifFilePrivateType));

#ifdef _WIN32
    _setmode(FileHandle, O_BINARY);    /* Make sure it is in binary mode. */
#endif /* _WIN32 */

    f = fdopen(FileHandle, "rb");    /* Make it into a stream: */

    /*@-mustfreeonly@*/
    GifFile->Private = (void *)Private;
    Private->FileHandle = FileHandle;
    Private->File = f;
    Private->FileState = FILE_STATE_READ;
    Private->Read = NULL;        /* don't use alternate input method (TVT) */
    GifFile->UserData = NULL;    /* TVT */
    /*@=mustfreeonly@*/

    /* Let's see if this is a GIF file: */
    /* coverity[check_return] */
    if (READ(GifFile, (unsigned char *)Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_READ_FAILED;
        (void)fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    /* Check for GIF prefix at start of file */
    Buf[GIF_STAMP_LEN] = 0;
    if (strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_GIF_FILE;
        (void)fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        (void)fclose(f);
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    GifFile->Error = 0;

    /* What version of GIF? */
    Private->gif89 = (Buf[GIF_VERSION_POS] == '9');

    return GifFile;
}

/******************************************************************************
 GifFileType constructor with user supplied input function (TVT)
******************************************************************************/
GifFileType *
DGifOpen(void *userData, InputFunc readFunc, int *Error)
{
    char Buf[GIF_STAMP_LEN + 1];
    GifFileType *GifFile;
    GifFilePrivateType *Private;

    GifFile = (GifFileType *)malloc(sizeof(GifFileType));
    if (GifFile == NULL) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_ENOUGH_MEM;
        return NULL;
    }

    memset(GifFile, '\0', sizeof(GifFileType));

    /* Belt and suspenders, in case the null pointer isn't zero */
    GifFile->SavedImages = NULL;
    GifFile->SColorMap = NULL;

    Private = (GifFilePrivateType *)calloc(1, sizeof(GifFilePrivateType));
    if (!Private) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_ENOUGH_MEM;
        free((char *)GifFile);
        return NULL;
    }
    /*@i1@*/memset(Private, '\0', sizeof(GifFilePrivateType));

    GifFile->Private = (void *)Private;
    Private->FileHandle = 0;
    Private->File = NULL;
    Private->FileState = FILE_STATE_READ;

    Private->Read = readFunc;    /* TVT */
    GifFile->UserData = userData;    /* TVT */

    /* Lets see if this is a GIF file: */
    /* coverity[check_return] */
    if (READ(GifFile, (unsigned char *)Buf, GIF_STAMP_LEN) != GIF_STAMP_LEN) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_READ_FAILED;
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    /* Check for GIF prefix at start of file */
    Buf[GIF_STAMP_LEN] = '\0';
    if (strncmp(GIF_STAMP, Buf, GIF_VERSION_POS) != 0) {
        if (Error != NULL)
	    *Error = D_GIF_ERR_NOT_GIF_FILE;
        free((char *)Private);
        free((char *)GifFile);
        return NULL;
    }

    if (DGifGetScreenDesc(GifFile) == GIF_ERROR) {
        free((char *)Private);
        free((char *)GifFile);
        if (Error != NULL)
	    *Error = D_GIF_ERR_NO_SCRN_DSCR;
        return NULL;
    }

    GifFile->Error = 0;

    /* What version of GIF? */
    Private->gif89 = (Buf[GIF_VERSION_POS] == '9');

    return GifFile;
}

/******************************************************************************
 This routine should be called before any other DGif calls. Note that
 this routine is called automatically from DGif file open routines.
******************************************************************************/
int
DGifGetScreenDesc(GifFileType *GifFile)
{
    int BitsPerPixel;
    bool SortFlag;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    /* Put the screen descriptor into the file: */
    if (DGifGetWord(GifFile, &GifFile->SWidth) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->SHeight) == GIF_ERROR)
        return GIF_ERROR;

    if (READ(GifFile, Buf, 3) != 3) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
	GifFreeMapObject(GifFile->SColorMap);
	GifFile->SColorMap = NULL;
        return GIF_ERROR;
    }
    GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
    SortFlag = (Buf[0] & 0x08) != 0;
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->SBackGroundColor = Buf[1];
    GifFile->AspectByte = Buf[2]; 
    if (Buf[0] & 0x80) {    /* Do we have global color map? */
	int i;

        GifFile->SColorMap = GifMakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->SColorMap == NULL) {
            GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        /* Get the global color map: */
	GifFile->SColorMap->SortFlag = SortFlag;
        for (i = 0; i < GifFile->SColorMap->ColorCount; i++) {
	    /* coverity[check_return] */
            if (READ(GifFile, Buf, 3) != 3) {
                GifFreeMapObject(GifFile->SColorMap);
                GifFile->SColorMap = NULL;
                GifFile->Error = D_GIF_ERR_READ_FAILED;
                return GIF_ERROR;
            }
            GifFile->SColorMap->Colors[i].Red = Buf[0];
            GifFile->SColorMap->Colors[i].Green = Buf[1];
            GifFile->SColorMap->Colors[i].Blue = Buf[2];
        }
    } else {
        GifFile->SColorMap = NULL;
    }

    /*
     * No check here for whether the background color is in range for the
     * screen color map.  Possibly there should be.
     */
    
    return GIF_OK;
}

/******************************************************************************
 This routine should be called before any attempt to read an image.
******************************************************************************/
int
DGifGetRecordType(GifFileType *GifFile, GifRecordType* Type)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    /* coverity[check_return] */
    if (READ(GifFile, &Buf, 1) != 1) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    switch (Buf) {
      case DESCRIPTOR_INTRODUCER:
          *Type = IMAGE_DESC_RECORD_TYPE;
          break;
      case EXTENSION_INTRODUCER:
          *Type = EXTENSION_RECORD_TYPE;
          break;
      case TERMINATOR_INTRODUCER:
          *Type = TERMINATE_RECORD_TYPE;
          break;
      default:
          *Type = UNDEFINED_RECORD_TYPE;
          GifFile->Error = D_GIF_ERR_WRONG_RECORD;
          return GIF_ERROR;
    }

    return GIF_OK;
}

/******************************************************************************
 This routine should be called before any attempt to read an image.
 Note it is assumed the Image desc. header has been read.
******************************************************************************/
int
DGifGetImageDesc(GifFileType *GifFile)
{
    unsigned int BitsPerPixel;
    GifByteType Buf[3];
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;
    SavedImage *sp;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (DGifGetWord(GifFile, &GifFile->Image.Left) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Top) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Width) == GIF_ERROR ||
        DGifGetWord(GifFile, &GifFile->Image.Height) == GIF_ERROR)
        return GIF_ERROR;
    if (READ(GifFile, Buf, 1) != 1) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
	GifFreeMapObject(GifFile->Image.ColorMap);
	GifFile->Image.ColorMap = NULL;
        return GIF_ERROR;
    }
    BitsPerPixel = (Buf[0] & 0x07) + 1;
    GifFile->Image.Interlace = (Buf[0] & 0x40) ? true : false;

    /* Setup the colormap */
    if (GifFile->Image.ColorMap) {
        GifFreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }
    /* Does this image have local color map? */
    if (Buf[0] & 0x80) {
	unsigned int i;

        GifFile->Image.ColorMap = GifMakeMapObject(1 << BitsPerPixel, NULL);
        if (GifFile->Image.ColorMap == NULL) {
            GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }

        /* Get the image local color map: */
        for (i = 0; i < GifFile->Image.ColorMap->ColorCount; i++) {
	    /* coverity[check_return] */
            if (READ(GifFile, Buf, 3) != 3) {
                GifFreeMapObject(GifFile->Image.ColorMap);
                GifFile->Error = D_GIF_ERR_READ_FAILED;
                GifFile->Image.ColorMap = NULL;
                return GIF_ERROR;
            }
            GifFile->Image.ColorMap->Colors[i].Red = Buf[0];
            GifFile->Image.ColorMap->Colors[i].Green = Buf[1];
            GifFile->Image.ColorMap->Colors[i].Blue = Buf[2];
        }
    }

    if (GifFile->SavedImages) {
        SavedImage* new_saved_images =
            (SavedImage *)reallocarray(GifFile->SavedImages,
                            (GifFile->ImageCount + 1), sizeof(SavedImage));
        if (new_saved_images == NULL) {
            GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
        GifFile->SavedImages = new_saved_images;
    } else {
        if ((GifFile->SavedImages =
             (SavedImage *) malloc(sizeof(SavedImage))) == NULL) {
            GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }

    sp = &GifFile->SavedImages[GifFile->ImageCount];
    memcpy(&sp->ImageDesc, &GifFile->Image, sizeof(GifImageDesc));
    if (GifFile->Image.ColorMap != NULL) {
        sp->ImageDesc.ColorMap = GifMakeMapObject(
                                 GifFile->Image.ColorMap->ColorCount,
                                 GifFile->Image.ColorMap->Colors);
        if (sp->ImageDesc.ColorMap == NULL) {
            GifFile->Error = D_GIF_ERR_NOT_ENOUGH_MEM;
            return GIF_ERROR;
        }
    }
    sp->RasterBits = (unsigned char *)NULL;
    sp->ExtensionBlockCount = 0;
    sp->ExtensionBlocks = (ExtensionBlock *) NULL;

    GifFile->ImageCount++;

    Private->PixelCount = (long)GifFile->Image.Width *
       (long)GifFile->Image.Height;

    /* Reset decompress algorithm parameters. */
    return DGifSetupDecompress(GifFile);
}

/******************************************************************************
 Get one full scanned line (Line) of length LineLen from GIF file.
******************************************************************************/
int
DGifGetLine(GifFileType *GifFile, GifPixelType *Line, int LineLen)
{
    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (!LineLen)
        LineLen = GifFile->Image.Width;

    if ((Private->PixelCount -= LineLen) > 0xffff0000UL) {
        GifFile->Error = D_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, Line, LineLen) == GIF_OK) {
        if (Private->PixelCount == 0) {
            /* We probably won't be called any more, so let's clean up
             * everything before we return: need to flush out all the
             * rest of image until an empty block (size 0)
             * detected. We use GetCodeNext.
	     */
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                    return GIF_ERROR;
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

/******************************************************************************
 Put one pixel (Pixel) into GIF file.
******************************************************************************/
int
DGifGetPixel(GifFileType *GifFile, GifPixelType Pixel)
{
    GifByteType *Dummy;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }
    if (--Private->PixelCount > 0xffff0000UL)
    {
        GifFile->Error = D_GIF_ERR_DATA_TOO_BIG;
        return GIF_ERROR;
    }

    if (DGifDecompressLine(GifFile, &Pixel, 1) == GIF_OK) {
        if (Private->PixelCount == 0) {
            /* We probably won't be called any more, so let's clean up
             * everything before we return: need to flush out all the
             * rest of image until an empty block (size 0)
             * detected. We use GetCodeNext.
	     */
            do
                if (DGifGetCodeNext(GifFile, &Dummy) == GIF_ERROR)
                    return GIF_ERROR;
            while (Dummy != NULL) ;
        }
        return GIF_OK;
    } else
        return GIF_ERROR;
}

/******************************************************************************
 Get an extension block (see GIF manual) from GIF file. This routine only
 returns the first data block, and DGifGetExtensionNext should be called
 after this one until NULL extension is returned.
 The Extension should NOT be freed by the user (not dynamically allocated).
 Note it is assumed the Extension description header has been read.
******************************************************************************/
int
DGifGetExtension(GifFileType *GifFile, int *ExtCode, GifByteType **Extension)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    /* coverity[check_return] */
    if (READ(GifFile, &Buf, 1) != 1) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    *ExtCode = Buf;

    return DGifGetExtensionNext(GifFile, Extension);
}

/******************************************************************************
 Get a following extension block (see GIF manual) from GIF file. This
 routine should be called until NULL Extension is returned.
 The Extension should NOT be freed by the user (not dynamically allocated).
******************************************************************************/
int
DGifGetExtensionNext(GifFileType *GifFile, GifByteType ** Extension)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (READ(GifFile, &Buf, 1) != 1) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }
    if (Buf > 0) {
        *Extension = Private->Buf;    /* Use private unused buffer. */
        (*Extension)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
	/* coverity[tainted_data,check_return] */
        if (READ(GifFile, &((*Extension)[1]), Buf) != Buf) {
            GifFile->Error = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else
        *Extension = NULL;

    return GIF_OK;
}

/******************************************************************************
 Extract a Graphics Control Block from raw extension data
******************************************************************************/

int DGifExtensionToGCB(const size_t GifExtensionLength,
		       const GifByteType *GifExtension,
		       GraphicsControlBlock *GCB)
{
    if (GifExtensionLength != 4) {
	return GIF_ERROR;
    }

    GCB->DisposalMode = (GifExtension[0] >> 2) & 0x07;
    GCB->UserInputFlag = (GifExtension[0] & 0x02) != 0;
    GCB->DelayTime = UNSIGNED_LITTLE_ENDIAN(GifExtension[1], GifExtension[2]);
    if (GifExtension[0] & 0x01)
	GCB->TransparentColor = (int)GifExtension[3];
    else
	GCB->TransparentColor = NO_TRANSPARENT_COLOR;

    return GIF_OK;
}

/******************************************************************************
 Extract the Graphics Control Block for a saved image, if it exists.
******************************************************************************/

int DGifSavedExtensionToGCB(GifFileType *GifFile,
			    int ImageIndex, GraphicsControlBlock *GCB)
{
    int i;

    if (ImageIndex < 0 || ImageIndex > GifFile->ImageCount - 1)
	return GIF_ERROR;

    GCB->DisposalMode = DISPOSAL_UNSPECIFIED;
    GCB->UserInputFlag = false;
    GCB->DelayTime = 0;
    GCB->TransparentColor = NO_TRANSPARENT_COLOR;

    for (i = 0; i < GifFile->SavedImages[ImageIndex].ExtensionBlockCount; i++) {
	ExtensionBlock *ep = &GifFile->SavedImages[ImageIndex].ExtensionBlocks[i];
	if (ep->Function == GRAPHICS_EXT_FUNC_CODE)
	    return DGifExtensionToGCB(ep->ByteCount, ep->Bytes, GCB);
    }

    return GIF_ERROR;
}

/******************************************************************************
 This routine should be called last, to close the GIF file.
******************************************************************************/
int
DGifCloseFile(GifFileType *GifFile, int *ErrorCode)
{
    GifFilePrivateType *Private;

    if (GifFile == NULL || GifFile->Private == NULL)
        return GIF_ERROR;

    if (GifFile->Image.ColorMap) {
        GifFreeMapObject(GifFile->Image.ColorMap);
        GifFile->Image.ColorMap = NULL;
    }

    if (GifFile->SColorMap) {
        GifFreeMapObject(GifFile->SColorMap);
        GifFile->SColorMap = NULL;
    }

    if (GifFile->SavedImages) {
        GifFreeSavedImages(GifFile);
        GifFile->SavedImages = NULL;
    }

    GifFreeExtensions(&GifFile->ExtensionBlockCount, &GifFile->ExtensionBlocks);

    Private = (GifFilePrivateType *) GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
	if (ErrorCode != NULL)
	    *ErrorCode = D_GIF_ERR_NOT_READABLE;
	free((char *)GifFile->Private);
	free(GifFile);
        return GIF_ERROR;
    }

    if (Private->File && (fclose(Private->File) != 0)) {
	if (ErrorCode != NULL)
	    *ErrorCode = D_GIF_ERR_CLOSE_FAILED;
	free((char *)GifFile->Private);
	free(GifFile);
        return GIF_ERROR;
    }

    free((char *)GifFile->Private);
    free(GifFile);
    if (ErrorCode != NULL)
	*ErrorCode = D_GIF_SUCCEEDED;
    return GIF_OK;
}

/******************************************************************************
 Get 2 bytes (word) from the given file:
******************************************************************************/
static int
DGifGetWord(GifFileType *GifFile, GifWord *Word)
{
    unsigned char c[2];

    /* coverity[check_return] */
    if (READ(GifFile, c, 2) != 2) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    *Word = (GifWord)UNSIGNED_LITTLE_ENDIAN(c[0], c[1]);
    return GIF_OK;
}

/******************************************************************************
 Get the image code in compressed form.  This routine can be called if the
 information needed to be piped out as is. Obviously this is much faster
 than decoding and encoding again. This routine should be followed by calls
 to DGifGetCodeNext, until NULL block is returned.
 The block should NOT be freed by the user (not dynamically allocated).
******************************************************************************/
int
DGifGetCode(GifFileType *GifFile, int *CodeSize, GifByteType **CodeBlock)
{
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    *CodeSize = Private->BitsPerPixel;

    return DGifGetCodeNext(GifFile, CodeBlock);
}

/******************************************************************************
 Continue to get the image code in compressed form. This routine should be
 called until NULL block is returned.
 The block should NOT be freed by the user (not dynamically allocated).
******************************************************************************/
int
DGifGetCodeNext(GifFileType *GifFile, GifByteType **CodeBlock)
{
    GifByteType Buf;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    /* coverity[tainted_data_argument] */
    /* coverity[check_return] */
    if (READ(GifFile, &Buf, 1) != 1) {
        GifFile->Error = D_GIF_ERR_READ_FAILED;
        return GIF_ERROR;
    }

    /* coverity[lower_bounds] */
    if (Buf > 0) {
        *CodeBlock = Private->Buf;    /* Use private unused buffer. */
        (*CodeBlock)[0] = Buf;  /* Pascal strings notation (pos. 0 is len.). */
	/* coverity[tainted_data] */
        if (READ(GifFile, &((*CodeBlock)[1]), Buf) != Buf) {
            GifFile->Error = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
    } else {
        *CodeBlock = NULL;
        Private->Buf[0] = 0;    /* Make sure the buffer is empty! */
        Private->PixelCount = 0;    /* And local info. indicate image read. */
    }

    return GIF_OK;
}

/******************************************************************************
 Setup the LZ decompression for this image:
******************************************************************************/
static int
DGifSetupDecompress(GifFileType *GifFile)
{
    int i, BitsPerPixel;
    GifByteType CodeSize;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    /* coverity[check_return] */
    if (READ(GifFile, &CodeSize, 1) < 1) {    /* Read Code size from file. */
	return GIF_ERROR;    /* Failed to read Code size. */
    }
    BitsPerPixel = CodeSize;

    /* this can only happen on a severely malformed GIF */
    if (BitsPerPixel > 8) {
	GifFile->Error = D_GIF_ERR_READ_FAILED;	/* somewhat bogus error code */
	return GIF_ERROR;    /* Failed to read Code size. */
    }

    Private->Buf[0] = 0;    /* Input Buffer empty. */
    Private->BitsPerPixel = BitsPerPixel;
    Private->ClearCode = (1 << BitsPerPixel);
    Private->EOFCode = Private->ClearCode + 1;
    Private->RunningCode = Private->EOFCode + 1;
    Private->RunningBits = BitsPerPixel + 1;    /* Number of bits per code. */
    Private->MaxCode1 = 1 << Private->RunningBits;    /* Max. code + 1. */
    Private->StackPtr = 0;    /* No pixels on the pixel stack. */
    Private->LastCode = NO_SUCH_CODE;
    Private->CrntShiftState = 0;    /* No information in CrntShiftDWord. */
    Private->CrntShiftDWord = 0;

    Prefix = Private->Prefix;
    for (i = 0; i <= LZ_MAX_CODE; i++)
        Prefix[i] = NO_SUCH_CODE;

    return GIF_OK;
}

/******************************************************************************
 The LZ decompression routine:
 This version decompress the given GIF file into Line of length LineLen.
 This routine can be called few times (one per scan line, for example), in
 order the complete the whole image.
******************************************************************************/
static int
DGifDecompressLine(GifFileType *GifFile, GifPixelType *Line, int LineLen)
{
    int i = 0;
    int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
    GifByteType *Stack, *Suffix;
    GifPrefixType *Prefix;
    GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

    StackPtr = Private->StackPtr;
    Prefix = Private->Prefix;
    Suffix = Private->Suffix;
    Stack = Private->Stack;
    EOFCode = Private->EOFCode;
    ClearCode = Private->ClearCode;
    LastCode = Private->LastCode;

    if (StackPtr > LZ_MAX_CODE) {
        return GIF_ERROR;
    }

    if (StackPtr != 0) {
        /* Let pop the stack off before continueing to read the GIF file: */
        while (StackPtr != 0 && i < LineLen)
            Line[i++] = Stack[--StackPtr];
    }

    while (i < LineLen) {    /* Decode LineLen items. */
        if (DGifDecompressInput(GifFile, &CrntCode) == GIF_ERROR)
            return GIF_ERROR;

        if (CrntCode == EOFCode) {
            /* Note however that usually we will not be here as we will stop
             * decoding as soon as we got all the pixel, or EOF code will
             * not be read at all, and DGifGetLine/Pixel clean everything.  */
	    GifFile->Error = D_GIF_ERR_EOF_TOO_SOON;
	    return GIF_ERROR;
        } else if (CrntCode == ClearCode) {
            /* We need to start over again: */
            for (j = 0; j <= LZ_MAX_CODE; j++)
                Prefix[j] = NO_SUCH_CODE;
            Private->RunningCode = Private->EOFCode + 1;
            Private->RunningBits = Private->BitsPerPixel + 1;
            Private->MaxCode1 = 1 << Private->RunningBits;
            LastCode = Private->LastCode = NO_SUCH_CODE;
        } else {
            /* Its regular code - if in pixel range simply add it to output
             * stream, otherwise trace to codes linked list until the prefix
             * is in pixel range: */
            if (CrntCode < ClearCode) {
                /* This is simple - its pixel scalar, so add it to output: */
                Line[i++] = CrntCode;
            } else {
                /* Its a code to needed to be traced: trace the linked list
                 * until the prefix is a pixel, while pushing the suffix
                 * pixels on our stack. If we done, pop the stack in reverse
                 * (thats what stack is good for!) order to output.  */
                if (Prefix[CrntCode] == NO_SUCH_CODE) {
                    CrntPrefix = LastCode;

                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    if (CrntCode == Private->RunningCode - 2) {
                        Suffix[Private->RunningCode - 2] =
                           Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
                                                                 LastCode,
                                                                 ClearCode);
                    } else {
                        Suffix[Private->RunningCode - 2] =
                           Stack[StackPtr++] = DGifGetPrefixChar(Prefix,
                                                                 CrntCode,
                                                                 ClearCode);
                    }
                } else
                    CrntPrefix = CrntCode;

                /* Now (if image is O.K.) we should not get a NO_SUCH_CODE
                 * during the trace. As we might loop forever, in case of
                 * defective image, we use StackPtr as loop counter and stop
                 * before overflowing Stack[]. */
                while (StackPtr < LZ_MAX_CODE &&
                       CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
                    Stack[StackPtr++] = Suffix[CrntPrefix];
                    CrntPrefix = Prefix[CrntPrefix];
                }
                if (StackPtr >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
                    GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
                    return GIF_ERROR;
                }
                /* Push the last character on stack: */
                Stack[StackPtr++] = CrntPrefix;

                /* Now lets pop all the stack into output: */
                while (StackPtr != 0 && i < LineLen)
                    Line[i++] = Stack[--StackPtr];
            }
            if (LastCode != NO_SUCH_CODE && Prefix[Private->RunningCode - 2] == NO_SUCH_CODE) {
                Prefix[Private->RunningCode - 2] = LastCode;

                if (CrntCode == Private->RunningCode - 2) {
                    /* Only allowed if CrntCode is exactly the running code:
                     * In that case CrntCode = XXXCode, CrntCode or the
                     * prefix code is last code and the suffix char is
                     * exactly the prefix of last code! */
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, LastCode, ClearCode);
                } else {
                    Suffix[Private->RunningCode - 2] =
                       DGifGetPrefixChar(Prefix, CrntCode, ClearCode);
                }
            }
            LastCode = CrntCode;
        }
    }

    Private->LastCode = LastCode;
    Private->StackPtr = StackPtr;

    return GIF_OK;
}

/******************************************************************************
 Routine to trace the Prefixes linked list until we get a prefix which is
 not code, but a pixel value (less than ClearCode). Returns that pixel value.
 If image is defective, we might loop here forever, so we limit the loops to
 the maximum possible if image O.k. - LZ_MAX_CODE times.
******************************************************************************/
static int
DGifGetPrefixChar(GifPrefixType *Prefix, int Code, int ClearCode)
{
    int i = 0;

    while (Code > ClearCode && i++ <= LZ_MAX_CODE) {
        if (Code > LZ_MAX_CODE) {
            return NO_SUCH_CODE;
        }
        Code = Prefix[Code];
    }
    return Code;
}

/******************************************************************************
 Interface for accessing the LZ codes directly. Set Code to the real code
 (12bits), or to -1 if EOF code is returned.
******************************************************************************/
int
DGifGetLZCodes(GifFileType *GifFile, int *Code)
{
    GifByteType *CodeBlock;
    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    if (!IS_READABLE(Private)) {
        /* This file was NOT open for reading: */
        GifFile->Error = D_GIF_ERR_NOT_READABLE;
        return GIF_ERROR;
    }

    if (DGifDecompressInput(GifFile, Code) == GIF_ERROR)
        return GIF_ERROR;

    if (*Code == Private->EOFCode) {
        /* Skip rest of codes (hopefully only NULL terminating block): */
        do {
            if (DGifGetCodeNext(GifFile, &CodeBlock) == GIF_ERROR)
                return GIF_ERROR;
        } while (CodeBlock != NULL) ;

        *Code = -1;
    } else if (*Code == Private->ClearCode) {
        /* We need to start over again: */
        Private->RunningCode = Private->EOFCode + 1;
        Private->RunningBits = Private->BitsPerPixel + 1;
        Private->MaxCode1 = 1 << Private->RunningBits;
    }

    return GIF_OK;
}

/******************************************************************************
 The LZ decompression input routine:
 This routine is responsable for the decompression of the bit stream from
 8 bits (bytes) packets, into the real codes.
 Returns GIF_OK if read successfully.
******************************************************************************/
static int
DGifDecompressInput(GifFileType *GifFile, int *Code)
{
    static const unsigned short CodeMasks[] = {
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000f, 0x001f, 0x003f, 0x007f,
	0x00ff, 0x01ff, 0x03ff, 0x07ff,
	0x0fff
    };

    GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

    GifByteType NextByte;

    /* The image can't contain more than LZ_BITS per code. */
    if (Private->RunningBits > LZ_BITS) {
        GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
        return GIF_ERROR;
    }
    
    while (Private->CrntShiftState < Private->RunningBits) {
        /* Needs to get more bytes from input stream for next code: */
        if (DGifBufferedInput(GifFile, Private->Buf, &NextByte) == GIF_ERROR) {
            return GIF_ERROR;
        }
        Private->CrntShiftDWord |=
	    ((unsigned long)NextByte) << Private->CrntShiftState;
        Private->CrntShiftState += 8;
    }
    *Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

    Private->CrntShiftDWord >>= Private->RunningBits;
    Private->CrntShiftState -= Private->RunningBits;

    /* If code cannot fit into RunningBits bits, must raise its size. Note
     * however that codes above 4095 are used for special signaling.
     * If we're using LZ_BITS bits already and we're at the max code, just
     * keep using the table as it is, don't increment Private->RunningCode.
     */
    if (Private->RunningCode < LZ_MAX_CODE + 2 &&
	++Private->RunningCode > Private->MaxCode1 &&
	Private->RunningBits < LZ_BITS) {
        Private->MaxCode1 <<= 1;
        Private->RunningBits++;
    }
    return GIF_OK;
}

/******************************************************************************
 This routines read one GIF data block at a time and buffers it internally
 so that the decompression routine could access it.
 The routine returns the next byte from its internal buffer (or read next
 block in if buffer empty) and returns GIF_OK if succesful.
******************************************************************************/
static int
DGifBufferedInput(GifFileType *GifFile, GifByteType *Buf, GifByteType *NextByte)
{
    if (Buf[0] == 0) {
        /* Needs to read the next buffer - this one is empty: */
	/* coverity[check_return] */
        if (READ(GifFile, Buf, 1) != 1) {
            GifFile->Error = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        /* There shouldn't be any empty data blocks here as the LZW spec
         * says the LZW termination code should come first.  Therefore we
         * shouldn't be inside this routine at that point.
         */
        if (Buf[0] == 0) {
            GifFile->Error = D_GIF_ERR_IMAGE_DEFECT;
            return GIF_ERROR;
        }
        if (READ(GifFile, &Buf[1], Buf[0]) != Buf[0]) {
            GifFile->Error = D_GIF_ERR_READ_FAILED;
            return GIF_ERROR;
        }
        *NextByte = Buf[1];
        Buf[1] = 2;    /* We use now the second place as last char read! */
        Buf[0]--;
    } else {
        *NextByte = Buf[Buf[1]++];
        Buf[0]--;
    }

    return GIF_OK;
}

/******************************************************************************
 This routine reads an entire GIF into core, hanging all its state info off
 the GifFileType pointer.  Call DGifOpenFileName() or DGifOpenFileHandle()
 first to initialize I/O.  Its inverse is EGifSpew().
*******************************************************************************/
int
DGifSlurp(GifFileType *GifFile)
{
    size_t ImageSize;
    GifRecordType RecordType;
    SavedImage *sp;
    GifByteType *ExtData;
    int ExtFunction;

    GifFile->ExtensionBlocks = NULL;
    GifFile->ExtensionBlockCount = 0;

    do {
        if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
            return (GIF_ERROR);

        switch (RecordType) {
          case IMAGE_DESC_RECORD_TYPE:
              if (DGifGetImageDesc(GifFile) == GIF_ERROR)
                  return (GIF_ERROR);

              sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
              /* Allocate memory for the image */
              if (sp->ImageDesc.Width < 0 && sp->ImageDesc.Height < 0 &&
                      sp->ImageDesc.Width > (INT_MAX / sp->ImageDesc.Height)) {
                  return GIF_ERROR;
              }
              ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;

              if (ImageSize > (SIZE_MAX / sizeof(GifPixelType))) {
                  return GIF_ERROR;
              }
              sp->RasterBits = (unsigned char *)reallocarray(NULL, ImageSize,
                      sizeof(GifPixelType));

              if (sp->RasterBits == NULL) {
                  return GIF_ERROR;
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
			      return GIF_ERROR;
		      }
	      }
	      else {
		  if (DGifGetLine(GifFile,sp->RasterBits,ImageSize)==GIF_ERROR)
		      return (GIF_ERROR);
	      }

              if (GifFile->ExtensionBlocks) {
                  sp->ExtensionBlocks = GifFile->ExtensionBlocks;
                  sp->ExtensionBlockCount = GifFile->ExtensionBlockCount;

                  GifFile->ExtensionBlocks = NULL;
                  GifFile->ExtensionBlockCount = 0;
              }
              break;

          case EXTENSION_RECORD_TYPE:
              if (DGifGetExtension(GifFile,&ExtFunction,&ExtData) == GIF_ERROR)
                  return (GIF_ERROR);
	      /* Create an extension block with our data */
              if (ExtData != NULL) {
		  if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
					   &GifFile->ExtensionBlocks, 
					   ExtFunction, ExtData[0], &ExtData[1])
		      == GIF_ERROR)
		      return (GIF_ERROR);
	      }
              while (ExtData != NULL) {
                  if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR)
                      return (GIF_ERROR);
                  /* Continue the extension block */
		  if (ExtData != NULL)
		      if (GifAddExtensionBlock(&GifFile->ExtensionBlockCount,
					       &GifFile->ExtensionBlocks,
					       CONTINUE_EXT_FUNC_CODE, 
					       ExtData[0], &ExtData[1]) == GIF_ERROR)
                      return (GIF_ERROR);
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
	return(GIF_ERROR);
    }

    return (GIF_OK);
}

/* end */

/*****************************************************************************

 GIF construction tools

****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "gif_lib.h"

#define MAX(x, y)    (((x) > (y)) ? (x) : (y))

/******************************************************************************
 Miscellaneous utility functions                          
******************************************************************************/

/* return smallest bitfield size n will fit in */
int
GifBitSize(int n)
{
    register int i;

    for (i = 1; i <= 8; i++)
        if ((1 << i) >= n)
            break;
    return (i);
}

/******************************************************************************
  Color map object functions                              
******************************************************************************/

/*
 * Allocate a color map of given size; initialize with contents of
 * ColorMap if that pointer is non-NULL.
 */
ColorMapObject *
GifMakeMapObject(int ColorCount, const GifColorType *ColorMap)
{
    ColorMapObject *Object;

    /*** FIXME: Our ColorCount has to be a power of two.  Is it necessary to
     * make the user know that or should we automatically round up instead? */
    if (ColorCount != (1 << GifBitSize(ColorCount))) {
        return ((ColorMapObject *) NULL);
    }
    
    Object = (ColorMapObject *)malloc(sizeof(ColorMapObject));
    if (Object == (ColorMapObject *) NULL) {
        return ((ColorMapObject *) NULL);
    }

    Object->Colors = (GifColorType *)calloc(ColorCount, sizeof(GifColorType));
    if (Object->Colors == (GifColorType *) NULL) {
	free(Object);
        return ((ColorMapObject *) NULL);
    }

    Object->ColorCount = ColorCount;
    Object->BitsPerPixel = GifBitSize(ColorCount);
    Object->SortFlag = false;

    if (ColorMap != NULL) {
        memcpy((char *)Object->Colors,
               (char *)ColorMap, ColorCount * sizeof(GifColorType));
    }

    return (Object);
}

/*******************************************************************************
Free a color map object
*******************************************************************************/
void
GifFreeMapObject(ColorMapObject *Object)
{
    if (Object != NULL) {
        (void)free(Object->Colors);
        (void)free(Object);
    }
}

#ifdef DEBUG
void
DumpColorMap(ColorMapObject *Object,
             FILE * fp)
{
    if (Object != NULL) {
        int i, j, Len = Object->ColorCount;

        for (i = 0; i < Len; i += 4) {
            for (j = 0; j < 4 && j < Len; j++) {
                (void)fprintf(fp, "%3d: %02x %02x %02x   ", i + j,
			      Object->Colors[i + j].Red,
			      Object->Colors[i + j].Green,
			      Object->Colors[i + j].Blue);
            }
            (void)fprintf(fp, "\n");
        }
    }
}
#endif /* DEBUG */

/*******************************************************************************
 Compute the union of two given color maps and return it.  If result can't 
 fit into 256 colors, NULL is returned, the allocated union otherwise.
 ColorIn1 is copied as is to ColorUnion, while colors from ColorIn2 are
 copied iff they didn't exist before.  ColorTransIn2 maps the old
 ColorIn2 into the ColorUnion color map table./
*******************************************************************************/
ColorMapObject *
GifUnionColorMap(const ColorMapObject *ColorIn1,
              const ColorMapObject *ColorIn2,
              GifPixelType ColorTransIn2[])
{
    int i, j, CrntSlot, RoundUpTo, NewGifBitSize;
    ColorMapObject *ColorUnion;

    /*
     * We don't worry about duplicates within either color map; if
     * the caller wants to resolve those, he can perform unions
     * with an empty color map.
     */

    /* Allocate table which will hold the result for sure. */
    ColorUnion = GifMakeMapObject(MAX(ColorIn1->ColorCount,
                               ColorIn2->ColorCount) * 2, NULL);

    if (ColorUnion == NULL)
        return (NULL);

    /* 
     * Copy ColorIn1 to ColorUnion.
     */
    for (i = 0; i < ColorIn1->ColorCount; i++)
        ColorUnion->Colors[i] = ColorIn1->Colors[i];
    CrntSlot = ColorIn1->ColorCount;

    /* 
     * Potentially obnoxious hack:
     *
     * Back CrntSlot down past all contiguous {0, 0, 0} slots at the end
     * of table 1.  This is very useful if your display is limited to
     * 16 colors.
     */
    while (ColorIn1->Colors[CrntSlot - 1].Red == 0
           && ColorIn1->Colors[CrntSlot - 1].Green == 0
           && ColorIn1->Colors[CrntSlot - 1].Blue == 0)
        CrntSlot--;

    /* Copy ColorIn2 to ColorUnion (use old colors if they exist): */
    for (i = 0; i < ColorIn2->ColorCount && CrntSlot <= 256; i++) {
        /* Let's see if this color already exists: */
        for (j = 0; j < ColorIn1->ColorCount; j++)
            if (memcmp (&ColorIn1->Colors[j], &ColorIn2->Colors[i], 
                        sizeof(GifColorType)) == 0)
                break;

        if (j < ColorIn1->ColorCount)
            ColorTransIn2[i] = j;    /* color exists in Color1 */
        else {
            /* Color is new - copy it to a new slot: */
            ColorUnion->Colors[CrntSlot] = ColorIn2->Colors[i];
            ColorTransIn2[i] = CrntSlot++;
        }
    }

    if (CrntSlot > 256) {
        GifFreeMapObject(ColorUnion);
        return ((ColorMapObject *) NULL);
    }

    NewGifBitSize = GifBitSize(CrntSlot);
    RoundUpTo = (1 << NewGifBitSize);

    if (RoundUpTo != ColorUnion->ColorCount) {
        register GifColorType *Map = ColorUnion->Colors;

        /* 
         * Zero out slots up to next power of 2.
         * We know these slots exist because of the way ColorUnion's
         * start dimension was computed.
         */
        for (j = CrntSlot; j < RoundUpTo; j++)
            Map[j].Red = Map[j].Green = Map[j].Blue = 0;

        /* perhaps we can shrink the map? */
        if (RoundUpTo < ColorUnion->ColorCount) {
            GifColorType *new_map = (GifColorType *)reallocarray(Map,
                                 RoundUpTo, sizeof(GifColorType));
            if( new_map == NULL ) {
                GifFreeMapObject(ColorUnion);
                return ((ColorMapObject *) NULL);
            }
            ColorUnion->Colors = new_map;
        }
    }

    ColorUnion->ColorCount = RoundUpTo;
    ColorUnion->BitsPerPixel = NewGifBitSize;

    return (ColorUnion);
}

/*******************************************************************************
 Apply a given color translation to the raster bits of an image
*******************************************************************************/
void
GifApplyTranslation(SavedImage *Image, GifPixelType Translation[])
{
    register int i;
    register int RasterSize = Image->ImageDesc.Height * Image->ImageDesc.Width;

    for (i = 0; i < RasterSize; i++)
        Image->RasterBits[i] = Translation[Image->RasterBits[i]];
}

/******************************************************************************
 Extension record functions                              
******************************************************************************/
int
GifAddExtensionBlock(int *ExtensionBlockCount,
		     ExtensionBlock **ExtensionBlocks,
		     int Function,
		     unsigned int Len,
		     unsigned char ExtData[])
{
    ExtensionBlock *ep;

    if (*ExtensionBlocks == NULL)
        *ExtensionBlocks=(ExtensionBlock *)malloc(sizeof(ExtensionBlock));
    else {
        ExtensionBlock* ep_new = (ExtensionBlock *)reallocarray
				 (*ExtensionBlocks, (*ExtensionBlockCount + 1),
                                      sizeof(ExtensionBlock));
        if( ep_new == NULL )
            return (GIF_ERROR);
        *ExtensionBlocks = ep_new;
    }

    if (*ExtensionBlocks == NULL)
        return (GIF_ERROR);

    ep = &(*ExtensionBlocks)[(*ExtensionBlockCount)++];

    ep->Function = Function;
    ep->ByteCount=Len;
    ep->Bytes = (GifByteType *)malloc(ep->ByteCount);
    if (ep->Bytes == NULL)
        return (GIF_ERROR);

    if (ExtData != NULL) {
        memcpy(ep->Bytes, ExtData, Len);
    }

    return (GIF_OK);
}

void
GifFreeExtensions(int *ExtensionBlockCount,
		  ExtensionBlock **ExtensionBlocks)
{
    ExtensionBlock *ep;

    if (*ExtensionBlocks == NULL)
        return;

    for (ep = *ExtensionBlocks;
	 ep < (*ExtensionBlocks + *ExtensionBlockCount); 
	 ep++)
        (void)free((char *)ep->Bytes);
    (void)free((char *)*ExtensionBlocks);
    *ExtensionBlocks = NULL;
    *ExtensionBlockCount = 0;
}

/******************************************************************************
 Image block allocation functions                          
******************************************************************************/

/* Private Function:
 * Frees the last image in the GifFile->SavedImages array
 */
void
FreeLastSavedImage(GifFileType *GifFile)
{
    SavedImage *sp;
    
    if ((GifFile == NULL) || (GifFile->SavedImages == NULL))
        return;

    /* Remove one SavedImage from the GifFile */
    GifFile->ImageCount--;
    sp = &GifFile->SavedImages[GifFile->ImageCount];

    /* Deallocate its Colormap */
    if (sp->ImageDesc.ColorMap != NULL) {
        GifFreeMapObject(sp->ImageDesc.ColorMap);
        sp->ImageDesc.ColorMap = NULL;
    }

    /* Deallocate the image data */
    if (sp->RasterBits != NULL)
        free((char *)sp->RasterBits);

    /* Deallocate any extensions */
    GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks);

    /*** FIXME: We could realloc the GifFile->SavedImages structure but is
     * there a point to it? Saves some memory but we'd have to do it every
     * time.  If this is used in GifFreeSavedImages then it would be inefficient
     * (The whole array is going to be deallocated.)  If we just use it when
     * we want to free the last Image it's convenient to do it here.
     */
}

/*
 * Append an image block to the SavedImages array  
 */
SavedImage *
GifMakeSavedImage(GifFileType *GifFile, const SavedImage *CopyFrom)
{
    if (GifFile->SavedImages == NULL)
        GifFile->SavedImages = (SavedImage *)malloc(sizeof(SavedImage));
    else
        GifFile->SavedImages = (SavedImage *)reallocarray(GifFile->SavedImages,
                               (GifFile->ImageCount + 1), sizeof(SavedImage));

    if (GifFile->SavedImages == NULL)
        return ((SavedImage *)NULL);
    else {
        SavedImage *sp = &GifFile->SavedImages[GifFile->ImageCount++];
        memset((char *)sp, '\0', sizeof(SavedImage));

        if (CopyFrom != NULL) {
            memcpy((char *)sp, CopyFrom, sizeof(SavedImage));

            /* 
             * Make our own allocated copies of the heap fields in the
             * copied record.  This guards against potential aliasing
             * problems.
             */

            /* first, the local color map */
            if (sp->ImageDesc.ColorMap != NULL) {
                sp->ImageDesc.ColorMap = GifMakeMapObject(
                                         CopyFrom->ImageDesc.ColorMap->ColorCount,
                                         CopyFrom->ImageDesc.ColorMap->Colors);
                if (sp->ImageDesc.ColorMap == NULL) {
                    FreeLastSavedImage(GifFile);
                    return (SavedImage *)(NULL);
                }
            }

            /* next, the raster */
            sp->RasterBits = (unsigned char *)reallocarray(NULL,
                                                  (CopyFrom->ImageDesc.Height *
                                                  CopyFrom->ImageDesc.Width),
						  sizeof(GifPixelType));
            if (sp->RasterBits == NULL) {
                FreeLastSavedImage(GifFile);
                return (SavedImage *)(NULL);
            }
            memcpy(sp->RasterBits, CopyFrom->RasterBits,
                   sizeof(GifPixelType) * CopyFrom->ImageDesc.Height *
                   CopyFrom->ImageDesc.Width);

            /* finally, the extension blocks */
            if (sp->ExtensionBlocks != NULL) {
                sp->ExtensionBlocks = (ExtensionBlock *)reallocarray(NULL,
                                      CopyFrom->ExtensionBlockCount,
				      sizeof(ExtensionBlock));
                if (sp->ExtensionBlocks == NULL) {
                    FreeLastSavedImage(GifFile);
                    return (SavedImage *)(NULL);
                }
                memcpy(sp->ExtensionBlocks, CopyFrom->ExtensionBlocks,
                       sizeof(ExtensionBlock) * CopyFrom->ExtensionBlockCount);
            }
        }

        return (sp);
    }
}

void
GifFreeSavedImages(GifFileType *GifFile)
{
    SavedImage *sp;

    if ((GifFile == NULL) || (GifFile->SavedImages == NULL)) {
        return;
    }
    for (sp = GifFile->SavedImages;
         sp < GifFile->SavedImages + GifFile->ImageCount; sp++) {
        if (sp->ImageDesc.ColorMap != NULL) {
            GifFreeMapObject(sp->ImageDesc.ColorMap);
            sp->ImageDesc.ColorMap = NULL;
        }

        if (sp->RasterBits != NULL)
            free((char *)sp->RasterBits);
	
	GifFreeExtensions(&sp->ExtensionBlockCount, &sp->ExtensionBlocks);
    }
    free((char *)GifFile->SavedImages);
    GifFile->SavedImages = NULL;
}

/* end */

/*	$OpenBSD: reallocarray.c,v 1.1 2014/05/08 21:43:49 deraadt Exp $	*/
/*
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb > 0 && SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(optr, size * nmemb);
}

#endif //fl_anim_gif_private_H
