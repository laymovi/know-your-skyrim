#include "PCH.h"

#include "BrowserQuests.h"

#include "Profile.h"
#include "Trail.h"

namespace BrowserQuests
{
	namespace
	{
		// a script can hold arrays of hundreds of markers, a quest names its places from this many references
		constexpr std::size_t kMostReferences = 24;

		std::unordered_map<const RE::TESQuest*, Ties> ties;
		std::unordered_map<RE::FormID, std::vector<const RE::TESQuest*>> byForm;
		bool read = false;
		const Ties none;

		bool Named(const RE::TESForm* a_form)
		{
			const char* name = a_form ? a_form->GetName() : nullptr;
			return name && *name;
		}

		template <class T>
		void AddOnce(std::vector<T>& a_list, T a_one)
		{
			if (std::ranges::find(a_list, a_one) == a_list.end()) {
				a_list.push_back(a_one);
			}
		}

		// The base a reference stands on. A leveled actor stands on a copy the game makes for it and frees again when
		// the actor is reset, the record it was made from lasts. A kept copy of a dragon of A Blade in the Dark was
		// read after the stage that brings the dragon anew and the game crashed
		const RE::TESForm* LastingBase(const RE::TESObjectREFR& a_reference)
		{
			const auto* base = a_reference.GetBaseObject();
			if (base && base->IsDynamicForm()) {
				const auto* leveled = a_reference.extraList.GetByType<RE::ExtraLeveledCreature>();
				return leveled ? leveled->originalBase : nullptr;
			}
			return base;
		}

		// A person or a thing the player can carry, by itself or by a reference of it. The player is left out, nearly
		// every quest holds the player in an alias or a script property. The ties last the whole game, a form the game
		// made while it runs may go before them and is left out
		void Tie(Ties& a_ties, const RE::TESQuest& a_quest, const RE::TESForm* a_form)
		{
			constexpr RE::FormID kPlayerBase = 0x7;
			const RE::TESForm* base = a_form;
			const auto* reference = a_form ? a_form->As<RE::TESObjectREFR>() : nullptr;
			if (reference) {
				base = LastingBase(*reference);
			}
			if (!base || base->IsDynamicForm() || base->GetFormID() == kPlayerBase || (reference && reference->IsPlayerRef())) {
				return;
			}
			if (const auto* person = base->As<RE::TESNPC>()) {
				AddOnce(a_ties.people, person);
			} else if (base->IsInventoryObject() && base->GetFormType() != RE::FormType::Light) {
				AddOnce(a_ties.things, base);
			} else {
				return;
			}
			if (reference && a_ties.references.size() < kMostReferences) {
				AddOnce(a_ties.references, reference->GetFormID());
			}
			if (Named(&a_quest)) {
				AddOnce(byForm[base->GetFormID()], &a_quest);
			}
		}

		// The variables of the scripts are read as they lie, their type and then their value, eight bytes each, the way
		// Inventory Reforged reads them. The scripts run on threads of their own while the browser asks, and a copy of
		// the smart pointer GetObject gives would count a reference on their object from here. A script of another mod
		// may also hold anything there. ZaZ Animation Pack's Config quest had an "object" of 0xA00000006 in an array
		// and reading through it crashed the game. So nothing is read through a value that can't be a pointer, and
		// every read of their memory that faults gives nothing back. Under the lock of the scripts only the handles get
		// copied, the forms of them are looked up after it, so that lock never waits on the lock of the game's forms
		constexpr std::size_t kVariables = 0x30;
		constexpr std::size_t kVariableSize = 0x10;
		constexpr std::size_t kObjectHandle = 0x20;
		constexpr std::size_t kArraySize = 0x10;
		constexpr std::size_t kArrayElements = 0x20;
		// Papyrus arrays hold 128 at most, the script extender's helpers make longer ones
		constexpr std::uint64_t kMostElements = 4096;
		constexpr std::uint32_t kMostVariables = 1024;
		// values read for one quest at most, so the lock is held a short while whatever a script holds
		constexpr std::size_t kMostValues = 16384;

		__declspec(noinline) bool Peek(std::uintptr_t a_at, std::uint64_t& a_value)
		{
			__try {
				a_value = *reinterpret_cast<const volatile std::uint64_t*>(a_at);
				return true;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}

		bool Pointer(std::uint64_t a_value)
		{
			constexpr std::uint64_t kLowest = 0x10000;
			constexpr std::uint64_t kHighest = 0x00007FFF'FFFFFFFF;
			return a_value >= kLowest && a_value < kHighest && a_value % alignof(void*) == 0;
		}

		// the handle of an object a variable holds, or of each object an array of them holds
		void CopyHandles(std::uintptr_t a_variable, std::vector<std::uint64_t>& a_handles, bool a_inArray, std::size_t& a_read)
		{
			std::uint64_t raw = 0;
			std::uint64_t value = 0;
			if (a_read >= kMostValues || !Peek(a_variable, raw) || !Peek(a_variable + sizeof(void*), value) || !Pointer(value)) {
				return;
			}
			a_read++;
			const RE::BSScript::TypeInfo type{ static_cast<RE::BSScript::TypeInfo::RawType>(raw) };
			if (type.IsObject()) {
				// a handle of the game's forms carries the form id in its low half, read without the lock the game's
				// GetHandle takes on the object
				std::uint64_t handle = 0;
				if (Peek(value + kObjectHandle, handle)) {
					a_handles.push_back(handle);
				}
			} else if (type.IsObjectArray() && !a_inArray) {
				std::uint64_t size = 0;
				if (!Peek(value + kArraySize, size)) {
					return;
				}
				const auto count = std::min<std::uint64_t>(size & 0xFFFFFFFF, kMostElements);
				for (std::uint64_t i = 0; i < count && a_read < kMostValues; ++i) {
					CopyHandles(value + kArrayElements + i * kVariableSize, a_handles, true, a_read);
				}
			}
		}

		// called under the lock of the scripts
		void CopyScripts(RE::BSScript::Internal::VirtualMachine& a_machine, RE::VMHandle a_handle, std::vector<std::uint64_t>& a_handles, std::size_t& a_read)
		{
			const auto found = a_machine.attachedScripts.find(a_handle);
			if (found == a_machine.attachedScripts.end()) {
				return;
			}
			for (std::uint32_t s = 0; s < found->second.size(); ++s) {
				const auto* script = found->second[s].get();
				// an object the scripts haven't finished making holds no values yet
				if (!script || !script->constructed || !script->initialized || !script->valid) {
					continue;
				}
				const auto* type = script->GetTypeInfo();
				if (!type) {
					continue;
				}
				const auto variables = reinterpret_cast<std::uintptr_t>(script) + kVariables;
				const auto count = std::min(type->GetTotalNumVariables(), kMostVariables);
				for (std::uint32_t v = 0; v < count && a_read < kMostValues; ++v) {
					CopyHandles(variables + v * kVariableSize, a_handles, false, a_read);
				}
			}
		}

		void Read()
		{
			if (read) {
				return;
			}
			read = true;
			static const auto work = Profile::Work("quest ties");
			const Profile::Timer timer{ work };
			using Fill = RE::BGSBaseAlias::FILL_TYPE;
			auto* machine = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = machine ? machine->GetObjectHandlePolicy1() : nullptr;
			Trail::Step("quest aliases and scripts begin");
			for (auto* quest : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESQuest>()) {
				Trail::Reading("quest aliases and scripts", quest);
				if (!quest) {
					continue;
				}
				auto& found = ties[quest];
				std::vector<RE::VMHandle> handles;
				if (policy) {
					handles.push_back(policy->GetHandleForObject(RE::FormType::Quest, quest));
				}
				for (auto* alias : quest->aliases) {
					if (!alias || alias->GetVMTypeID() != RE::BGSRefAlias::VMTYPEID) {
						continue;
					}
					const auto* reference = static_cast<RE::BGSRefAlias*>(alias);
					switch (alias->fillType.get()) {
					case Fill::kForced:
						Tie(found, *quest, reference->fillData.forced.forcedRef.get().get());
						break;
					case Fill::kCreated:
						Tie(found, *quest, reference->fillData.created.object);
						break;
					case Fill::kUniqueActor:
						Tie(found, *quest, reference->fillData.uniqueActor.uniqueActor);
						break;
					default:
						break;
					}
					if (policy) {
						handles.push_back(policy->GetHandleForObject(RE::BGSRefAlias::VMTYPEID, alias));
					}
				}
				if (!machine) {
					continue;
				}
				std::vector<std::uint64_t> held;
				std::size_t read = 0;
				{
					const RE::BSSpinLockGuard locker{ machine->attachedScriptsLock };
					for (const auto handle : handles) {
						CopyScripts(*machine, handle, held, read);
					}
				}
				for (const auto handle : held) {
					Tie(found, *quest, RE::TESForm::LookupByID(static_cast<RE::FormID>(handle & 0xFFFFFFFF)));
				}
			}
			logger::info("quests tie to {} things and people", byForm.size());
		}

		// An alias the game fills only while the quest runs, the giver or the city of a radiant quest, reads as its
		// name in words between brackets: "QuestGiver" is "[Quest Giver]"
		std::string Slot(std::string_view a_name)
		{
			std::string words = "[";
			for (std::size_t i = 0; i < a_name.size(); ++i) {
				const auto letter = static_cast<unsigned char>(a_name[i]);
				if (i > 0 && std::isupper(letter) && std::islower(static_cast<unsigned char>(a_name[i - 1]))) {
					words += ' ';
				}
				words += a_name[i] == '_' ? ' ' : a_name[i];
			}
			return words + "]";
		}

		// what an alias holds now, by the name a journal text gives it
		std::string AliasHolds(RE::TESQuest& a_quest, std::string_view a_name)
		{
			for (const auto* alias : a_quest.aliases) {
				if (!alias || _stricmp(alias->aliasName.c_str(), std::string(a_name).c_str()) != 0 || alias->GetVMTypeID() != RE::BGSRefAlias::VMTYPEID) {
					continue;
				}
				// the name of a reference takes the lock of its extra data, so it's asked with the quest's lock let go
				RE::ObjectRefHandle handle;
				{
					const RE::BSReadLockGuard locker{ a_quest.aliasAccessLock };
					const auto held = a_quest.refAliasMap.find(alias->aliasID);
					if (held != a_quest.refAliasMap.end()) {
						handle = held->second;
					}
				}
				const auto filled = handle.get();
				if (const char* name = filled ? filled->GetDisplayFullName() : nullptr; name && *name) {
					return name;
				}
				const auto* reference = static_cast<const RE::BGSRefAlias*>(alias);
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kUniqueActor && Named(reference->fillData.uniqueActor.uniqueActor)) {
					return reference->fillData.uniqueActor.uniqueActor->GetName();
				}
				if (alias->fillType.get() == RE::BGSBaseAlias::FILL_TYPE::kCreated && Named(reference->fillData.created.object)) {
					return reference->fillData.created.object->GetName();
				}
			}
			return Slot(a_name);
		}

		// the value of a global a journal text names, "<Global=FavorReward>", by the globals the quest gives its texts
		std::string GlobalSays(const RE::TESQuest& a_quest, std::string_view a_name)
		{
			if (!a_quest.textGlobals) {
				return std::string(a_name);
			}
			for (const auto* global : *a_quest.textGlobals) {
				if (global && _stricmp(global->GetFormEditorID(), std::string(a_name).c_str()) == 0) {
					return std::format("{}", static_cast<std::int64_t>(global->value));
				}
			}
			return std::string(a_name);
		}
	}

	std::string Filled(RE::TESQuest& a_quest, std::string_view a_text)
	{
		std::string filled;
		for (std::size_t i = 0; i < a_text.size(); ++i) {
			const auto close = a_text[i] == '<' ? a_text.find('>', i) : std::string_view::npos;
			if (close == std::string_view::npos) {
				filled += a_text[i];
				continue;
			}
			const auto inner = a_text.substr(i + 1, close - i - 1);
			const auto equals = inner.find('=');
			if (equals != std::string_view::npos) {
				const auto name = inner.substr(equals + 1);
				filled += inner.starts_with("Alias") ? AliasHolds(a_quest, name) : (inner.starts_with("Global") ? GlobalSays(a_quest, name) : std::string(name));
			}
			i = close;
		}
		return filled;
	}

	const Ties& TiesOf(const RE::TESQuest& a_quest)
	{
		Read();
		const auto found = ties.find(&a_quest);
		return found != ties.end() ? found->second : none;
	}

	std::vector<const RE::TESQuest*> QuestsOf(const RE::TESForm& a_form)
	{
		Read();
		const auto found = byForm.find(a_form.GetFormID());
		return found != byForm.end() ? found->second : std::vector<const RE::TESQuest*>{};
	}

	// A quest the player has in the journal shows an objective. Notice boards and handlers keep quests running at a
	// stage for a long time before the player ever takes them, those still wait for the player
	int StateOf(const RE::TESQuest& a_quest)
	{
		using State = RE::QUEST_OBJECTIVE_STATE;
		if (a_quest.IsCompleted()) {
			return 3;
		}
		auto& quest = const_cast<RE::TESQuest&>(a_quest);
		if (quest.IsRunning()) {
			bool shown = false;
			// CommonLib's list can't be walked through a const quest
			for (const auto* objective : quest.objectives) {
				const auto state = objective ? objective->state.get() : State::kDormant;
				shown = shown || state == State::kDisplayed || state == State::kCompletedDisplayed || state == State::kFailedDisplayed;
			}
			return a_quest.currentStage > 0 && shown ? 2 : 1;
		}
		return a_quest.currentStage > 0 || a_quest.alreadyRun ? 4 : 0;
	}

	std::vector<Objective> ObjectivesOf(RE::TESQuest& a_quest)
	{
		using State = RE::QUEST_OBJECTIVE_STATE;
		std::vector<Objective> all;
		for (const auto* objective : a_quest.objectives) {
			if (!objective || objective->displayText.empty()) {
				continue;
			}
			Objective one{ Filled(a_quest, objective->displayText.c_str()), 3, objective->index };
			switch (objective->state.get()) {
			case State::kDisplayed:
				one.state = 0;
				break;
			case State::kCompleted:
			case State::kCompletedDisplayed:
				one.state = 1;
				break;
			case State::kFailed:
			case State::kFailedDisplayed:
				one.state = 2;
				break;
			default:
				break;
			}
			all.push_back(std::move(one));
		}
		// the journal's order, by the objectives' numbers
		std::ranges::stable_sort(all, {}, &Objective::index);
		return all;
	}

	void Forget()
	{
		ties.clear();
		byForm.clear();
		read = false;
	}
}
