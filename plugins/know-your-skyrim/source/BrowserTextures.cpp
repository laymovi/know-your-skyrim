#include "PCH.h"

#include "BrowserTextures.h"

#include <cstring>
#include <set>
#include <thread>
#include <unordered_set>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Options.h"
#include "Trail.h"

namespace BrowserTextures
{
	namespace
	{
		namespace fs = std::filesystem;
		using Clock = std::chrono::steady_clock;

		// the share of a frame the models are read in while the inventory is open, a model out of an archive takes a few
		// milliseconds
		constexpr auto kFrameBudget = std::chrono::microseconds(8000);
		constexpr std::string_view kModOrganizer = "Mod Organizer";
		constexpr std::string_view kVortex = "Vortex";
		constexpr std::string_view kArchive = "archive";
		constexpr std::size_t kLongestPath = 260;
		constexpr std::uint16_t kMostMods = 0xFFFE;

		// What the thread found. The game's thread looks at it only once ready is set
		std::vector<Mod> mods;
		std::unordered_map<std::string, std::vector<std::uint16_t>> owners;
		std::atomic<bool> started{ false };
		std::atomic<bool> ready{ false };

		// the mods that change the textures a model names, by the model's path
		std::unordered_map<std::string, std::vector<std::uint16_t>> modelChangers;
		// the mods that change a thing, by its place in the rows, and whether its models were read
		std::unordered_map<std::uint32_t, std::vector<std::uint16_t>> itemChangers;
		std::vector<char> itemRead;
		std::size_t nextItem = 0;
		bool wantAll = false;
		std::vector<std::size_t> changedBy;

		// lower case and back slashes, the way the game's files are named
		std::string Plain(std::string_view a_path)
		{
			std::string plain;
			plain.reserve(a_path.size());
			for (const char letter : a_path) {
				plain.push_back(letter == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
			}
			const auto start = plain.find_first_not_of('\\');
			return start == std::string::npos ? std::string() : plain.substr(start);
		}

		std::string Narrow(const fs::path& a_path)
		{
			const auto wide = a_path.u8string();
			return std::string(reinterpret_cast<const char*>(wide.data()), wide.size());
		}

		fs::path Wide(std::string_view a_utf8)
		{
			return fs::path(std::u8string(reinterpret_cast<const char8_t*>(a_utf8.data()), a_utf8.size()));
		}

		// a texture a model or a texture set names, as a path of Data
		std::string TexturePath(std::string_view a_named)
		{
			auto path = Plain(a_named);
			if (path.starts_with("data\\")) {
				path.erase(0, 5);
			}
			return path.starts_with("textures\\") ? path : "textures\\" + path;
		}

		// Textures every thing shares are no look of a thing of its own, like the reflections of the cubemaps and the
		// skin, faces and hair of the people an armor shows parts of. A mod of skins or eyes changes no thing
		bool Shared(std::string_view a_texture)
		{
			return a_texture.starts_with("textures\\cubemaps\\") || a_texture.starts_with("textures\\actors\\character\\");
		}

		enum class Kind
		{
			kTexture,
			kPlugin,
			kArchive,
			kNote,
			kOther
		};

		// A file of a mod by its path in the mod. Readmes, pictures of its page and what a mod manager keeps beside the
		// files do nothing in the game
		Kind KindOf(std::string_view a_path)
		{
			if (a_path.starts_with("textures\\")) {
				return Kind::kTexture;
			}
			const auto dot = a_path.rfind('.');
			const auto ending = dot != std::string_view::npos ? a_path.substr(dot) : std::string_view();
			const bool top = a_path.find('\\') == std::string_view::npos;
			if (top && (ending == ".esp" || ending == ".esm" || ending == ".esl")) {
				return Kind::kPlugin;
			}
			if (top && ending == ".bsa") {
				return Kind::kArchive;
			}
			if (a_path.starts_with("fomod\\") || a_path.starts_with("docs\\") || a_path.starts_with("readme") || a_path == "meta.ini") {
				return Kind::kNote;
			}
			constexpr std::array<std::string_view, 10> kNotes{ ".txt", ".md", ".pdf", ".htm", ".html", ".rtf", ".jpg", ".jpeg", ".png", ".url" };
			return top && std::ranges::find(kNotes, ending) != kNotes.end() ? Kind::kNote : Kind::kOther;
		}

		// A plugin with no records, made only so the game loads its archive. The number of records stands in its header
		bool Empty(const fs::path& a_plugin)
		{
			std::ifstream file(a_plugin, std::ios::binary);
			std::array<char, 42> head{};
			if (!file.read(head.data(), head.size()) || std::memcmp(head.data(), "TES4", 4) != 0 || std::memcmp(head.data() + 24, "HEDR", 4) != 0) {
				return false;
			}
			std::int32_t records = 0;
			std::memcpy(&records, head.data() + 34, sizeof(records));
			return records == 0;
		}

		template <class T>
		bool Take(std::ifstream& a_file, T& a_value)
		{
			return static_cast<bool>(a_file.read(reinterpret_cast<char*>(&a_value), sizeof(T)));
		}

		// The files an archive of Skyrim holds by their paths, from its tables of names alone. None for a file that is no
		// archive of Skyrim, or one without names
		std::vector<std::string> ArchiveFiles(const fs::path& a_archive)
		{
			struct Header
			{
				char magic[4];
				std::uint32_t version;
				std::uint32_t folderOffset;
				std::uint32_t flags;
				std::uint32_t folders;
				std::uint32_t files;
				std::uint32_t folderNames;
				std::uint32_t fileNames;
				std::uint32_t fileFlags;
			};
			constexpr std::uint32_t kFolderNames = 0x1;
			constexpr std::uint32_t kFileNames = 0x2;
			std::vector<std::string> found;
			std::ifstream file(a_archive, std::ios::binary);
			Header header{};
			if (!Take(file, header) || std::memcmp(header.magic, "BSA\0", 4) != 0 || (header.version != 104 && header.version != 105) ||
				(header.flags & kFolderNames) == 0 || (header.flags & kFileNames) == 0) {
				return found;
			}
			// a folder's record is its hash, how many files it has and where they are, wider in the archives of SE
			const std::uint32_t record = header.version == 105 ? 24 : 16;
			std::vector<std::uint32_t> counts(header.folders);
			for (auto& count : counts) {
				std::vector<char> one(record);
				if (!file.read(one.data(), record)) {
					return {};
				}
				std::memcpy(&count, one.data() + 8, sizeof(count));
			}
			std::vector<std::string> folders;
			folders.reserve(counts.size());
			for (const auto count : counts) {
				std::uint8_t length = 0;
				std::string name;
				if (!Take(file, length)) {
					return {};
				}
				name.resize(length);
				if (!file.read(name.data(), length)) {
					return {};
				}
				if (!name.empty() && name.back() == '\0') {
					name.pop_back();
				}
				folders.push_back(Plain(name));
				file.seekg(static_cast<std::streamoff>(count) * 16, std::ios::cur);
			}
			std::string names(header.fileNames, '\0');
			if (!file.read(names.data(), header.fileNames)) {
				return {};
			}
			std::size_t at = 0;
			for (std::size_t f = 0; f < folders.size(); ++f) {
				for (std::uint32_t i = 0; i < counts[f] && at < names.size(); ++i) {
					const auto end = names.find('\0', at);
					found.push_back(folders[f] + "\\" + Plain(std::string_view(names).substr(at, end - at)));
					at = end == std::string::npos ? names.size() : end + 1;
				}
			}
			return found;
		}

		// The files of one mod sorted out
		struct Sorted
		{
			std::vector<std::string> textures;
			std::vector<fs::path> plugins;
			std::vector<fs::path> archives;
			bool other = false;
		};

		void Sort(Sorted& a_sorted, std::string a_path, const fs::path& a_file)
		{
			switch (KindOf(a_path)) {
			case Kind::kTexture:
				a_sorted.textures.push_back(std::move(a_path));
				break;
			case Kind::kPlugin:
				a_sorted.plugins.push_back(a_file);
				break;
			case Kind::kArchive:
				a_sorted.archives.push_back(a_file);
				break;
			case Kind::kOther:
				a_sorted.other = true;
				break;
			default:
				break;
			}
		}

		// Textures and nothing else that does anything: plugins with no records, archives of textures alone
		bool OnlyTextures(Sorted& a_sorted)
		{
			if (a_sorted.other || !std::ranges::all_of(a_sorted.plugins, Empty)) {
				return false;
			}
			for (const auto& archive : a_sorted.archives) {
				for (auto& file : ArchiveFiles(archive)) {
					if (!file.starts_with("textures\\")) {
						return false;
					}
					a_sorted.textures.push_back(std::move(file));
				}
			}
			return !a_sorted.textures.empty();
		}

		void AddMod(std::string a_name, std::string_view a_from, const std::vector<std::string>& a_textures)
		{
			if (mods.size() >= kMostMods) {
				return;
			}
			const auto index = static_cast<std::uint16_t>(mods.size());
			mods.push_back({ std::move(a_name), a_from, a_textures.size() });
			for (const auto& texture : a_textures) {
				auto& list = owners[texture];
				if (list.empty() || list.back() != index) {
					list.push_back(index);
				}
			}
		}

		// The instance of Mod Organizer that runs the game: the folder the ini names, the one its file system came from
		// when it keeps its instance beside itself, or the instance its registry names
		std::optional<fs::path> ModOrganizerFolder()
		{
			if (const auto told = Options::Text("sModOrganizer:Mods"); !told.empty()) {
				return Wide(told);
			}
			auto* system = GetModuleHandleW(L"usvfs_x64.dll");
			if (!system) {
				return std::nullopt;
			}
			std::wstring file(MAX_PATH, L'\0');
			file.resize(GetModuleFileNameW(system, file.data(), static_cast<DWORD>(file.size())));
			const auto home = fs::path(file).parent_path();
			std::error_code failed;
			if (fs::exists(home / "portable.txt", failed)) {
				return home;
			}
			std::array<wchar_t, MAX_PATH> name{};
			DWORD size = static_cast<DWORD>(name.size() * sizeof(wchar_t));
			if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Mod Organizer Team\\Mod Organizer", L"CurrentInstance", RRF_RT_REG_SZ, nullptr, name.data(), &size) != ERROR_SUCCESS || !name[0]) {
				return std::nullopt;
			}
			std::array<wchar_t, MAX_PATH> local{};
			if (GetEnvironmentVariableW(L"LOCALAPPDATA", local.data(), static_cast<DWORD>(local.size())) == 0) {
				return std::nullopt;
			}
			return fs::path(local.data()) / L"ModOrganizer" / name.data();
		}

		// Its folder of mods and the folder of the profile it runs, from ModOrganizer.ini. A path there may start with
		// the base folder of the instance
		std::optional<std::pair<fs::path, fs::path>> FoldersOf(const fs::path& a_instance)
		{
			std::ifstream file(a_instance / "ModOrganizer.ini");
			if (!file) {
				return std::nullopt;
			}
			std::string profile;
			std::string base;
			std::string modsAt;
			std::string profilesAt;
			for (std::string line; std::getline(file, line);) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const auto equals = line.find('=');
				if (equals == std::string::npos) {
					continue;
				}
				const auto key = line.substr(0, equals);
				auto value = line.substr(equals + 1);
				if (key == "selected_profile") {
					profile = std::move(value);
				} else if (key == "base_directory") {
					base = std::move(value);
				} else if (key == "mod_directory") {
					modsAt = std::move(value);
				} else if (key == "profiles_directory") {
					profilesAt = std::move(value);
				}
			}
			// "@ByteArray(Default)"
			if (profile.starts_with("@ByteArray(") && profile.ends_with(')')) {
				profile = profile.substr(11, profile.size() - 12);
			}
			if (profile.empty()) {
				return std::nullopt;
			}
			const auto unescaped = [](std::string a_text) {
				for (auto at = a_text.find("\\\\"); at != std::string::npos; at = a_text.find("\\\\", at + 1)) {
					a_text.erase(at, 1);
				}
				return a_text;
			};
			const fs::path home = base.empty() ? a_instance : Wide(unescaped(base));
			const auto folder = [&](const std::string& a_told, const char* a_default) {
				auto text = unescaped(a_told);
				if (const auto at = text.find("%BASE_DIR%"); at != std::string::npos) {
					return Wide(Narrow(home) + text.substr(at + 10));
				}
				return text.empty() ? home / a_default : Wide(text);
			};
			return std::pair{ folder(modsAt, "mods"), folder(profilesAt, "profiles") / Wide(profile) };
		}

		// Every mod the profile has on, read whole unless something not a texture turns up
		void ReadModOrganizer(std::unordered_set<std::string>& a_seen)
		{
			const auto instance = ModOrganizerFolder();
			const auto folders = instance ? FoldersOf(*instance) : std::nullopt;
			if (!folders) {
				return;
			}
			const auto& [modsFolder, profile] = *folders;
			std::ifstream list(profile / "modlist.txt");
			std::size_t on = 0;
			const auto before = mods.size();
			for (std::string line; std::getline(list, line);) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				if (line.size() < 2 || line.front() != '+') {
					continue;
				}
				on++;
				const auto name = line.substr(1);
				const auto folder = modsFolder / Wide(name);
				Sorted sorted;
				std::error_code failed;
				for (fs::recursive_directory_iterator at(folder, fs::directory_options::skip_permission_denied, failed), end; !failed && at != end && !sorted.other; at.increment(failed)) {
					if (at->is_regular_file(failed)) {
						Sort(sorted, Plain(Narrow(at->path().lexically_relative(folder))), at->path());
					}
				}
				for (const auto& plugin : sorted.plugins) {
					a_seen.insert(Plain(Narrow(plugin.filename())));
				}
				if (OnlyTextures(sorted)) {
					AddMod(name, kModOrganizer, sorted.textures);
				}
			}
			logger::info("texture mods: {} of {} mods on in the profile {} of Mod Organizer", mods.size() - before, on, Narrow(profile));
		}

		// a string of JSON from its opening quote on, its escapes undone
		std::string JsonString(std::string_view a_text, std::size_t a_at)
		{
			std::string text;
			for (auto at = a_at + 1; at < a_text.size() && a_text[at] != '"'; ++at) {
				if (a_text[at] == '\\' && at + 1 < a_text.size()) {
					at++;
					text.push_back(a_text[at] == 'n' ? '\n' : a_text[at] == 't' ? '\t' : a_text[at]);
					if (a_text[at] == 'u') {
						text.back() = '?';
						at += 4;
					}
					continue;
				}
				text.push_back(a_text[at]);
			}
			return text;
		}

		// the value of a key of the object around a_at, "relPath" of {"relPath": "...", "source": "..."}
		std::string Field(std::string_view a_text, std::size_t a_from, std::size_t a_to, std::string_view a_key)
		{
			const auto key = a_text.substr(a_from, a_to - a_from).find(std::format("\"{}\"", a_key));
			if (key == std::string_view::npos) {
				return {};
			}
			const auto quote = a_text.find('"', a_text.find(':', a_from + key + a_key.size() + 2) + 1);
			return quote < a_to ? JsonString(a_text, quote) : std::string();
		}

		// Vortex leaves what it deployed into Data in vortex.deployment.json, every file with the mod it comes from
		void ReadVortex(std::unordered_set<std::string>& a_seen)
		{
			std::ifstream file("Data/vortex.deployment.json", std::ios::binary);
			if (!file) {
				return;
			}
			const std::string text{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
			std::map<std::string, Sorted> bySource;
			for (auto at = text.find("\"relPath\""); at != std::string::npos; at = text.find("\"relPath\"", at + 1)) {
				const auto from = text.rfind('{', at);
				const auto to = text.find('}', at);
				if (from == std::string::npos || to == std::string::npos) {
					break;
				}
				const auto path = Plain(Field(text, from, to, "relPath"));
				const auto source = Field(text, from, to, "source");
				if (!path.empty() && !source.empty()) {
					Sort(bySource[source], path, fs::path("Data") / Wide(path));
				}
			}
			const auto before = mods.size();
			for (auto& [source, sorted] : bySource) {
				for (const auto& plugin : sorted.plugins) {
					a_seen.insert(Plain(Narrow(plugin.filename())));
				}
				if (OnlyTextures(sorted)) {
					AddMod(source, kVortex, sorted.textures);
				}
			}
			logger::info("texture mods: {} of {} mods Vortex deployed", mods.size() - before, bySource.size());
		}

		// A plugin with no records put in by hand loads its archives, "Name.bsa" and "Name - Textures.bsa"
		void ReadArchives(const std::vector<std::string>& a_plugins, const std::unordered_set<std::string>& a_seen)
		{
			const auto before = mods.size();
			for (const auto& plugin : a_plugins) {
				const auto data = fs::path("Data");
				if (a_seen.contains(Plain(plugin)) || !Empty(data / Wide(plugin))) {
					continue;
				}
				const auto stem = plugin.substr(0, plugin.rfind('.'));
				Sorted sorted;
				std::error_code failed;
				for (const auto* ending : { ".bsa", " - Textures.bsa" }) {
					if (const auto archive = data / Wide(stem + ending); fs::exists(archive, failed)) {
						sorted.archives.push_back(archive);
					}
				}
				if (!sorted.archives.empty() && OnlyTextures(sorted)) {
					AddMod(stem, kArchive, sorted.textures);
				}
			}
			logger::info("texture mods: {} archives of plugins with no records", mods.size() - before);
		}

		void Read(const std::vector<std::string>& a_plugins)
		{
			const auto start = Clock::now();
			try {
				std::unordered_set<std::string> seen;
				ReadModOrganizer(seen);
				ReadVortex(seen);
				ReadArchives(a_plugins, seen);
			} catch (const std::exception& a_error) {
				logger::warn("texture mods: reading stopped, {}", a_error.what());
			}
			logger::info("texture mods: {} found with {} textures in {:.0f} ms", mods.size(), owners.size(),
				std::chrono::duration<double, std::milli>(Clock::now() - start).count());
			ready = true;
		}

		// a file of the game, loose or in an archive
		std::string ReadGameFile(const std::string& a_path)
		{
			RE::BSResourceNiBinaryStream stream(a_path);
			std::string bytes;
			if (!stream.good()) {
				return bytes;
			}
			std::array<char, 16384> chunk{};
			for (bool full = true; full;) {
				const auto before = stream.tell();
				full = stream.read(chunk.data(), static_cast<std::uint32_t>(chunk.size()));
				const auto got = stream.tell() - before;
				bytes.append(chunk.data(), got);
				full = full && got > 0;
			}
			return bytes;
		}

		// The textures a model names are every path that ends in .dds among its bytes
		std::vector<std::string> TexturesNamed(std::string_view a_bytes)
		{
			std::vector<std::string> found;
			const auto allowed = [](char a_letter) {
				const auto letter = static_cast<unsigned char>(a_letter);
				return std::isalnum(letter) || letter == '_' || letter == '\\' || letter == '/' || letter == ' ' || letter == '.' || letter == '-';
			};
			for (std::size_t at = a_bytes.find('.'); at != std::string_view::npos && at + 4 <= a_bytes.size(); at = a_bytes.find('.', at + 1)) {
				if (_strnicmp(a_bytes.data() + at, ".dds", 4) != 0) {
					continue;
				}
				auto start = at;
				while (start > 0 && at - start < kLongestPath && allowed(a_bytes[start - 1])) {
					start--;
				}
				if (auto path = start < at ? TexturePath(a_bytes.substr(start, at + 4 - start)) : std::string(); !path.empty() && !Shared(path)) {
					found.push_back(std::move(path));
				}
			}
			return found;
		}

		void Merge(std::vector<std::uint16_t>& a_into, const std::vector<std::uint16_t>& a_more)
		{
			for (const auto mod : a_more) {
				if (const auto at = std::ranges::lower_bound(a_into, mod); at == a_into.end() || *at != mod) {
					a_into.insert(at, mod);
				}
			}
		}

		void MergeTexture(std::vector<std::uint16_t>& a_into, std::string_view a_texture)
		{
			const auto path = TexturePath(a_texture);
			if (const auto found = Shared(path) ? owners.end() : owners.find(path); found != owners.end()) {
				Merge(a_into, found->second);
			}
		}

		const std::vector<std::uint16_t>& ChangersOfModel(std::string_view a_model)
		{
			auto key = Plain(a_model);
			if (const auto known = modelChangers.find(key); known != modelChangers.end()) {
				return known->second;
			}
			std::vector<std::uint16_t> changers;
			for (const auto& texture : TexturesNamed(ReadGameFile(key.starts_with("meshes\\") ? key : "meshes\\" + key))) {
				if (const auto found = owners.find(texture); found != owners.end()) {
					Merge(changers, found->second);
				}
			}
			return modelChangers.emplace(std::move(key), std::move(changers)).first->second;
		}

		// A model and the texture sets that take the place of its textures
		void AddModel(std::vector<std::uint16_t>& a_changers, const RE::TESModel* a_model)
		{
			const char* path = a_model ? a_model->GetModel() : nullptr;
			if (!path || !*path) {
				return;
			}
			Merge(a_changers, ChangersOfModel(path));
			const auto* swap = skyrim_cast<const RE::TESModelTextureSwap*>(a_model);
			for (std::uint32_t i = 0; swap && swap->alternateTextures && i < swap->numAlternateTextures; ++i) {
				if (const auto* set = swap->alternateTextures[i].textureSet) {
					for (const auto& texture : set->textures) {
						if (!texture.textureName.empty()) {
							MergeTexture(a_changers, texture.textureName.c_str());
						}
					}
				}
			}
		}

		// the models of a thing are what lies on the ground and what an armor looks like worn
		std::vector<std::uint16_t> ReadLooks(const BrowserIndex::Item& a_item)
		{
			std::vector<std::uint16_t> changers;
			if (const auto* armor = a_item.form->As<RE::TESObjectARMO>()) {
				for (const auto& model : armor->worldModels) {
					AddModel(changers, &model);
				}
				for (const auto* addon : armor->armorAddons) {
					if (!addon) {
						continue;
					}
					for (const auto& model : addon->bipedModels) {
						AddModel(changers, &model);
					}
				}
			} else {
				AddModel(changers, a_item.form->As<RE::TESModel>());
			}
			return changers;
		}

		bool HasLooks(const BrowserIndex::Item& a_item)
		{
			using Family = BrowserIndex::Family;
			return a_item.unit == BrowserIndex::kNone && a_item.family != Family::kQuests && a_item.family != Family::kPlaces && a_item.family != Family::kPseudo;
		}

		const std::vector<std::uint16_t>* Known(const BrowserIndex::Item& a_item)
		{
			const auto place = BrowserIndex::PlaceOf(a_item);
			if (place >= itemRead.size() || !itemRead[place]) {
				return nullptr;
			}
			static const std::vector<std::uint16_t> none;
			const auto found = itemChangers.find(place);
			return found != itemChangers.end() ? &found->second : &none;
		}
	}

	void Start()
	{
		if (started.exchange(true)) {
			return;
		}
		std::vector<std::string> plugins;
		for (const auto* file : RE::TESDataHandler::GetSingleton()->files) {
			if (file && file->compileIndex != 0xFF) {
				plugins.emplace_back(file->GetFilename());
			}
		}
		itemRead.assign(BrowserIndex::Items().size(), 0);
		Trail::Step("texture mods, read on a thread of their own");
		std::thread([plugins = std::move(plugins)] { Read(plugins); }).detach();
	}

	bool Ready()
	{
		return ready;
	}

	std::span<const Mod> Mods()
	{
		return ready ? std::span<const Mod>(mods) : std::span<const Mod>();
	}

	std::optional<std::uint16_t> ModNamed(std::string_view a_name)
	{
		for (std::size_t i = 0; ready && i < mods.size(); ++i) {
			if (_stricmp(mods[i].name.c_str(), std::string(a_name).c_str()) == 0) {
				return static_cast<std::uint16_t>(i);
			}
		}
		return std::nullopt;
	}

	std::vector<std::uint16_t> ChangersOf(const BrowserIndex::Item& a_item)
	{
		if (!ready || !HasLooks(a_item)) {
			return {};
		}
		if (const auto* known = Known(a_item)) {
			return *known;
		}
		const auto place = BrowserIndex::PlaceOf(a_item);
		auto changers = ReadLooks(a_item);
		itemRead[place] = 1;
		if (!changers.empty()) {
			itemChangers[place] = changers;
		}
		return changers;
	}

	bool Changes(std::uint16_t a_mod, const BrowserIndex::Item& a_item)
	{
		const auto* known = ready ? Known(a_item) : nullptr;
		return known && std::ranges::binary_search(*known, a_mod);
	}

	void WantAll()
	{
		Start();
		wantAll = true;
	}

	std::pair<std::size_t, std::size_t> Progress()
	{
		return { nextItem, BrowserIndex::Items().size() };
	}

	void Frame()
	{
		if (!ready || !wantAll) {
			return;
		}
		const auto& items = BrowserIndex::Items();
		if (nextItem >= items.size()) {
			return;
		}
		const auto start = Clock::now();
		while (nextItem < items.size() && Clock::now() - start < kFrameBudget) {
			ChangersOf(items[nextItem]);
			nextItem++;
		}
		if (nextItem == items.size()) {
			changedBy.assign(mods.size(), 0);
			for (const auto& [place, changers] : itemChangers) {
				for (const auto mod : changers) {
					changedBy[mod]++;
				}
			}
			logger::info("texture mods: the models of {} things read, {} things have a texture a mod changes, {} models", items.size(), itemChangers.size(), modelChangers.size());
		}
	}

	std::optional<std::size_t> ChangedBy(std::uint16_t a_mod)
	{
		return a_mod < changedBy.size() ? std::optional(changedBy[a_mod]) : std::nullopt;
	}
}
