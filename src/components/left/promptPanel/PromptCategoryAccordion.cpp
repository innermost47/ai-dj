#include "PromptCategoryAccordion.h"
#include "ObsidianAccordion.h"
#include "PromptBankItem.h"

void PromptCategoryAccordion::setItems(std::vector<std::unique_ptr<PromptBankItem>> &&newItems)
{
	std::vector<std::unique_ptr<AccordionItem>> upcasted;
	upcasted.reserve(newItems.size());
	for (auto &item : newItems)
		upcasted.emplace_back(std::move(item));
	ObsidianAccordion::setItems(std::move(upcasted));
}
