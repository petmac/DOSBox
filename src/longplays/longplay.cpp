#include "longplay.h"

#include "hardware.h"
#include "../save_state.h"

#include <assert.h>
#include <time.h>
#include <vector>

void CAPTURE_VideoEvent(bool pressed);

static const bool RESIZE = false;
static const Bit32u CURRENT_VERSION = 1;

struct CaptureInfo
{
	std::string file_name;
	Bit32u width = 0;
	Bit32u height = 0;
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

private:

	void writeSave(std::ostream &stream) const;
	void writeScript() const;
};

static LongPlaySaveStateComponent save_state_component;

static void readCapture(std::istream &stream, CaptureInfo &capture)
{
	readString(stream, capture.file_name);
	readPOD(stream, capture.width);
	readPOD(stream, capture.height);
	readPOD(stream, capture.frame_count);
}

static void writeCapture(std::ostream &stream, const CaptureInfo &capture)
{
	writeString(stream, capture.file_name);
	writePOD(stream, capture.width);
	writePOD(stream, capture.height);
	writePOD(stream, capture.frame_count);
}

static void scriptCapture(FILE *script, const CaptureInfo &capture)
{
	fprintf(
		script,
		"%s %u %u %u\n",
		capture.file_name.c_str(),
		static_cast<unsigned int>(capture.width),
		static_cast<unsigned int>(capture.height),
		static_cast<unsigned int>(capture.frame_count));
}

void LONGPLAY_Init(Section *section)
{
	// Register with the save state.
	SaveState &save_state = SaveState::instance();
	save_state.registerComponent("Longplay", save_state_component);

	// Not capturing?
	if ((CaptureState & CAPTURE_VIDEO) == 0)
	{
		// Start capturing.
		CAPTURE_VideoEvent(true);
		assert(CaptureState & CAPTURE_VIDEO);
	}
}

void LONGPLAY_SetCaptureFile(const char *file_name)
{
	// Have an old capture?
	if (save_state_component.current_capture.frame_count > 0)
	{
		// Remember it.
		save_state_component.previous_captures.push_back(save_state_component.current_capture);
	}

	// Start the new capture.
	save_state_component.current_capture = CaptureInfo();
	save_state_component.current_capture.file_name = file_name;
}

void LONGPLAY_BeginCapture(Bitu width, Bitu height)
{
	save_state_component.current_capture.width = width;
	save_state_component.current_capture.height = height;
	save_state_component.current_capture.frame_count = 0;
}

void LONGPLAY_SetFrameCount(Bitu frame_count)
{
	save_state_component.current_capture.frame_count = frame_count;
}

void LongPlaySaveStateComponent::getBytes(std::ostream& stream)
{
	writeSave(stream);
	writeScript();
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
	if (CaptureState & CAPTURE_VIDEO)
	{
		// Is there not yet an open capture file?
		if (current_capture.file_name.empty())
		{
			// Capture has not actually started yet. Abort it before the file is created.
			CaptureState &= ~CAPTURE_VIDEO;
		}
		else
		{
			// Capture has started, so shut it down cleanly.
			CAPTURE_VideoEvent(true);
		}
		assert((CaptureState & CAPTURE_VIDEO) == 0);
	}

	// Clear current capture.
	current_capture = CaptureInfo();

	// Was there a capture running when state was saved?
	if (stream_capture_state & CAPTURE_VIDEO)
	{
		// Start a new capture.
		CAPTURE_VideoEvent(true);
		assert(CaptureState & CAPTURE_VIDEO);
	}
}

void LongPlaySaveStateComponent::writeSave(std::ostream &stream) const
{
	// Save version and capture state.
	writePOD(stream, CURRENT_VERSION);
	writePOD(stream, static_cast<Bit32u>(CaptureState));

	// Is no capture file open?
	if (current_capture.file_name.empty())
	{
		writePOD(stream, static_cast<Bit32u>(previous_captures.size()));
	}
	else
	{
		writePOD(stream, static_cast<Bit32u>(previous_captures.size() + 1));
	}

	// Write previous captures.
	for (CaptureList::const_iterator it = previous_captures.begin(); it != previous_captures.end(); ++it)
	{
		writeCapture(stream, *it);
	}

	// Is there a capture running?
	if (!current_capture.file_name.empty())
	{
		writeCapture(stream, current_capture);
	}
}

void LongPlaySaveStateComponent::writeScript() const
{
	// Get the current time.
	time_t t = {};
	time(&t);
	const struct tm *const gmt = gmtime(&t);

	// Compute the path.
	char path[256] = {};
	sprintf(path, "longplay_%04d-%02d-%02d_%02d-%02d-%02d.txt",
		gmt->tm_year + 1900,
		gmt->tm_mon + 1,
		gmt->tm_mday,
		gmt->tm_hour,
		gmt->tm_min,
		gmt->tm_sec);

	// Open the script file.
	FILE *script = fopen(path, "w");
	if (script == NULL)
	{
		return;
	}

	// Write out previous captures.
	for (CaptureList::const_iterator it = previous_captures.begin(); it != previous_captures.end(); ++it)
	{
		scriptCapture(script, *it);
	}

	// Write out the current capture, if any.
	if (CaptureState & CAPTURE_VIDEO)
	{
		scriptCapture(script, current_capture);
	}

	// Close the script.
	fclose(script);
	script = NULL;
}
