// Copyright 2011 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
//   WebP encoder: internal header.
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef WEBP_ENC_VP8I_ENC_H_
#define WEBP_ENC_VP8I_ENC_H_

#include <string.h>     // for memcpy()
#include "src/dsp/dsp.h"
#include "src/utils/thread_utils.h"
#include "src/webp/encode.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Various defines and enums

// version numbers
#define ENC_MAJ_VERSION 1
#define ENC_MIN_VERSION 5
#define ENC_REV_VERSION 0

//------------------------------------------------------------------------------
// internal functions. Not public.

  // in webpenc.c
// Assign an error code to a picture. Return false for convenience.
int WebPEncodingSetError(const WebPPicture* const pic, WebPEncodingError error);
int WebPReportProgress(const WebPPicture* const pic,
                       int percent, int* const percent_store);

  // misc utils for picture_*.c:

// Returns true if 'picture' is non-NULL and dimensions/colorspace are within
// their valid ranges. If returning false, the 'error_code' in 'picture' is
// updated.
int WebPValidatePicture(const WebPPicture* const picture);

// Remove reference to the ARGB/YUVA buffer (doesn't free anything).
void WebPPictureResetBuffers(WebPPicture* const picture);

// Allocates ARGB buffer according to set width/height (previous one is
// always free'd). Preserves the YUV(A) buffer. Returns false in case of error
// (invalid param, out-of-memory).
int WebPPictureAllocARGB(WebPPicture* const picture);

// Replace samples that are fully transparent by 'color' to help compressibility
// (no guarantee, though). Assumes pic->use_argb is true.
void WebPReplaceTransparentPixels(WebPPicture* const pic, uint32_t color);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}    // extern "C"
#endif

#endif  // WEBP_ENC_VP8I_ENC_H_
