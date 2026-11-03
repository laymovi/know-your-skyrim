#include "PCH.h"

#include "Options.h"

namespace Options
{
	namespace
	{
		constexpr auto kShipped = "Data/SKSE/Plugins/KnowYourSkyrim.ini";
		constexpr auto kSetInMenu = "Data/MCM/Settings/KnowYourSkyrim.ini";
		constexpr std::string_view kBlank = " \t\r";
		// a file some menus write starts with this mark, the first section would be lost with it
		constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";

		std::atomic<bool> cheats{ false };
		std::atomic<bool> progression{ false };
		// "Section:key" to its value, the menu's file over the shipped one
		std::map<std::string, std::string> merged;
		std::mutex mergedLock;

		std::string Trimmed(std::string_view a_text)
		{
			const auto first = a_text.find_first_not_of(kBlank);
			const auto last = a_text.find_last_not_of(kBlank);
			return first == std::string_view::npos ? std::string() : std::string(a_text.substr(first, last - first + 1));
		}

		// "Section:key" to its value, a line of ; is a comment
		std::map<std::string, std::string> Read(const char* a_file)
		{
			std::map<std::string, std::string> told;
			std::ifstream file(a_file);
			std::string section;
			for (std::string line; std::getline(file, line);) {
				if (line.starts_with(kByteOrderMark)) {
					line.erase(0, kByteOrderMark.size());
				}
				const auto text = Trimmed(line);
				if (text.empty() || text.front() == ';' || text.front() == '#') {
					continue;
				}
				if (text.front() == '[' && text.back() == ']') {
					section = text.substr(1, text.size() - 2);
					continue;
				}
				const auto equals = text.find('=');
				if (equals != std::string::npos) {
					told[section + ":" + Trimmed(std::string_view(text).substr(0, equals))] = Trimmed(std::string_view(text).substr(equals + 1));
				}
			}
			return told;
		}

		bool Switched(const std::string& a_value)
		{
			return !a_value.empty() && a_value != "0" && _stricmp(a_value.c_str(), "false") != 0;
		}

		// "bEnabled:Cheats" as the menu names it to "Cheats:bEnabled" as the files key it. A script hands a text over
		// in the letter case the game met first, so the case doesn't count
		std::string KeyOf(std::string_view a_id)
		{
			const auto colon = a_id.find(':');
			const auto key = std::string(a_id.substr(colon + 1)) + ':' + std::string(a_id.substr(0, colon));
			for (const auto& [known, value] : merged) {
				if (_stricmp(known.c_str(), key.c_str()) == 0) {
					return known;
				}
			}
			return key;
		}
	}

	void Load()
	{
		auto told = Read(kShipped);
		for (auto& [key, value] : Read(kSetInMenu)) {
			told[key] = std::move(value);
		}
		const auto found = told.find("Cheats:bEnabled");
		const bool on = found != told.end() && Switched(found->second);
		if (on != cheats.exchange(on)) {
			logger::info("cheats {}", on ? "on" : "off");
		}
		const auto mode = told.find("Progression:bEnabled");
		const bool known = mode != told.end() && Switched(mode->second);
		if (known != progression.exchange(known)) {
			logger::info("progression mode {}", known ? "on" : "off");
		}
		const std::scoped_lock lock{ mergedLock };
		merged = std::move(told);
	}

	bool Cheats()
	{
		return cheats;
	}

	bool Progression()
	{
		return progression;
	}

	float Value(std::string_view a_id)
	{
		if (a_id.find(':') == std::string_view::npos) {
			return 0.0F;
		}
		const std::scoped_lock lock{ mergedLock };
		const auto found = merged.find(KeyOf(a_id));
		return found != merged.end() && Switched(found->second) ? 1.0F : 0.0F;
	}

	std::string Text(std::string_view a_id)
	{
		if (a_id.find(':') == std::string_view::npos) {
			return {};
		}
		const std::scoped_lock lock{ mergedLock };
		const auto found = merged.find(KeyOf(a_id));
		return found != merged.end() ? found->second : std::string();
	}

	void Set(std::string_view a_id, float a_value)
	{
		if (a_id.find(':') == std::string_view::npos) {
			return;
		}
		{
			const std::scoped_lock lock{ mergedLock };
			const auto key = KeyOf(a_id);
			auto lines = Read(kSetInMenu);
			lines[key] = merged[key] = a_value != 0.0F ? "1" : "0";
			const std::filesystem::path file(kSetInMenu);
			std::error_code failed;
			std::filesystem::create_directories(file.parent_path(), failed);
			std::ofstream stream(file, std::ios::trunc);
			// the lines come sorted as "Section:key", every section is written once with all of its keys
			std::string section;
			for (const auto& [where, value] : lines) {
				const auto at = where.find(':');
				if (where.substr(0, at) != section) {
					section = where.substr(0, at);
					stream << (stream.tellp() > 0 ? "\n[" : "[") << section << "]\n";
				}
				stream << where.substr(at + 1) << " = " << value << '\n';
			}
			if (!stream) {
				logger::warn("the menu couldn't write {}", kSetInMenu);
			}
		}
		Load();
	}
}
