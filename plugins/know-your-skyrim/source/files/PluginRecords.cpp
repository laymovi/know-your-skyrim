#include "PCH.h"

#include "PluginRecords.h"

#include "GameArchives.h"

namespace PluginRecords
{
	std::string Zero(std::span<const std::byte> a_field)
	{
		std::string text(reinterpret_cast<const char*>(a_field.data()), a_field.size());
		if (const auto end = text.find('\0'); end != std::string::npos) {
			text.resize(end);
		}
		return text;
	}

	bool Map(const PluginSources::Source& a_source, const std::function<void(std::span<const std::byte>)>& a_walk)
	{
		auto* file = ::CreateFileW(a_source.path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			return false;
		}
		LARGE_INTEGER size{};
		::GetFileSizeEx(file, &size);
		auto* mapping = size.QuadPart >= static_cast<LONGLONG>(kHeader) ? ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr) : nullptr;
		const auto* view = mapping ? static_cast<const std::byte*>(::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0)) : nullptr;
		if (view) {
			a_walk(std::span<const std::byte>(view, static_cast<std::size_t>(size.QuadPart)));
			::UnmapViewOfFile(view);
		}
		if (mapping) {
			::CloseHandle(mapping);
		}
		::CloseHandle(file);
		return view != nullptr;
	}

	bool Localized(std::span<const std::byte> a_file)
	{
		return a_file.size() >= kHeader && Take<std::uint32_t>(a_file, 0) == Tag("TES4") && (Take<std::uint32_t>(a_file, 8) & kLocalized) != 0;
	}

	std::string Stem(const PluginSources::Source& a_source)
	{
		return GameArchives::Key(GameArchives::Narrow(a_source.path.stem()));
	}

	std::unordered_map<std::uint32_t, std::string> Strings(const GameArchives::Files& a_files, std::string_view a_stem, std::string_view a_kind, std::string_view a_language,
		const std::vector<std::uint32_t>& a_ids)
	{
		std::unordered_map<std::uint32_t, std::string> texts;
		const auto language = GameArchives::Key(a_language);
		auto bytes = a_files.Read(std::format("strings\\{}_{}.{}", a_stem, language, a_kind));
		if (!bytes && language != "english") {
			bytes = a_files.Read(std::format("strings\\{}_english.{}", a_stem, a_kind));
		}
		if (!bytes) {
			return texts;
		}
		const std::span<const std::byte> table(*bytes);
		const auto count = Take<std::uint32_t>(table, 0);
		const std::size_t start = 8 + static_cast<std::size_t>(count) * 8;
		std::unordered_map<std::uint32_t, std::uint32_t> offsets;
		for (std::size_t i = 0; i < count && 16 + i * 8 <= table.size(); ++i) {
			offsets[Take<std::uint32_t>(table, 8 + i * 8)] = Take<std::uint32_t>(table, 12 + i * 8);
		}
		for (const auto id : a_ids) {
			const auto found = offsets.find(id);
			if (found == offsets.end()) {
				continue;
			}
			const auto at = start + found->second;
			const auto length = Take<std::uint32_t>(table, at);
			if (at + 4 + length <= table.size()) {
				texts[id] = Zero(table.subspan(at + 4, length));
			}
		}
		return texts;
	}
}
