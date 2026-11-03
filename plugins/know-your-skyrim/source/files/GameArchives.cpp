#include "PCH.h"

#include "GameArchives.h"

namespace GameArchives
{
	namespace
	{
		constexpr std::uint32_t kVersion = 105;
		constexpr std::uint32_t kFolderNames = 0x1;
		constexpr std::uint32_t kFileNames = 0x2;
		constexpr std::uint32_t kPackedByDefault = 0x4;
		constexpr std::uint32_t kNamesInData = 0x100;
		constexpr std::uint32_t kPackToggle = 0x40000000;
		constexpr std::uint32_t kSizeBits = 0x3FFFFFFF;
		constexpr std::size_t kHeader = 36;
		constexpr std::size_t kFolderRecord = 24;
		constexpr std::size_t kFileRecord = 16;

		constexpr std::uint32_t kFrameMagic = 0x184D2204;
		constexpr std::uint8_t kBlockChecksum = 0x10;
		constexpr std::uint8_t kContentSize = 0x08;
		constexpr std::uint8_t kContentChecksum = 0x04;
		constexpr std::uint8_t kDictionary = 0x01;
		constexpr std::uint32_t kStored = 0x80000000;
		// a script or a strings file this big is no file of the game
		constexpr std::size_t kLargest = 256 * 1024 * 1024;

		template <class T>
		T Take(std::span<const std::byte> a_data, std::size_t a_at)
		{
			T value{};
			if (a_at + sizeof(T) <= a_data.size()) {
				std::memcpy(&value, a_data.data() + a_at, sizeof(T));
			}
			return value;
		}

		// one block of LZ4 appended to what came before, its matches may reach back into earlier blocks
		bool Block(std::span<const std::byte> a_in, std::vector<std::byte>& a_out)
		{
			std::size_t at = 0;
			const auto length = [&](std::size_t a_start) {
				std::size_t count = a_start;
				if (a_start == 15) {
					std::uint8_t more = 255;
					while (more == 255 && at < a_in.size()) {
						more = static_cast<std::uint8_t>(a_in[at++]);
						count += more;
					}
				}
				return count;
			};
			while (at < a_in.size()) {
				const auto token = static_cast<std::uint8_t>(a_in[at++]);
				const auto literals = length(token >> 4);
				if (at + literals > a_in.size() || a_out.size() + literals > kLargest) {
					return false;
				}
				a_out.insert(a_out.end(), a_in.begin() + at, a_in.begin() + at + literals);
				at += literals;
				// the last sequence of a block has its literals only
				if (at >= a_in.size()) {
					break;
				}
				if (at + 2 > a_in.size()) {
					return false;
				}
				const std::size_t back = static_cast<std::uint8_t>(a_in[at]) | static_cast<std::size_t>(static_cast<std::uint8_t>(a_in[at + 1])) << 8;
				at += 2;
				const auto match = length(token & 0xF) + 4;
				if (back == 0 || back > a_out.size() || a_out.size() + match > kLargest) {
					return false;
				}
				// a match may run over its own start, it's copied one byte at a time
				auto from = a_out.size() - back;
				for (std::size_t i = 0; i < match; ++i) {
					a_out.push_back(a_out[from + i]);
				}
			}
			return true;
		}
	}

	std::string Narrow(const std::filesystem::path& a_path)
	{
		try {
			return a_path.string();
		} catch (const std::system_error&) {
			const auto name = a_path.u8string();
			return std::string(name.begin(), name.end());
		}
	}

	std::string Key(std::string_view a_path)
	{
		std::string key(a_path);
		for (auto& letter : key) {
			letter = letter == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
		}
		while (key.starts_with('\\')) {
			key.erase(0, 1);
		}
		return key;
	}

	bool Unpack(std::span<const std::byte> a_frame, std::vector<std::byte>& a_out)
	{
		if (a_frame.size() < 7 || Take<std::uint32_t>(a_frame, 0) != kFrameMagic) {
			return false;
		}
		const auto flags = static_cast<std::uint8_t>(a_frame[4]);
		std::size_t at = 6;
		at += flags & kContentSize ? 8 : 0;
		at += flags & kDictionary ? 4 : 0;
		// the check byte of the header
		at += 1;
		while (at + 4 <= a_frame.size()) {
			const auto told = Take<std::uint32_t>(a_frame, at);
			at += 4;
			if (told == 0) {
				return true;
			}
			const std::size_t size = told & ~kStored;
			if (at + size > a_frame.size()) {
				return false;
			}
			const auto block = a_frame.subspan(at, size);
			if (told & kStored) {
				a_out.insert(a_out.end(), block.begin(), block.end());
			} else if (!Block(block, a_out)) {
				return false;
			}
			at += size + (flags & kBlockChecksum ? 4 : 0);
		}
		// a frame may end without its end mark when the archive gives its exact size
		return !(flags & kContentChecksum) || at >= a_frame.size();
	}

	Files::Archive::Archive(std::filesystem::path a_path) :
		path(std::move(a_path))
	{
		file = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			file = nullptr;
			return;
		}
		LARGE_INTEGER length{};
		::GetFileSizeEx(file, &length);
		mapping = length.QuadPart >= static_cast<LONGLONG>(kHeader) ? ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr) : nullptr;
		view = mapping ? static_cast<const std::byte*>(::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0)) : nullptr;
		size = view ? static_cast<std::size_t>(length.QuadPart) : 0;
	}

	Files::Archive::Archive(Archive&& a_other) noexcept :
		path(std::move(a_other.path)), namesInData(a_other.namesInData), wanted(a_other.wanted), file(a_other.file), mapping(a_other.mapping), view(a_other.view),
		size(a_other.size), heldFrom(a_other.heldFrom), held(std::move(a_other.held))
	{
		a_other.file = a_other.mapping = nullptr;
		a_other.view = nullptr;
		a_other.size = 0;
	}

	Files::Archive::~Archive()
	{
		Close();
	}

	void Files::Archive::Close()
	{
		if (view) {
			::UnmapViewOfFile(view);
		}
		if (mapping) {
			::CloseHandle(mapping);
		}
		if (file) {
			::CloseHandle(file);
		}
		file = mapping = nullptr;
		view = nullptr;
		size = 0;
		held.clear();
		held.shrink_to_fit();
	}

	std::span<const std::byte> Files::Archive::Part(std::size_t a_from, std::size_t a_size) const
	{
		if (a_from >= heldFrom && a_from + a_size <= heldFrom + held.size()) {
			return std::span<const std::byte>(held).subspan(a_from - heldFrom, a_size);
		}
		const auto bytes = Bytes();
		return a_from + a_size <= bytes.size() ? bytes.subspan(a_from, a_size) : std::span<const std::byte>();
	}

	bool Files::Archive::Hold(std::size_t a_from, std::size_t a_to)
	{
		if (!file || a_to <= a_from || a_to > size) {
			return false;
		}
		std::vector<std::byte> bytes(a_to - a_from);
		OVERLAPPED at{};
		at.Offset = static_cast<DWORD>(a_from);
		at.OffsetHigh = static_cast<DWORD>(static_cast<std::uint64_t>(a_from) >> 32);
		DWORD read = 0;
		if (!::ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, &at) || read != bytes.size()) {
			return false;
		}
		heldFrom = a_from;
		held = std::move(bytes);
		return true;
	}

	Files::Files(const std::vector<std::string>& a_archives, std::initializer_list<std::string_view> a_folders)
	{
		const std::vector<std::string_view> folders(a_folders);
		archives.reserve(a_archives.size());
		for (const auto& name : a_archives) {
			archives.emplace_back(std::filesystem::path("Data") / name);
			// a broken archive is left out, the others still count
			try {
				Index(static_cast<std::uint32_t>(archives.size() - 1), folders);
			} catch (const std::exception& a_error) {
				logger::warn("archive {} left out: {}", name, a_error.what());
			}
			// an archive of none of the folders asked for needn't stay mapped, a load order has hundreds
			if (!archives.back().wanted) {
				archives.back().Close();
			}
		}
		// loose files are listed once, an open tried for every script of a load order costs far more through
		// a virtual file system. A name in letters the code page of the system hasn't got stays in UTF-8
		for (const auto folder : folders) {
			std::error_code error;
			for (std::filesystem::directory_iterator at(std::filesystem::path("Data") / folder, error), end; !error && at != end; at.increment(error)) {
				if (at->is_regular_file(error)) {
					const auto name = at->path().filename().u8string();
					loose.insert(Key(std::format("{}\\{}", folder, std::string(name.begin(), name.end()))));
				}
			}
		}
	}

	void Files::Index(std::uint32_t a_archive, std::span<const std::string_view> a_folders)
	{
		auto& archive = archives[a_archive];
		const auto bytes = archive.Bytes();
		if (bytes.size() < kHeader) {
			return;
		}
		const auto header = bytes.first(kHeader);
		const auto version = Take<std::uint32_t>(header, 4);
		const auto flags = Take<std::uint32_t>(header, 12);
		const auto folderCount = Take<std::uint32_t>(header, 16);
		const auto fileCount = Take<std::uint32_t>(header, 20);
		const auto namesLength = Take<std::uint32_t>(header, 28);
		if (std::memcmp(header.data(), "BSA\0", 4) != 0 || version != kVersion || !(flags & kFolderNames) || !(flags & kFileNames)) {
			return;
		}
		archive.namesInData = (flags & kNamesInData) != 0;
		// counts a broken header makes up would ask for more than the file holds
		const auto blocks = static_cast<std::uint64_t>(folderCount) * (kFolderRecord + 1) + static_cast<std::uint64_t>(fileCount) * kFileRecord +
		                    Take<std::uint32_t>(header, 24) + namesLength;
		if (kHeader + blocks > bytes.size()) {
			return;
		}
		// the folder names and the file records follow the folder records, then the file names, all of it in a row
		const auto records = bytes.subspan(kHeader, static_cast<std::size_t>(folderCount) * kFolderRecord);
		const auto folderBytes = bytes.subspan(kHeader + records.size(), static_cast<std::size_t>(folderCount) + static_cast<std::size_t>(fileCount) * kFileRecord + Take<std::uint32_t>(header, 24));
		const auto names = bytes.subspan(kHeader + records.size() + folderBytes.size(), namesLength);
		std::size_t inFolders = 0;
		std::size_t inNames = 0;
		const auto nextName = [&]() {
			std::string name;
			while (inNames < names.size() && names[inNames] != std::byte{ 0 }) {
				name += static_cast<char>(names[inNames++]);
			}
			inNames++;
			return name;
		};
		const bool packedByDefault = (flags & kPackedByDefault) != 0;
		for (std::uint32_t f = 0; f < folderCount; ++f) {
			const auto count = Take<std::uint32_t>(records, f * kFolderRecord + 8);
			const auto length = static_cast<std::uint8_t>(inFolders < folderBytes.size() ? folderBytes[inFolders] : std::byte{ 0 });
			std::string folder;
			for (std::size_t i = 1; i < length && inFolders + i < folderBytes.size(); ++i) {
				folder += static_cast<char>(folderBytes[inFolders + i]);
			}
			inFolders += 1 + length;
			const auto key = Key(folder);
			const bool wanted = std::ranges::find(a_folders, std::string_view(key)) != a_folders.end();
			archive.wanted = archive.wanted || wanted;
			if (count > (folderBytes.size() - std::min(inFolders, folderBytes.size())) / kFileRecord) {
				return;
			}
			Stretch stretch{ a_archive, bytes.size(), 0 };
			for (std::uint32_t i = 0; i < count; ++i) {
				const auto size = Take<std::uint32_t>(folderBytes, inFolders + 8);
				const auto offset = Take<std::uint32_t>(folderBytes, inFolders + 12);
				inFolders += kFileRecord;
				auto name = nextName();
				if (wanted) {
					// a later archive wins
					entries[Key(std::format("{}\\{}", key, name))] = { a_archive, offset, size & kSizeBits, packedByDefault != ((size & kPackToggle) != 0) };
					stretch.from = std::min<std::size_t>(stretch.from, offset);
					stretch.to = std::max<std::size_t>(stretch.to, static_cast<std::size_t>(offset) + (size & kSizeBits));
				}
			}
			if (wanted && stretch.from < stretch.to && stretch.to <= bytes.size()) {
				stretches[key].push_back(stretch);
			}
		}
	}

	std::size_t Files::Hold(std::string_view a_folder)
	{
		const auto found = stretches.find(std::string(a_folder));
		std::size_t holding = 0;
		for (const auto& stretch : found != stretches.end() ? found->second : std::vector<Stretch>{}) {
			if (archives[stretch.archive].Hold(stretch.from, stretch.to)) {
				holding += stretch.to - stretch.from;
			}
		}
		return holding;
	}

	std::optional<std::vector<std::byte>> Files::Read(std::string_view a_path) const
	{
		using Clock = std::chrono::steady_clock;
		const auto since = [](Clock::time_point a_start) { return std::chrono::duration<double, std::milli>(Clock::now() - a_start).count(); };
		const auto start = Clock::now();
		const auto key = Key(a_path);
		if (loose.contains(key)) {
			std::ifstream stream(std::filesystem::path("Data") / key, std::ios::binary);
			if (stream) {
				std::vector<std::byte> data;
				stream.seekg(0, std::ios::end);
				const auto size = static_cast<std::size_t>(std::max<std::streamoff>(stream.tellg(), 0));
				if (size <= kLargest) {
					data.resize(size);
					stream.seekg(0);
					stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
					took.loose += since(start);
					took.looseFiles++;
					return data;
				}
			}
		}
		const auto found = entries.find(key);
		if (found == entries.end() || found->second.size > kLargest) {
			return std::nullopt;
		}
		const auto& entry = found->second;
		const auto& archive = archives[entry.archive];
		const auto raw = archive.Part(entry.offset, entry.size);
		if (raw.size() != entry.size) {
			return std::nullopt;
		}
		std::size_t skip = 0;
		if (archive.namesInData && !raw.empty()) {
			skip = 1 + static_cast<std::uint8_t>(raw[0]);
		}
		if (skip > raw.size()) {
			return std::nullopt;
		}
		if (!entry.packed) {
			std::vector<std::byte> data(raw.begin() + skip, raw.end());
			took.archived += since(start);
			took.archivedFiles++;
			return data;
		}
		if (skip + 4 > raw.size()) {
			return std::nullopt;
		}
		const auto whole = Take<std::uint32_t>(raw, skip);
		std::vector<std::byte> data;
		data.reserve(std::min<std::size_t>(whole, kLargest));
		const bool unpacked = Unpack(raw.subspan(skip + 4), data) && data.size() == whole;
		took.unpacked += since(start);
		took.unpackedFiles++;
		if (!unpacked) {
			return std::nullopt;
		}
		return data;
	}

	// the archives the game loads: the ones its ini lists, then one for each plugin of the load order. Archives
	// of textures hold no scripts and no strings
	std::vector<std::string> LoadOrderArchives()
	{
		std::vector<std::string> names;
		std::unordered_set<std::string> known;
		const auto add = [&](std::string_view a_name) {
			const auto start = a_name.find_first_not_of(" \t");
			const auto end = a_name.find_last_not_of(" \t");
			if (start == std::string_view::npos) {
				return;
			}
			std::string name(a_name.substr(start, end - start + 1));
			const auto key = Key(name);
			if (key.find(" - textures") == std::string::npos && known.insert(key).second) {
				names.push_back(std::move(name));
			}
		};
		auto* settings = RE::INISettingCollection::GetSingleton();
		for (const auto* key : { "sResourceArchiveList:Archive", "sResourceArchiveList2:Archive" }) {
			const auto* setting = settings ? settings->GetSetting(key) : nullptr;
			const char* listed = setting && setting->GetType() == RE::Setting::Type::kString ? setting->GetString() : nullptr;
			for (std::string_view rest = listed ? listed : ""; !rest.empty();) {
				const auto comma = rest.find(',');
				add(rest.substr(0, comma));
				rest = comma == std::string_view::npos ? std::string_view() : rest.substr(comma + 1);
			}
		}
		for (const auto* file : RE::TESDataHandler::GetSingleton()->files) {
			if (file && file->compileIndex != 0xFF) {
				add(std::format("{}.bsa", Narrow(std::filesystem::path(file->fileName).stem())));
			}
		}
		return names;
	}

	std::string Language()
	{
		const auto* setting = RE::INISettingCollection::GetSingleton()->GetSetting("sLanguage:General");
		const char* told = setting ? setting->GetString() : nullptr;
		return told && *told ? told : "ENGLISH";
	}
}
