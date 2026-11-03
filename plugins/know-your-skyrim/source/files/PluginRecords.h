#pragma once

#include "PluginSources.h"

namespace GameArchives
{
	class Files;
}

// What the readers of plugin files on the browser's threads share: a file mapped for one walk, the fields of a record,
// the texts of a localized plugin from its strings files
namespace PluginRecords
{
	inline constexpr std::size_t kHeader = 24;
	inline constexpr std::uint32_t kCompressed = 0x00040000;
	inline constexpr std::uint32_t kLocalized = 0x80;

	constexpr std::uint32_t Tag(const char (&a_tag)[5])
	{
		return static_cast<std::uint32_t>(a_tag[0]) | static_cast<std::uint32_t>(a_tag[1]) << 8 | static_cast<std::uint32_t>(a_tag[2]) << 16 |
		       static_cast<std::uint32_t>(a_tag[3]) << 24;
	}

	template <class T>
	T Take(std::span<const std::byte> a_data, std::size_t a_at)
	{
		T value{};
		if (a_at + sizeof(T) <= a_data.size()) {
			std::memcpy(&value, a_data.data() + a_at, sizeof(T));
		}
		return value;
	}

	// a text of a field up to its first zero
	std::string Zero(std::span<const std::byte> a_field);

	// the fields of a record one after another, a field longer than 65535 bytes is told by an XXXX before it
	template <class F>
	void Fields(std::span<const std::byte> a_record, F&& a_take)
	{
		std::size_t at = 0;
		std::uint32_t longer = 0;
		while (at + 6 <= a_record.size()) {
			const auto type = Take<std::uint32_t>(a_record, at);
			const std::size_t length = longer ? longer : Take<std::uint16_t>(a_record, at + 4);
			longer = 0;
			if (type == Tag("XXXX") && at + 10 <= a_record.size()) {
				longer = Take<std::uint32_t>(a_record, at + 6);
				at += 10;
				continue;
			}
			if (at + 6 + length > a_record.size()) {
				return;
			}
			a_take(type, a_record.subspan(at + 6, length));
			at += 6 + length;
		}
	}

	// the whole file mapped for a_walk, false when it can't be read
	bool Map(const PluginSources::Source& a_source, const std::function<void(std::span<const std::byte>)>& a_walk);

	// the plugin's texts are in its strings files, the file's header says
	bool Localized(std::span<const std::byte> a_file);

	// the stem of a plugin's file in lower case, the name its strings files start with
	std::string Stem(const PluginSources::Source& a_source);

	// The texts of a_ids from a strings file of the plugin, "dlstrings" or "ilstrings", in the game's language or in
	// English when the game's is missing. A text of such a file is its length and then its letters
	std::unordered_map<std::uint32_t, std::string> Strings(const GameArchives::Files& a_files, std::string_view a_stem, std::string_view a_kind, std::string_view a_language,
		const std::vector<std::uint32_t>& a_ids);
}
