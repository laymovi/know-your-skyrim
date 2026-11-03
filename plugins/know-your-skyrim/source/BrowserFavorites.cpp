#include "PCH.h"

#include "BrowserFavorites.h"

namespace BrowserFavorites
{
	namespace
	{
		constexpr auto kFile = "KnowYourSkyrim.favorites.txt";

		std::unordered_set<RE::FormID> starred;
		// lines of plugins this load order lacks, kept so they come back with the plugin
		std::vector<std::string> absent;
		bool read = false;
		std::uint32_t version = 0;

		std::optional<std::filesystem::path> Path()
		{
			const auto folder = SKSE::log::log_directory();
			return folder ? std::optional(*folder / kFile) : std::nullopt;
		}

		// "Skyrim.esm|012EB7"
		std::optional<std::string> LineOf(RE::FormID a_form)
		{
			const auto* form = RE::TESForm::LookupByID(a_form);
			const auto* file = form ? form->GetFile(0) : nullptr;
			if (!file) {
				return std::nullopt;
			}
			const auto own = file->IsLight() ? a_form & 0xFFF : a_form & 0xFFFFFF;
			return std::format("{}|{:06X}", file->GetFilename(), own);
		}

		void Read()
		{
			if (read) {
				return;
			}
			read = true;
			const auto path = Path();
			std::ifstream file(path ? *path : std::filesystem::path());
			auto* data = RE::TESDataHandler::GetSingleton();
			for (std::string line; std::getline(file, line);) {
				const auto bar = line.find('|');
				if (bar == std::string::npos) {
					continue;
				}
				std::uint32_t own = 0;
				const auto number = std::string_view(line).substr(bar + 1);
				std::from_chars(number.data(), number.data() + number.size(), own, 16);
				const auto form = data->LookupFormID(own, line.substr(0, bar));
				if (form != 0) {
					starred.insert(form);
				} else {
					absent.push_back(line);
				}
			}
			logger::info("{} favorites, {} of plugins not in this load order", starred.size(), absent.size());
		}

		void Write()
		{
			const auto path = Path();
			if (!path) {
				return;
			}
			std::vector<std::string> lines = absent;
			for (const auto form : starred) {
				if (auto line = LineOf(form)) {
					lines.push_back(std::move(*line));
				}
			}
			std::ranges::sort(lines);
			std::ofstream file(*path, std::ios::trunc);
			for (const auto& line : lines) {
				file << line << '\n';
			}
			if (!file) {
				logger::warn("the favorites couldn't be written to {}", path->string());
			}
		}
	}

	bool Has(RE::FormID a_form)
	{
		Read();
		return starred.contains(a_form);
	}

	void Set(RE::FormID a_form, bool a_on)
	{
		Read();
		if (a_on ? starred.insert(a_form).second : starred.erase(a_form) > 0) {
			version++;
			Write();
		}
	}

	std::uint32_t Version()
	{
		return version;
	}
}
