#include "longplay.h"

#include "hardware.h"
#include "../save_state.h"

#include <assert.h>

void CAPTURE_VideoEvent(bool pressed);

static const Bit32u CURRENT_VERSION = 1;

class LongPlaySaveStateComponent : public SaveState::Component
{
public:

	void getBytes(std::ostream& stream) override;
	void setBytes(std::istream& stream) override;
};

static std::string file_name;
static Bit32u frame_count;
static LongPlaySaveStateComponent save_state_component;

void LONGPLAY_Init(Section *section)
{
	// Register with the save state.
	SaveState &save_state = SaveState::instance();
	save_state.registerComponent("Longplay", save_state_component);
}

void LONGPLAY_SetCaptureFile(const char *file_name)
{
	::file_name = file_name;
}

void LONGPLAY_SetFrameCount(Bitu frame_count)
{
	::frame_count = frame_count;
}

void LongPlaySaveStateComponent::getBytes(std::ostream& stream)
{
	// Save version and capture state.
	writePOD(stream, CURRENT_VERSION);
	writePOD(stream, static_cast<Bit32u>(CaptureState));

	// Is there a capture running?
	if (CaptureState & CAPTURE_VIDEO)
	{
		writeString(stream, file_name);
		writePOD(stream, frame_count);
	}

	// TODO Write out log/script.
}

void LongPlaySaveStateComponent::setBytes(std::istream& stream)
{
	// Read version.
	Bit32u stream_version = 0;
	readPOD(stream, stream_version);

	// TODO Check version and migrate.

	// Read capture state.
	Bit32u stream_capture_state = 0;
	readPOD(stream, stream_capture_state);

	// Was there a capture running when state was saved?
	if (stream_capture_state & CAPTURE_VIDEO)
	{
		// Read capture info from stream.
		std::string stream_file_name;
		Bit32u stream_frame_count = 0;
		readString(stream, stream_file_name);
		readPOD(stream, stream_frame_count);

		// Close current capture.
		if (CaptureState & CAPTURE_VIDEO) {
			CAPTURE_VideoEvent(true);
			assert((CaptureState & CAPTURE_VIDEO) == 0);
		}

		// Start a new capture.
		CAPTURE_VideoEvent(true);
		assert(CaptureState & CAPTURE_VIDEO);
	}
	else
	{
		// TODO Close current capture?
	}
}
