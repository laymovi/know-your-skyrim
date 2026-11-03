#include "PCH.h"

#include "Knowledge.h"

#include "BrowserFavorites.h"
#include "BrowserQuests.h"
#include "Holders.h"
#include "Options.h"
#include "Places.h"
#include "Profile.h"

namespace Knowledge
{
	namespace
	{
		// the record of the save's SKSE file holds the form ids of everything met
		constexpr std::uint32_t kOwner = 'KYSK';
		constexpr std::uint32_t kSeen = 'SEEN';
		constexpr std::uint32_t kSeenVersion = 1;
		constexpr RE::FormID kPlayer = 0x14;
		// the forms the game makes while it runs, a leveled actor's copy of its NPC, go when it's reset
		constexpr RE::FormID kMadeAtRuntime = 0xFF000000;
		// a quest in the journal: going or done, as BrowserQuests counts it
		constexpr int kGoing = 2;
		constexpr int kDone = 3;

		std::unordered_set<RE::FormID> seen;
		std::unordered_set<RE::FormID> craftable;
		// the marker of the map nearest to each place
		std::unordered_map<const RE::BGSLocation*, RE::FormID> markers;
		// the places the character found as they were counted at version placesCounted, with the places around them
		std::unordered_set<RE::FormID> knownPlaces;
		std::mutex lock;
		std::atomic<std::uint32_t> version{ 0 };
		std::uint32_t placesCounted = 0xFFFFFFFF;
		// the save had a record of its own
		bool recorded = false;

		void Add(RE::FormID a_form)
		{
			if (a_form == 0 || a_form >= kMadeAtRuntime) {
				return;
			}
			const std::scoped_lock guard{ lock };
			if (seen.insert(a_form).second) {
				version++;
			}
		}

		bool Seen(RE::FormID a_form)
		{
			const std::scoped_lock guard{ lock };
			return seen.contains(a_form);
		}

		// what a reference stands for is its base, and for a leveled actor the NPCs its copy was made from
		void AddReference(const RE::TESObjectREFR* a_reference)
		{
			const auto* base = a_reference ? a_reference->GetBaseObject() : nullptr;
			if (!base) {
				return;
			}
			Add(base->GetFormID());
			if (const auto* leveled = a_reference->extraList.GetByType<RE::ExtraLeveledCreature>()) {
				Add(leveled->originalBase ? leveled->originalBase->GetFormID() : 0);
				Add(leveled->templateBase ? leveled->templateBase->GetFormID() : 0);
			}
		}

		std::size_t AddInventory(RE::TESObjectREFR* a_holder)
		{
			std::size_t added = 0;
			if (!a_holder) {
				return added;
			}
			for (const auto& [thing, held] : a_holder->GetInventory()) {
				if (thing && held.first > 0) {
					Add(thing->GetFormID());
					added++;
				}
			}
			return added;
		}

		// being inside a place is being in every place around it too
		void AddPlace(const RE::BGSLocation* a_place)
		{
			for (int depth = 0; a_place && depth < 8; ++depth, a_place = a_place->parentLoc) {
				Add(a_place->GetFormID());
			}
		}

		// what a merchant sells lies in the chest of a faction they trade for
		void AddStock(RE::TESObjectREFR* a_merchant)
		{
			const auto* actor = a_merchant ? a_merchant->As<RE::Actor>() : nullptr;
			const auto* person = actor ? actor->GetActorBase() : nullptr;
			if (!person) {
				return;
			}
			for (const auto& rank : person->factions) {
				if (rank.faction && rank.faction->IsVendor()) {
					AddInventory(rank.faction->vendorData.merchantContainer);
				}
			}
		}

		// The game's own question whether someone ever spoke to the player, the flag lives on the person in the save
		bool TalkedTo(RE::Actor& a_actor)
		{
			RE::TESConditionItem asked;
			asked.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetTalkedToPC;
			asked.data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
			asked.data.comparisonValue.f = 1.0F;
			asked.data.object = RE::CONDITIONITEMOBJECT::kSelf;
			RE::ConditionCheckParams params(&a_actor, nullptr);
			return asked(params);
		}

		// A save with no record of its own takes what the player carries and the people who spoke to the player or
		// follow. The people are taken out of the forms under their lock, the game's question is asked after it
		void Seed()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}
			const auto start = std::chrono::steady_clock::now();
			const auto carried = AddInventory(player);
			std::vector<RE::Actor*> actors;
			{
				const auto& [forms, formsLock] = RE::TESForm::GetAllForms();
				const RE::BSReadLockGuard guard{ formsLock };
				for (const auto& [id, form] : *forms) {
					if (auto* actor = form ? form->As<RE::Actor>() : nullptr; actor && actor != player) {
						actors.push_back(actor);
					}
				}
			}
			std::size_t met = 0;
			for (auto* actor : actors) {
				if (actor->IsPlayerTeammate() || TalkedTo(*actor)) {
					AddReference(actor);
					met++;
				}
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			logger::info("knowledge: a save with no record of its own, {} things carried, {} of {} people met, {:.1f} ms", carried, met, actors.size(), took);
		}

		// the time the events take, the plugin's log tells it with the rest of its work each time the inventory closes
		std::size_t Work()
		{
			static const auto work = Profile::Work("knowledge events");
			return work;
		}

		class Sinks :
			public RE::BSTEventSink<RE::TESContainerChangedEvent>,
			public RE::BSTEventSink<RE::TESDeathEvent>,
			public RE::BSTEventSink<RE::TESCombatEvent>,
			public RE::BSTEventSink<RE::TESActorLocationChangeEvent>,
			public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
			public RE::BSTEventSink<SKSE::CrosshairRefEvent>
		{
		public:
			using Control = RE::BSEventNotifyControl;

			Control ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (a_event && a_event->newContainer == kPlayer) {
					Add(a_event->baseObj);
				}
				return Control::kContinue;
			}

			Control ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (a_event && a_event->actorKiller && a_event->actorKiller->IsPlayerRef()) {
					AddReference(a_event->actorDying.get());
				}
				return Control::kContinue;
			}

			Control ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (!a_event) {
					return Control::kContinue;
				}
				const auto* actor = a_event->actor.get();
				const auto* target = a_event->targetActor.get();
				if (target && target->IsPlayerRef()) {
					AddReference(actor);
				} else if (actor && actor->IsPlayerRef()) {
					AddReference(target);
				}
				return Control::kContinue;
			}

			Control ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (a_event && a_event->actor && a_event->actor->IsPlayerRef()) {
					AddPlace(a_event->newLoc);
				}
				return Control::kContinue;
			}

			// the HUD names what the crosshair is on, the player has seen it
			Control ProcessEvent(const SKSE::CrosshairRefEvent* a_event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (a_event && a_event->crosshairRef) {
					AddReference(a_event->crosshairRef.get());
				}
				return Control::kContinue;
			}

			// a chest or a follower's things, a merchant's stock, the one the player speaks to
			Control ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				const Profile::Timer timer{ Work() };
				if (!a_event || !a_event->opening) {
					return Control::kContinue;
				}
				if (a_event->menuName == RE::ContainerMenu::MENU_NAME) {
					const auto holder = RE::TESObjectREFR::LookupByHandle(RE::ContainerMenu::GetTargetRefHandle());
					AddReference(holder.get());
					AddInventory(holder.get());
				} else if (a_event->menuName == RE::BarterMenu::MENU_NAME) {
					const auto merchant = RE::TESObjectREFR::LookupByHandle(RE::BarterMenu::GetTargetRefHandle());
					AddReference(merchant.get());
					AddStock(merchant.get());
				} else if (a_event->menuName == RE::DialogueMenu::MENU_NAME) {
					if (const auto* topics = RE::MenuTopicManager::GetSingleton()) {
						AddReference(topics->speaker.get().get());
					}
				}
				return Control::kContinue;
			}
		};

		void Save(SKSE::SerializationInterface* a_save)
		{
			const std::scoped_lock guard{ lock };
			if (!a_save->OpenRecord(kSeen, kSeenVersion)) {
				logger::warn("knowledge: the save's SKSE file took no record");
				return;
			}
			const auto count = static_cast<std::uint32_t>(seen.size());
			a_save->WriteRecordData(count);
			for (const auto form : seen) {
				a_save->WriteRecordData(form);
			}
			logger::info("knowledge: {} things met kept in the save", count);
		}

		// a form of a plugin taken out of the load order since is left behind
		void Load(SKSE::SerializationInterface* a_load)
		{
			std::uint32_t type = 0;
			std::uint32_t recordVersion = 0;
			std::uint32_t length = 0;
			while (a_load->GetNextRecordInfo(type, recordVersion, length)) {
				if (type != kSeen || recordVersion != kSeenVersion) {
					continue;
				}
				std::uint32_t count = 0;
				a_load->ReadRecordData(count);
				std::unordered_set<RE::FormID> read;
				read.reserve(count);
				for (std::uint32_t i = 0; i < count; ++i) {
					RE::FormID form = 0;
					RE::FormID now = 0;
					if (a_load->ReadRecordData(form) != sizeof(form)) {
						break;
					}
					if (a_load->ResolveFormID(form, now)) {
						read.insert(now);
					}
				}
				{
					const std::scoped_lock guard{ lock };
					seen = std::move(read);
				}
				recorded = true;
				version++;
				logger::info("knowledge: {} things met read from the save, {} of them of plugins this load order lacks", seen.size(), count - seen.size());
			}
		}

		void Revert(SKSE::SerializationInterface*)
		{
			const std::scoped_lock guard{ lock };
			seen.clear();
			recorded = false;
			version++;
		}

		bool MarkerFound(RE::FormID a_marker)
		{
			const auto* marker = a_marker != 0 ? RE::TESForm::LookupByID<RE::TESObjectREFR>(a_marker) : nullptr;
			const auto* extra = marker ? marker->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
			return extra && extra->mapData && extra->mapData->flags.any(RE::MapMarkerData::Flag::kVisible);
		}

		// markers don't move, each place's nearest is found once
		RE::FormID MarkerNear(const RE::BGSLocation& a_place)
		{
			const auto [at, fresh] = markers.try_emplace(&a_place, 0);
			if (fresh) {
				at->second = Places::MarkerOf(&a_place);
			}
			return at->second;
		}

		// A place is found when the player was in it, cleared it or found the marker of the map nearest to it. A shop
		// of a town shares the town's marker, so a found Whiterun shows Belethor's General Goods, and a dungeon has a
		// marker of its own. The places around a place found are found too, a hold with any of its places. Counted
		// again once anything more is met or the inventory opens
		void CountPlaces()
		{
			if (placesCounted == version) {
				return;
			}
			placesCounted = version;
			knownPlaces.clear();
			for (const auto& item : BrowserIndex::Items()) {
				const auto* place = item.family == BrowserIndex::Family::kPlaces ? item.form->As<RE::BGSLocation>() : nullptr;
				if (!place || knownPlaces.contains(place->GetFormID())) {
					continue;
				}
				if (Seen(place->GetFormID()) || place->everCleared || MarkerFound(MarkerNear(*place))) {
					for (int depth = 0; place && depth < 8; ++depth, place = place->parentLoc) {
						knownPlaces.insert(place->GetFormID());
					}
				}
			}
		}

		bool PlaceKnown(const RE::BGSLocation* a_place)
		{
			CountPlaces();
			return a_place && knownPlaces.contains(a_place->GetFormID());
		}

		bool LivingKnown(const BrowserIndex::Item& a_item)
		{
			if (a_item.unit == BrowserIndex::kNone) {
				return Seen(a_item.form->GetFormID());
			}
			for (const auto* person : BrowserIndex::Units()[a_item.unit]) {
				if (person && Seen(person->GetFormID())) {
					return true;
				}
			}
			return false;
		}

		bool ThingKnown(const RE::TESForm& a_thing)
		{
			const auto id = a_thing.GetFormID();
			if (Seen(id) || craftable.contains(id)) {
				return true;
			}
			const auto* book = a_thing.As<RE::TESObjectBOOK>();
			if (!book) {
				return false;
			}
			if (book->IsRead()) {
				return true;
			}
			auto* spell = book->TeachesSpell() ? const_cast<RE::TESObjectBOOK*>(book)->GetSpell() : nullptr;
			const auto* player = RE::PlayerCharacter::GetSingleton();
			return spell && player && player->HasSpell(spell);
		}

		Sinks sinks;
	}

	bool On()
	{
		return Options::Progression();
	}

	bool Known(const BrowserIndex::Item& a_item)
	{
		using Family = BrowserIndex::Family;
		if (!On()) {
			return true;
		}
		const auto* form = a_item.form;
		if (!form) {
			return false;
		}
		if (BrowserFavorites::Has(form->GetFormID())) {
			return true;
		}
		switch (a_item.family) {
		case Family::kPerks:
			return true;
		case Family::kPseudo:
			return false;
		case Family::kQuests:
			{
				const auto* quest = form->As<RE::TESQuest>();
				const auto state = quest ? BrowserQuests::StateOf(*quest) : 0;
				return state == kGoing || state == kDone;
			}
		case Family::kPlaces:
			return PlaceKnown(form->As<RE::BGSLocation>());
		case Family::kLiving:
			return LivingKnown(a_item);
		default:
			return ThingKnown(*form);
		}
	}

	std::uint32_t Version()
	{
		return version;
	}

	void Listen()
	{
		auto* events = RE::ScriptEventSourceHolder::GetSingleton();
		events->AddEventSink<RE::TESContainerChangedEvent>(&sinks);
		events->AddEventSink<RE::TESDeathEvent>(&sinks);
		events->AddEventSink<RE::TESCombatEvent>(&sinks);
		events->AddEventSink<RE::TESActorLocationChangeEvent>(&sinks);
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(&sinks);
		SKSE::GetCrosshairRefEventSource()->AddEventSink(&sinks);
	}

	void Register()
	{
		const auto* serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID(kOwner);
		serialization->SetSaveCallback(Save);
		serialization->SetLoadCallback(Load);
		serialization->SetRevertCallback(Revert);
	}

	void Loaded()
	{
		if (!recorded) {
			Seed();
			recorded = true;
		}
	}

	void MenuOpened()
	{
		if (!On()) {
			return;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		const auto start = std::chrono::steady_clock::now();
		std::unordered_set<RE::FormID> could;
		std::size_t recipes = 0;
		for (auto* recipe : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>()) {
			const auto* made = recipe ? recipe->createdItem : nullptr;
			if (!made || Holders::Improves(*recipe)) {
				continue;
			}
			recipes++;
			if (!recipe->conditions.head || recipe->conditions(player, player)) {
				could.insert(made->GetFormID());
			}
		}
		// the markers of the map found in the world meanwhile count the places anew, and the rows with them
		craftable = std::move(could);
		version++;
		const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		logger::info("knowledge: {} of {} recipes the player could make, {:.2f} ms", craftable.size(), recipes, took);
	}
}
