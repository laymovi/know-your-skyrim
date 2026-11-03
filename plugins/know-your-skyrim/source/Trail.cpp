#include "PCH.h"

#include "Trail.h"

namespace Trail
{
	namespace
	{
		std::string lastStep;
		const RE::TESFile* lastFile = nullptr;
	}

	void Step(std::string_view a_step)
	{
		logger::info("step: {}", a_step);
	}

	void Reading(std::string_view a_step, const RE::TESForm* a_form)
	{
		const auto* file = a_form ? a_form->GetFile(0) : nullptr;
		if (file == lastFile && a_step == lastStep) {
			return;
		}
		lastFile = file;
		lastStep = a_step;
		logger::info("step: {} of {}", a_step, file ? file->GetFilename() : "no plugin"sv);
	}
}
