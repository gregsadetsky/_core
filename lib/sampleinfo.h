// Copyright 2023 Zack Scholl.
//
// Author: Zack Scholl (zack.scholl@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#ifndef SAMPLEINFO_H
#define SAMPLEINFO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct SampleInfo {
  uint32_t size;
  uint32_t bpm : 9;           // 0-511
  uint32_t play_mode : 3;     // 0-7
  uint32_t one_shot : 1;      // 0-1 (off/on)
  uint32_t tempo_match : 1;   // 0-1 (off/on)
  uint32_t oversampling : 1;  // 0-1 (1x or 2x)
  uint32_t num_channels : 1;  // 0-1 (mono or stereo)
  uint32_t version : 7;
  uint32_t reserved : 9;
  // slice info
  uint16_t splice_trigger : 15;
  uint16_t splice_variable : 1;  // 0-1 (off/on)
  uint8_t slice_num;             // 0-255
  uint8_t slice_current;         // 0-255
  // slice structs
  int32_t *slice_start;
  int32_t *slice_stop;
  int8_t *slice_type;
} SampleInfo;

typedef struct SampleInfoPack {
  uint32_t size;
  uint32_t
      flags;  // Holds bpm, slice_num, slice_current, play_mode, one_shot,
              // tempo_match, oversampling, num_channels, version and reserved
  uint16_t splice_info;  // Holds splice_trigger and splice_variable
  uint8_t slice_num;     // 0-255
} SampleInfoPack;

#define SAMPLEINFOPACK_SIZE (4 + 4 + 2 + 1)

void SampleInfo_free(SampleInfo *si) {
  if (si != NULL) {
    free(si->slice_start);
    free(si->slice_stop);
    free(si->slice_type);
  }
  free(si);
}

#ifndef NOSDCARD
// sdcard version
#endif

#ifdef NOSDCARD
// disk version (for testing)
SampleInfo *SampleInfo_readFromDisk() {
  FILE *file = fopen("sampleinfo.bin", "rb");
  if (file == NULL) {
    perror("Error opening file");
    return NULL;
  }

  SampleInfoPack *sip = (SampleInfoPack *)malloc(SAMPLEINFOPACK_SIZE);
  if (sip == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return NULL;
  }

  if (fread(sip, SAMPLEINFOPACK_SIZE, 1, file) != 1) {
    perror("Error reading struct from file");
    fclose(file);
    free(sip);
    return NULL;
  }

  // create SampleInfo from SampleInfoPack
  SampleInfo *si = (SampleInfo *)malloc(sizeof(SampleInfo));
  if (si == NULL) {
    perror("Error allocating memory");
    fclose(file);
    free(sip);
    return NULL;
  }

  si->size = sip->size;
  si->bpm = sip->flags & 0x1FF;
  si->play_mode = (sip->flags >> 9) & 0x7;
  si->one_shot = (sip->flags >> 12) & 0x1;
  si->tempo_match = (sip->flags >> 13) & 0x1;
  si->oversampling = (sip->flags >> 14) & 0x1;
  si->num_channels = (sip->flags >> 15) & 0x1;
  si->version = (sip->flags >> 16) & 0x7F;
  si->reserved = (sip->flags >> 23) & 0x1FF;
  si->splice_trigger = sip->splice_info & 0x7FFF;
  si->splice_variable = (sip->splice_info >> 15) & 0x1;
  si->slice_num = sip->slice_num;
  si->slice_current = 0;

  printf("size: %d\n", si->size);
  printf("bpm: %d\n", si->bpm);
  printf("play_mode: %d\n", si->play_mode);
  printf("one_shot: %d\n", si->one_shot);
  printf("tempo_match: %d\n", si->tempo_match);
  printf("oversampling: %d\n", si->oversampling);
  printf("num_channels: %d\n", si->num_channels);
  printf("version: %d\n", si->version);
  printf("reserved: %d\n", si->reserved);
  printf("splice_trigger: %d\n", si->splice_trigger);
  printf("splice_variable: %d\n", si->splice_variable);
  printf("slice_num: %d\n", si->slice_num);

  // read from the file to collect the arrays
  // read in the slice_start
  si->slice_start = malloc(sizeof(int32_t) * si->slice_num);
  if (si->slice_start == NULL) {
    perror("Error allocating memory for array");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }
  if (fread(si->slice_start, sizeof(int32_t), si->slice_num, file) !=
      si->slice_num) {
    perror("Error reading slice_start from file");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }
  for (int i = 0; i < si->slice_num; i++) {
    printf("slice_start[%d]: %d\n", i, si->slice_start[i]);
  }

  // read in the slice_stop
  si->slice_stop = malloc(sizeof(int32_t) * si->slice_num);
  if (si->slice_stop == NULL) {
    perror("Error allocating memory for array");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }
  if (fread(si->slice_stop, sizeof(int32_t), si->slice_num, file) !=
      si->slice_num) {
    perror("Error reading slice_stop from file");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }

  // read in the slice_type
  si->slice_type = malloc(sizeof(int8_t) * si->slice_num);
  if (si->slice_type == NULL) {
    perror("Error allocating memory for array");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }
  if (fread(si->slice_type, sizeof(int8_t), si->slice_num, file) !=
      si->slice_num) {
    perror("Error reading slice_type from file");
    fclose(file);
    free(sip);
    SampleInfo_free(si);
    return NULL;
  }

  fclose(file);

  // free the SampleInfoPack
  free(sip);

  // initialize variables
  if (si->bpm < 30) {
    si->bpm = 30;
  } else if (si->bpm > 300) {
    si->bpm = 300;
  }
  return si;
}
#endif

#endif