#include "PCH.h"

#include "PluginSources.h"

namespace PluginSources
{
	RE::FormID Source::Runtime(std::uint32_t a_raw) const
	{
		const auto index = std::min<std::size_t>(a_raw >> 24, owners.size() - 1);
		const auto& owner = owners[index];
		return owner.light ? (owner.partial << 12 | (a_raw & 0xFFF)) : (owner.partial << 24 | (a_raw & 0xFFFFFF));
	}

	Source SourceOf(const RE::TESFile& a_file)
	{
		Source source;
		const std::filesystem::path folder = *a_file.path ? std::filesystem::path(a_file.path) : std::filesystem::path("Data");
		source.path = folder / a_file.fileName;
		for (std::uint32_t i = 0; i < a_file.masterCount; ++i) {
			const auto* master = a_file.masterPtrs ? a_file.masterPtrs[i] : nullptr;
			source.owners.push_back(master ? Owner{ master->GetPartialIndex(), master->IsLight() } : Owner{});
		}
		source.owners.push_back({ a_file.GetPartialIndex(), a_file.IsLight() });
		return source;
	}

	std::vector<Source> LoadOrder()
	{
		std::vector<Source> sources;
		for (const auto* file : RE::TESDataHandler::GetSingleton()->files) {
			// a file the load order doesn't use has no place in it
			if (file && file->compileIndex != 0xFF) {
				sources.push_back(SourceOf(*file));
			}
		}
		return sources;
	}
}
