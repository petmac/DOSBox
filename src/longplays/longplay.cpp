#include "longplay.h"

#include "../save_state.h"

#include <stdint.h>

static const uint32_t CURRENT_VERSION = 0;

class LongPlaySaveStateComponent : public SaveState::Component
{
public:

	void getBytes(std::ostream& stream) override;
	void setBytes(std::istream& stream) override;
};

static LongPlaySaveStateComponent save_state_component;

void LONGPLAY_Init(Section *section)
{
	// Register with the save state.
	SaveState &save_state = SaveState::instance();
	save_state.registerComponent("longplay", save_state_component);
}

void LongPlaySaveStateComponent::getBytes(std::ostream& stream)
{
}

void LongPlaySaveStateComponent::setBytes(std::istream& stream)
{
}
