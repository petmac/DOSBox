#pragma once

#include "config.h"

class Section;

extern const char *const LONGPLAY_SCALER;
extern const char *const LONGPLAY_SCALER_NONE;
extern const char *const LONGPLAY_SCALER_INTEGRAL;

void LONGPLAY_Init(Section *section);
void LONGPLAY_SetCaptureFile(const char *file_name);
void LONGPLAY_BeginCapture(Bitu width, Bitu height);
void LONGPLAY_SetFrameCount(Bitu frame_count);
