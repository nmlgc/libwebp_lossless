// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// WebPPicture utils for colorspace conversion
//
// Author: Skal (pascal.massimino@gmail.com)

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "src/enc/vp8i_enc.h"
#include "src/utils/random_utils.h"
#include "src/utils/utils.h"
#include "src/dsp/dsp.h"
#include "src/dsp/lossless.h"
#include "src/dsp/cpu.h"

#if defined(WEBP_USE_THREAD) && !defined(_WIN32)
#include <pthread.h>
#endif

#ifdef WORDS_BIGENDIAN
// uint32_t 0xff000000 is 0xff,00,00,00 in memory
#define CHANNEL_OFFSET(i) (i)
#else
// uint32_t 0xff000000 is 0x00,00,00,ff in memory
#define CHANNEL_OFFSET(i) (3-(i))
#endif

#define ALPHA_OFFSET CHANNEL_OFFSET(0)

//------------------------------------------------------------------------------
// Detection of non-trivial transparency

// Returns true if alpha[] has non-0xff values.
static int CheckNonOpaque(const uint8_t* alpha, int width, int height,
                          int x_step, int y_step) {
  if (alpha == NULL) return 0;
  WebPInitAlphaProcessing();
  if (x_step == 1) {
    for (; height-- > 0; alpha += y_step) {
      if (WebPHasAlpha8b(alpha, width)) return 1;
    }
  } else {
    for (; height-- > 0; alpha += y_step) {
      if (WebPHasAlpha32b(alpha, width)) return 1;
    }
  }
  return 0;
}

// Checking for the presence of non-opaque alpha.
int WebPPictureHasTransparency(const WebPPicture* picture) {
  if (picture == NULL) return 0;
  if (picture->use_argb) {
    if (picture->argb != NULL) {
      return CheckNonOpaque((const uint8_t*)picture->argb + ALPHA_OFFSET,
                            picture->width, picture->height,
                            4, picture->argb_stride * sizeof(*picture->argb));
    }
    return 0;
  }
  return CheckNonOpaque(picture->a, picture->width, picture->height,
                        1, picture->a_stride);
}

//------------------------------------------------------------------------------
// automatic import / conversion

static int Import(WebPPicture* const picture,
                  const uint8_t* rgb, int rgb_stride,
                  int step, int swap_rb, int import_alpha) {
  int y;
  // swap_rb -> b,g,r,a , !swap_rb -> r,g,b,a
  const uint8_t* r_ptr = rgb + (swap_rb ? 2 : 0);
  const uint8_t* g_ptr = rgb + 1;
  const uint8_t* b_ptr = rgb + (swap_rb ? 0 : 2);
  const int width = picture->width;
  const int height = picture->height;

  if (abs(rgb_stride) < (import_alpha ? 4 : 3) * width) return 0;

  if (!picture->use_argb) {
    return 0;
  }
  if (!WebPPictureAlloc(picture)) return 0;

  VP8LDspInit();
  WebPInitAlphaProcessing();

  if (import_alpha) {
    // dst[] byte order is {a,r,g,b} for big-endian, {b,g,r,a} for little endian
    uint32_t* dst = picture->argb;
    const int do_copy = (ALPHA_OFFSET == 3) && swap_rb;
    assert(step == 4);
    if (do_copy) {
      for (y = 0; y < height; ++y) {
        memcpy(dst, rgb, width * 4);
        rgb += rgb_stride;
        dst += picture->argb_stride;
      }
    } else {
      for (y = 0; y < height; ++y) {
#ifdef WORDS_BIGENDIAN
        // BGRA or RGBA input order.
        const uint8_t* a_ptr = rgb + 3;
        WebPPackARGB(a_ptr, r_ptr, g_ptr, b_ptr, width, dst);
        r_ptr += rgb_stride;
        g_ptr += rgb_stride;
        b_ptr += rgb_stride;
#else
        // RGBA input order. Need to swap R and B.
        VP8LConvertBGRAToRGBA((const uint32_t*)rgb, width, (uint8_t*)dst);
#endif
        rgb += rgb_stride;
        dst += picture->argb_stride;
      }
    }
  } else {
    uint32_t* dst = picture->argb;
    assert(step >= 3);
    for (y = 0; y < height; ++y) {
      WebPPackRGB(r_ptr, g_ptr, b_ptr, width, step, dst);
      r_ptr += rgb_stride;
      g_ptr += rgb_stride;
      b_ptr += rgb_stride;
      dst += picture->argb_stride;
    }
  }
  return 1;
}

// Public API

#if !defined(WEBP_REDUCE_CSP)

int WebPPictureImportBGR(WebPPicture* picture,
                         const uint8_t* bgr, int bgr_stride) {
  return (picture != NULL && bgr != NULL)
             ? Import(picture, bgr, bgr_stride, 3, 1, 0)
             : 0;
}

int WebPPictureImportBGRA(WebPPicture* picture,
                          const uint8_t* bgra, int bgra_stride) {
  return (picture != NULL && bgra != NULL)
             ? Import(picture, bgra, bgra_stride, 4, 1, 1)
             : 0;
}


int WebPPictureImportBGRX(WebPPicture* picture,
                          const uint8_t* bgrx, int bgrx_stride) {
  return (picture != NULL && bgrx != NULL)
             ? Import(picture, bgrx, bgrx_stride, 4, 1, 0)
             : 0;
}

#endif   // WEBP_REDUCE_CSP

int WebPPictureImportRGB(WebPPicture* picture,
                         const uint8_t* rgb, int rgb_stride) {
  return (picture != NULL && rgb != NULL)
             ? Import(picture, rgb, rgb_stride, 3, 0, 0)
             : 0;
}

int WebPPictureImportRGBA(WebPPicture* picture,
                          const uint8_t* rgba, int rgba_stride) {
  return (picture != NULL && rgba != NULL)
             ? Import(picture, rgba, rgba_stride, 4, 0, 1)
             : 0;
}

int WebPPictureImportRGBX(WebPPicture* picture,
                          const uint8_t* rgbx, int rgbx_stride) {
  return (picture != NULL && rgbx != NULL)
             ? Import(picture, rgbx, rgbx_stride, 4, 0, 0)
             : 0;
}

//------------------------------------------------------------------------------
