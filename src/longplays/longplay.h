#pragma once

#include "config.h"

class Section;

void LONGPLAY_Init(Section *section);
void LONGPLAY_SetCaptureFile(const char *file_name);
void LONGPLAY_SetFrameCount(Bitu frame_count);
