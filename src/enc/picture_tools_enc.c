// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// WebPPicture tools: alpha handling, etc.
//
// Author: Skal (pascal.massimino@gmail.com)

#include <assert.h>

#include "src/enc/vp8i_enc.h"
#include "src/dsp/yuv.h"

//------------------------------------------------------------------------------
// Helper: clean up fully transparent area to help compressibility.

void WebPReplaceTransparentPixels(WebPPicture* const pic, uint32_t color) {
  if (pic != NULL && pic->use_argb) {
    int y = pic->height;
    uint32_t* argb = pic->argb;
    color &= 0xffffffu;   // force alpha=0
    WebPInitAlphaProcessing();
    while (y-- > 0) {
      WebPAlphaReplace(argb, pic->width, color);
      argb += pic->argb_stride;
    }
  }
}

//------------------------------------------------------------------------------
// Blend color and remove transparency info

#define BLEND(V0, V1, ALPHA) \
    ((((V0) * (255 - (ALPHA)) + (V1) * (ALPHA)) * 0x101 + 256) >> 16)
#define BLEND_10BIT(V0, V1, ALPHA) \
    ((((V0) * (1020 - (ALPHA)) + (V1) * (ALPHA)) * 0x101 + 1024) >> 18)

static WEBP_INLINE uint32_t MakeARGB32(int r, int g, int b) {
  return (0xff000000u | (r << 16) | (g << 8) | b);
}

void WebPBlendAlpha(WebPPicture* picture, uint32_t background_rgb) {
  const int red = (background_rgb >> 16) & 0xff;
  const int green = (background_rgb >> 8) & 0xff;
  const int blue = (background_rgb >> 0) & 0xff;
  int x, y;
  if (picture == NULL) return;
  if (!picture->use_argb) {
    // omit last pixel during u/v loop
    const int uv_width = (picture->width >> 1);
    const int Y0 = VP8RGBToY(red, green, blue, YUV_HALF);
    // VP8RGBToU/V expects the u/v values summed over four pixels
    const int U0 = VP8RGBToU(4 * red, 4 * green, 4 * blue, 4 * YUV_HALF);
    const int V0 = VP8RGBToV(4 * red, 4 * green, 4 * blue, 4 * YUV_HALF);
    const int has_alpha = picture->colorspace & WEBP_CSP_ALPHA_BIT;
    uint8_t* y_ptr = picture->y;
    uint8_t* u_ptr = picture->u;
    uint8_t* v_ptr = picture->v;
    uint8_t* a_ptr = picture->a;
    if (!has_alpha || a_ptr == NULL) return;    // nothing to do
    for (y = 0; y < picture->height; ++y) {
      // Luma blending
      for (x = 0; x < picture->width; ++x) {
        const uint8_t alpha = a_ptr[x];
        if (alpha < 0xff) {
          y_ptr[x] = BLEND(Y0, y_ptr[x], alpha);
        }
      }
      // Chroma blending every even line
      if ((y & 1) == 0) {
        uint8_t* const a_ptr2 =
            (y + 1 == picture->height) ? a_ptr : a_ptr + picture->a_stride;
        for (x = 0; x < uv_width; ++x) {
          // Average four alpha values into a single blending weight.
          // TODO(skal): might lead to visible contouring. Can we do better?
          const uint32_t alpha =
              a_ptr[2 * x + 0] + a_ptr[2 * x + 1] +
              a_ptr2[2 * x + 0] + a_ptr2[2 * x + 1];
          u_ptr[x] = BLEND_10BIT(U0, u_ptr[x], alpha);
          v_ptr[x] = BLEND_10BIT(V0, v_ptr[x], alpha);
        }
        if (picture->width & 1) {  // rightmost pixel
          const uint32_t alpha = 2 * (a_ptr[2 * x + 0] + a_ptr2[2 * x + 0]);
          u_ptr[x] = BLEND_10BIT(U0, u_ptr[x], alpha);
          v_ptr[x] = BLEND_10BIT(V0, v_ptr[x], alpha);
        }
      } else {
        u_ptr += picture->uv_stride;
        v_ptr += picture->uv_stride;
      }
      memset(a_ptr, 0xff, picture->width);  // reset alpha value to opaque
      a_ptr += picture->a_stride;
      y_ptr += picture->y_stride;
    }
  } else {
    uint32_t* argb = picture->argb;
    const uint32_t background = MakeARGB32(red, green, blue);
    for (y = 0; y < picture->height; ++y) {
      for (x = 0; x < picture->width; ++x) {
        const int alpha = (argb[x] >> 24) & 0xff;
        if (alpha != 0xff) {
          if (alpha > 0) {
            int r = (argb[x] >> 16) & 0xff;
            int g = (argb[x] >>  8) & 0xff;
            int b = (argb[x] >>  0) & 0xff;
            r = BLEND(red, r, alpha);
            g = BLEND(green, g, alpha);
            b = BLEND(blue, b, alpha);
            argb[x] = MakeARGB32(r, g, b);
          } else {
            argb[x] = background;
          }
        }
      }
      argb += picture->argb_stride;
    }
  }
}

#undef BLEND
#undef BLEND_10BIT

//------------------------------------------------------------------------------
