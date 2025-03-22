// Copyright 2011 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Coding tools configuration
//
// Author: Skal (pascal.massimino@gmail.com)

#ifdef HAVE_CONFIG_H
#include "src/webp/config.h"
#endif

#include "src/webp/encode.h"

//------------------------------------------------------------------------------
// WebPConfig
//------------------------------------------------------------------------------

int WebPConfigInitInternal(WebPConfig* config,
                           WebPPreset preset, float quality, int version) {
  if (WEBP_ABI_IS_INCOMPATIBLE(version, WEBP_ENCODER_ABI_VERSION)) {
    return 0;   // caller/system version mismatch!
  }
  if (config == NULL) return 0;

  config->quality = quality;
  config->lossy_01 = 0;
  config->lossy_02 = 0.;
  config->method = 4;
  config->lossy_04 = 50;
  config->lossy_05 = 60;   // mid-filtering
  config->lossy_06 = 0;
  config->lossy_07 = 1;        // default: strong (so U/V is filtered too)
  config->lossy_15 = 0;
  config->lossy_03 = 4;
  config->lossy_12 = 1;
  config->lossy_20 = 0;
  config->lossy_21 = 100;
  config->lossy_13 = 0;
  config->lossy_14 = 0;
  config->lossy_08 = 0;
  config->lossy_16 = 0;
  config->lossy_09 = 1;
  config->lossy_10 = 1;
  config->lossy_11 = 100;
  config->lossless = 0;
  config->exact = 0;
  config->image_hint = WEBP_HINT_DEFAULT;
  config->lossy_17 = 0;
  config->thread_level = 0;
  config->lossy_18 = 0;
  config->near_lossless = 100;
  config->lossy_19 = 0;

  // TODO(skal): tune.
  switch (preset) {
    case WEBP_PRESET_PICTURE:
      config->lossy_04 = 80;
      config->lossy_06 = 4;
      config->lossy_05 = 35;
      config->lossy_14 &= ~2;   // no dithering
      break;
    case WEBP_PRESET_PHOTO:
      config->lossy_04 = 80;
      config->lossy_06 = 3;
      config->lossy_05 = 30;
      config->lossy_14 |= 2;
      break;
    case WEBP_PRESET_DRAWING:
      config->lossy_04 = 25;
      config->lossy_06 = 6;
      config->lossy_05 = 10;
      break;
    case WEBP_PRESET_ICON:
      config->lossy_04 = 0;
      config->lossy_05 = 0;   // disable filtering to retain sharpness
      config->lossy_14 &= ~2;   // no dithering
      break;
    case WEBP_PRESET_TEXT:
      config->lossy_04 = 0;
      config->lossy_05 = 0;   // disable filtering to retain sharpness
      config->lossy_14 &= ~2;   // no dithering
      config->lossy_03 = 2;
      break;
    case WEBP_PRESET_DEFAULT:
    default:
      break;
  }
  return WebPValidateConfig(config);
}

int WebPValidateConfig(const WebPConfig* config) {
  if (config == NULL) return 0;
  if (config->quality < 0 || config->quality > 100) return 0;
  if (config->lossy_01 < 0) return 0;
  if (config->lossy_02 < 0) return 0;
  if (config->method < 0 || config->method > 6) return 0;
  if (config->lossy_03 < 1 || config->lossy_03 > 4) return 0;
  if (config->lossy_04 < 0 || config->lossy_04 > 100) return 0;
  if (config->lossy_05 < 0 || config->lossy_05 > 100) return 0;
  if (config->lossy_06 < 0 || config->lossy_06 > 7) return 0;
  if (config->lossy_07 < 0 || config->lossy_07 > 1) return 0;
  if (config->lossy_08 < 0 || config->lossy_08 > 1) return 0;
  if (config->lossy_12 < 1 || config->lossy_12 > 10) return 0;
  if (config->lossy_20 < 0 || config->lossy_21 > 100 || config->lossy_20 > config->lossy_21) {
    return 0;
  }
  if (config->lossy_13 < 0 || config->lossy_13 > 1) return 0;
  if (config->lossy_14 < 0 || config->lossy_14 > 7) return 0;
  if (config->lossy_15 < 0 || config->lossy_15 > 3) return 0;
  if (config->lossy_16 < 0 || config->lossy_16 > 100) return 0;
  if (config->lossy_09 < 0) return 0;
  if (config->lossy_10 < 0) return 0;
  if (config->lossy_11 < 0 || config->lossy_11 > 100) return 0;
  if (config->lossless < 0 || config->lossless > 1) return 0;
  if (config->near_lossless < 0 || config->near_lossless > 100) return 0;
  if (config->image_hint >= WEBP_HINT_LAST) return 0;
  if (config->lossy_17 < 0 || config->lossy_17 > 1) return 0;
  if (config->thread_level < 0 || config->thread_level > 1) return 0;
  if (config->lossy_18 < 0 || config->lossy_18 > 1) return 0;
  if (config->exact < 0 || config->exact > 1) return 0;
  if (config->lossy_19 < 0 || config->lossy_19 > 1) return 0;

  return 1;
}

//------------------------------------------------------------------------------

#define MAX_LEVEL 9

// Mapping between -z level and -m / -q parameter settings.
static const struct {
  uint8_t method_;
  uint8_t quality_;
} kLosslessPresets[MAX_LEVEL + 1] = {
  { 0,  0 }, { 1, 20 }, { 2, 25 }, { 3, 30 }, { 3, 50 },
  { 4, 50 }, { 4, 75 }, { 4, 90 }, { 5, 90 }, { 6, 100 }
};

int WebPConfigLosslessPreset(WebPConfig* config, int level) {
  if (config == NULL || level < 0 || level > MAX_LEVEL) return 0;
  config->lossless = 1;
  config->method = kLosslessPresets[level].method_;
  config->quality = kLosslessPresets[level].quality_;
  return 1;
}

//------------------------------------------------------------------------------
