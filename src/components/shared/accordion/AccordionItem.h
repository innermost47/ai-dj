#pragma once
#include "ObsidianBase.h"

class AccordionItem : public virtual ObsidianComponent
{
  public:
	AccordionItem() = default;
	~AccordionItem() override = default;

	virtual int getPreferredHeight(int width) const = 0;

  private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AccordionItem)
};