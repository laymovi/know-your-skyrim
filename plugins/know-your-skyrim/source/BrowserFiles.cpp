#include "PCH.h"

#include "BrowserFiles.h"

#include <thread>

#include "GameArchives.h"
#include "PluginSources.h"

namespace BrowserFiles
{
	namespace
	{
		constexpr std::size_t kHeader = 24;
		constexpr std::uint32_t kCompressed = 0x00040000;
		constexpr std::uint32_t kDeleted = 0x20;
		constexpr std::uint32_t kDisabled = 0x800;

		enum GroupType : std::int32_t
		{
			kTop = 0,
			kWorldChildren = 1,
			kCellChildren = 6,
			kTopicChildren = 7
		};

		constexpr std::uint32_t Tag(const char (&a_tag)[5])
		{
			return static_cast<std::uint32_t>(a_tag[0]) | static_cast<std::uint32_t>(a_tag[1]) << 8 | static_cast<std::uint32_t>(a_tag[2]) << 16 |
			       static_cast<std::uint32_t>(a_tag[3]) << 24;
		}

		constexpr auto kGroup = Tag("GRUP");
		constexpr auto kCell = Tag("CELL");
		constexpr auto kWorld = Tag("WRLD");
		constexpr auto kReference = Tag("REFR");
		constexpr auto kCharacter = Tag("ACHR");
		constexpr auto kName = Tag("NAME");
		constexpr auto kData = Tag("DATA");
		constexpr auto kLong = Tag("XXXX");

		using PluginSources::Source;

		struct State
		{
			std::vector<Source> sources;
			std::unordered_set<RE::FormID> wanted;
			std::unordered_map<RE::FormID, Spot> byReference;
			std::unordered_map<RE::FormID, std::vector<Spot>> byBase;
			// made on the first ask for a single reference, few ever ask
			std::optional<std::unordered_map<RE::FormID, const Spot*>> spots;
			std::size_t packed = 0;
			std::size_t unread = 0;
			std::atomic<bool> ready = false;
		};

		// the thread may still read when the game quits, so the state is never freed
		State* state = nullptr;

		template <class T>
		T Read(const std::byte* a_at)
		{
			T value;
			std::memcpy(&value, a_at, sizeof(T));
			return value;
		}

		struct Place
		{
			RE::FormID world = 0;
			RE::FormID cell = 0;
		};

		class Walker
		{
		public:
			Walker(State& a_state, const Source& a_source, const std::byte* a_data, std::size_t a_size) :
				state(a_state), source(a_source), data(a_data), size(a_size) {}

			void Walk()
			{
				std::size_t at = 0;
				while (at + kHeader <= size) {
					const auto type = Read<std::uint32_t>(data + at);
					const auto length = Read<std::uint32_t>(data + at + 4);
					if (type != kGroup) {
						at += kHeader + length;
						continue;
					}
					if (length < kHeader || at + length > size) {
						return;
					}
					const auto label = Read<std::uint32_t>(data + at + 8);
					if (label == kCell || label == kWorld) {
						Group(at + kHeader, at + length, {});
					}
					at += length;
				}
			}

		private:
			void Group(std::size_t a_from, std::size_t a_to, Place a_place)
			{
				auto at = a_from;
				while (at + kHeader <= a_to) {
					const auto type = Read<std::uint32_t>(data + at);
					const auto length = Read<std::uint32_t>(data + at + 4);
					if (type == kGroup) {
						if (length < kHeader || at + length > a_to) {
							return;
						}
						const auto label = Read<std::uint32_t>(data + at + 8);
						const auto kind = Read<std::int32_t>(data + at + 12);
						auto inner = a_place;
						if (kind == kWorldChildren) {
							inner.world = source.Runtime(label);
						} else if (kind == kCellChildren) {
							inner.cell = source.Runtime(label);
						}
						if (kind != kTopicChildren) {
							Group(at + kHeader, at + length, inner);
						}
						at += length;
						continue;
					}
					if (at + kHeader + length > a_to) {
						return;
					}
					if ((type == kReference || type == kCharacter) && a_place.cell != 0) {
						Reference(at, length, a_place);
					}
					at += kHeader + length;
				}
			}

			void Reference(std::size_t a_at, std::uint32_t a_length, Place a_place)
			{
				const auto flags = Read<std::uint32_t>(data + a_at + 8);
				const auto id = source.Runtime(Read<std::uint32_t>(data + a_at + 12));
				if (flags & kCompressed) {
					state.packed++;
					return;
				}
				std::optional<RE::FormID> base;
				std::optional<RE::NiPoint3> position;
				auto at = a_at + kHeader;
				const auto end = at + a_length;
				std::uint32_t longer = 0;
				while (at + 6 <= end) {
					const auto type = Read<std::uint32_t>(data + at);
					const std::uint32_t length = longer ? longer : Read<std::uint16_t>(data + at + 4);
					longer = 0;
					if (type == kLong && at + 10 <= end) {
						longer = Read<std::uint32_t>(data + at + 6);
						at += 10;
						continue;
					}
					if (at + 6 + length > end) {
						break;
					}
					if (type == kName && length >= 4) {
						base = source.Runtime(Read<std::uint32_t>(data + at + 6));
					} else if (type == kData && length >= 12) {
						position = RE::NiPoint3{ Read<float>(data + at + 6), Read<float>(data + at + 10), Read<float>(data + at + 14) };
					}
					at += 6 + length;
				}
				const auto known = state.byReference.find(id);
				if ((flags & kDeleted) || !base || !position || !state.wanted.contains(*base)) {
					if (known != state.byReference.end()) {
						state.byReference.erase(known);
					}
					return;
				}
				// a world's references lie in its cell that lasts, the cell outside is told by the place
				const bool inside = a_place.world == 0;
				state.byReference[id] = { id, *base, a_place.cell, inside ? 0 : a_place.world, *position, (flags & kDisabled) != 0 };
			}

			State& state;
			const Source& source;
			const std::byte* data;
			std::size_t size;
		};

		void ReadFile(State& a_state, const Source& a_source)
		{
			std::error_code error;
			const auto size = std::filesystem::file_size(a_source.path, error);
			if (error || size < kHeader) {
				a_state.unread++;
				return;
			}
			auto* file = ::CreateFileW(a_source.path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
			if (file == INVALID_HANDLE_VALUE) {
				a_state.unread++;
				return;
			}
			auto* mapping = ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
			const auto* view = mapping ? static_cast<const std::byte*>(::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0)) : nullptr;
			if (view) {
				Walker(a_state, a_source, view, static_cast<std::size_t>(size)).Walk();
				::UnmapViewOfFile(view);
			} else {
				a_state.unread++;
			}
			if (mapping) {
				::CloseHandle(mapping);
			}
			::CloseHandle(file);
		}

		void Run(State* a_state)
		{
			const auto start = std::chrono::steady_clock::now();
			for (const auto& source : a_state->sources) {
				logger::info("step: reading the plugin file {}", GameArchives::Narrow(source.path));
				ReadFile(*a_state, source);
			}
			for (const auto& [id, spot] : a_state->byReference) {
				a_state->byBase[spot.base].push_back(spot);
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("the plugin files place {} references of {} things the browser knows, {} files read in {:.0f} ms, {} packed references left out, {} files unread",
				a_state->byReference.size(), a_state->byBase.size(), a_state->sources.size() - a_state->unread, took, a_state->packed, a_state->unread);
			a_state->byReference.clear();
			a_state->ready = true;
		}
	}

	void Start(std::unordered_set<RE::FormID> a_wanted)
	{
		if (state) {
			return;
		}
		state = new State();
		state->wanted = std::move(a_wanted);
		state->sources = PluginSources::LoadOrder();
		logger::info("step: {} plugin files are read in the background", state->sources.size());
		std::thread(Run, state).detach();
	}

	bool Started()
	{
		return state != nullptr;
	}

	bool Ready()
	{
		return state && state->ready;
	}

	std::span<const Spot> SpotsOf(RE::FormID a_base)
	{
		if (!Ready()) {
			return {};
		}
		const auto found = state->byBase.find(a_base);
		return found != state->byBase.end() ? std::span<const Spot>(found->second) : std::span<const Spot>();
	}

	const Spot* SpotOfReference(RE::FormID a_reference)
	{
		if (!Ready()) {
			return nullptr;
		}
		if (!state->spots) {
			auto& spots = state->spots.emplace();
			for (const auto& [base, list] : state->byBase) {
				for (const auto& spot : list) {
					spots.emplace(spot.reference, &spot);
				}
			}
		}
		const auto found = state->spots->find(a_reference);
		return found != state->spots->end() ? found->second : nullptr;
	}
}
