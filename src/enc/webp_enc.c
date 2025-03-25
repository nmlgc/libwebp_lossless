// Copyright 2011 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// WebP encoder: main entry point
//
// Author: Skal (pascal.massimino@gmail.com)

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "src/enc/vp8i_enc.h"
#include "src/enc/vp8li_enc.h"
#include "src/utils/utils.h"

//------------------------------------------------------------------------------

int WebPGetEncoderVersion(void) {
  return (ENC_MAJ_VERSION << 16) | (ENC_MIN_VERSION << 8) | ENC_REV_VERSION;
}

//------------------------------------------------------------------------------

int WebPEncodingSetError(const WebPPicture* const pic,
                         WebPEncodingError error) {
  assert((int)error < VP8_ENC_ERROR_LAST);
  assert((int)error >= VP8_ENC_OK);
  // The oldest error reported takes precedence over the new one.
  if (pic->error_code == VP8_ENC_OK) {
    ((WebPPicture*)pic)->error_code = error;
  }
  return 0;
}

int WebPReportProgress(const WebPPicture* const pic,
                       int percent, int* const percent_store) {
  if (percent_store != NULL && percent != *percent_store) {
    *percent_store = percent;
    if (pic->progress_hook && !pic->progress_hook(percent, pic)) {
      // user abort requested
      return WebPEncodingSetError(pic, VP8_ENC_ERROR_USER_ABORT);
    }
  }
  return 1;  // ok
}
//------------------------------------------------------------------------------

int WebPEncode(const WebPConfig* config, WebPPicture* pic) {
  int ok = 0;
  if (pic == NULL) return 0;

  pic->error_code = VP8_ENC_OK;  // all ok so far
  if (config == NULL) {  // bad params
    return WebPEncodingSetError(pic, VP8_ENC_ERROR_NULL_PARAMETER);
  }
  if (!WebPValidateConfig(config)) {
    return WebPEncodingSetError(pic, VP8_ENC_ERROR_INVALID_CONFIGURATION);
  }
  if (!WebPValidatePicture(pic)) return 0;
  if (pic->width > WEBP_MAX_DIMENSION || pic->height > WEBP_MAX_DIMENSION) {
    return WebPEncodingSetError(pic, VP8_ENC_ERROR_BAD_DIMENSION);
  }

  if (pic->stats != NULL) memset(pic->stats, 0, sizeof(*pic->stats));

  if (!config->lossless) {
    return 0;
  } else {
    // Make sure we have ARGB samples.
    if (pic->argb == NULL) {
      return 0;
    }

    if (!config->exact) {
      WebPReplaceTransparentPixels(pic, 0x000000);
    }

    ok = VP8LEncodeImage(config, pic);  // Sets pic->error in case of problem.
  }

  return ok;
}
