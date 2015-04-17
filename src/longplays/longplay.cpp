#include "longplay.h"

#include "hardware.h"
#include "../save_state.h"

#include <assert.h>
#include <vector>

void CAPTURE_VideoEvent(bool pressed);

static const Bit32u CURRENT_VERSION = 1;

struct CaptureInfo
{
	std::string file_name;
	Bit32u frame_count = 0;
};

typedef std::vector<CaptureInfo> CaptureList;

class LongPlaySaveStateComponent : public SaveState::Component
{
public:

	void getBytes(std::ostream& stream) override;
	void setBytes(std::istream& stream) override;

	CaptureList previous_captures;
	CaptureInfo current_capture;
};

static LongPlaySaveStateComponent save_state_component;

static void readCapture(std::istream &stream, CaptureInfo &capture)
{
	readString(stream, capture.file_name);
	readPOD(stream, capture.frame_count);
}

static void writeCapture(std::ostream &stream, const CaptureInfo &capture)
{
	writeString(stream, capture.file_name);
	writePOD(stream, capture.frame_count);
}

void LONGPLAY_Init(Section *section)
{
	// Register with the save state.
	SaveState &save_state = SaveState::instance();
	save_state.registerComponent("Longplay", save_state_component);
}

void LONGPLAY_SetCaptureFile(const char *file_name)
{
	save_state_component.current_capture.file_name = file_name;
	save_state_component.current_capture.frame_count = 0;
}

void LONGPLAY_SetFrameCount(Bitu frame_count)
{
	save_state_component.current_capture.frame_count = frame_count;
}

void LongPlaySaveStateComponent::getBytes(std::ostream& stream)
{
	// Save version and capture state.
	writePOD(stream, CURRENT_VERSION);
	writePOD(stream, static_cast<Bit32u>(CaptureState));

	// Is there a capture running?
	if (CaptureState & CAPTURE_VIDEO)
	{
		writePOD(stream, static_cast<Bit32u>(previous_captures.size() + 1));
	}
	else
	{
		writePOD(stream, static_cast<Bit32u>(previous_captures.size()));
	}

	// Write previous captures.
	for (CaptureList::const_iterator it = previous_captures.begin(); it != previous_captures.end(); ++it)
	{
		writeCapture(stream, *it);
	}

	// Is there a capture running?
	if (CaptureState & CAPTURE_VIDEO)
	{
		writeCapture(stream, current_capture);
	}

	// TODO Write out log/script.
}

void LongPlaySaveStateComponent::setBytes(std::istream& stream)
{
	// Read version.
	Bit32u stream_version = 0;
	readPOD(stream, stream_version);

	// TODO Check version and migrate.

	// Read header.
	Bit32u stream_capture_state = 0;
	Bit32u stream_capture_count = 0;
	readPOD(stream, stream_capture_state);
	readPOD(stream, stream_capture_count);

	// Read previous captures.
	previous_captures.resize(stream_capture_count);
	for (CaptureList::iterator it = previous_captures.begin(); it != previous_captures.end(); ++it)
	{
		readCapture(stream, *it);
	}

	// Close current capture.
	if (CaptureState & CAPTURE_VIDEO) {
		CAPTURE_VideoEvent(true);
		assert((CaptureState & CAPTURE_VIDEO) == 0);
	}

	// Was there a capture running when state was saved?
	if (stream_capture_state & CAPTURE_VIDEO)
	{
		// Start a new capture.
		CAPTURE_VideoEvent(true);
		assert(CaptureState & CAPTURE_VIDEO);
	}
}
