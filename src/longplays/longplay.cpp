#include "longplay.h"

#include "hardware.h"
#include "../save_state.h"

#include <assert.h>
#include <vector>

void CAPTURE_VideoEvent(bool pressed);

const char *const LONGPLAY_SCALER = "scaler";
const char *const LONGPLAY_SCALER_NONE = "none";
const char *const LONGPLAY_SCALER_INTEGRAL = "integral";

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

	const Section *section = NULL;
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

static void scriptAddBorders(FILE *avs, Bitu from_width, Bitu from_height, Bitu to_width, Bitu to_height)
{
	// Compute the borders to add.
	Bitu left_border = (to_width - from_width) / 2;
	Bitu top_border = (to_height - from_height) / 2;
	Bitu right_border = to_width - from_width - left_border;
	Bitu bottom_border = to_height - from_height - top_border;

	// Add the borders.
	fprintf(
		avs,
		".AddBorders(%u, %u, %u, %u)",
		static_cast<unsigned int>(left_border),
		static_cast<unsigned int>(top_border),
		static_cast<unsigned int>(right_border),
		static_cast<unsigned int>(bottom_border));
}

static void scriptResize(FILE *avs, Bitu to_width, Bitu to_height)
{
	fprintf(
		avs,
		".BicubicResize(%u, %u)",
		static_cast<unsigned int>(to_width),
		static_cast<unsigned int>(to_height));
}

static void scriptCapture(FILE *avs, const CaptureInfo &capture, Bitu largest_width, Bitu largest_height, const Section *section)
{
	// TODO Handle frame count == 0?
	fprintf(
		avs,
		"AviSource(\"%s\").AssumeFPS(70).Trim(0, %u)",
		capture.file_name.c_str(),
		static_cast<unsigned int>(capture.frame_count - 1));

	// Need to resize or add borders?
	if ((capture.width != largest_width) ||
		(capture.height != largest_height))
	{
		if (RESIZE)
		{
			scriptResize(avs, largest_width, largest_height);
		}
		else
		{
			scriptAddBorders(avs, capture.width, capture.height, largest_width, largest_height);
		}
	}
}

void LONGPLAY_Init(Section *section)
{
	// Register with the save state.
	SaveState &save_state = SaveState::instance();
	save_state.registerComponent("Longplay", save_state_component);
	save_state_component.section = section;

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
	if (CaptureState & CAPTURE_VIDEO) {
		CAPTURE_VideoEvent(true);
		assert((CaptureState & CAPTURE_VIDEO) == 0);
	}

	// Clear current capture.
	current_capture.file_name.clear();
	current_capture.frame_count = 0;

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
}

void LongPlaySaveStateComponent::writeScript() const
{
	// Compute the largest width and height.
	Bitu largest_width = current_capture.width;
	Bitu largest_height = current_capture.height;
	for (CaptureList::const_iterator it = previous_captures.begin(); it != previous_captures.end(); ++it)
	{
		if (it->width > largest_width)
		{
			largest_width = it->width;
		}
		if (it->height > largest_height)
		{
			largest_height = it->height;
		}
	}

	// Open the script file.
	FILE *avs = fopen("capture.avs", "w");
	if (avs == NULL)
	{
		return;
	}

	if (previous_captures.empty())
	{
		if (CaptureState & CAPTURE_VIDEO)
		{
			scriptCapture(avs, current_capture, largest_width, largest_height, section);
		}
	}
	else
	{
		scriptCapture(avs, previous_captures.front(), largest_width, largest_height, section);

		for (CaptureList::size_type i = 1; i < previous_captures.size(); ++i)
		{
			fputs(" ++ ", avs);

			const CaptureInfo &capture = previous_captures[i];
			scriptCapture(avs, capture, largest_width, largest_height, section);
		}

		if (CaptureState & CAPTURE_VIDEO)
		{
			fputs(" ++ ", avs);

			scriptCapture(avs, current_capture, largest_width, largest_height, section);
		}
	}

	// Close the script.
	fclose(avs);
	avs = NULL;
}
