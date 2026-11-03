#include "PCH.h"

#include "BrowserIndex.h"

#include <bit>
#include <cctype>
#include <set>
#include <unordered_set>

#include "BrowserFavorites.h"
#include "BrowserPerks.h"
#include "BrowserQuestLinks.h"
#include "BrowserQuests.h"
#include "ItemTexts.h"
#include "BrowserTextures.h"
#include "Holders.h"
#include "Knowledge.h"
#include "People.h"
#include "Places.h"
#include "Profile.h"
#include "Stats.h"
#include "Trail.h"
#include "Translations.h"

namespace BrowserIndex
{
	namespace
	{
		constexpr std::array<std::string_view, kFamilies> kFamilyNames{ "weapons", "armor", "potions", "scrolls", "food", "ingredients", "books", "keys", "misc", "living",
			"quests", "pseudo", "places", "perks" };

		// the keys of the panel's chips (24-subcategories.as of Inventory Reforged) and the browser's living, quest and place
		// ones (20-families.as), one bit each in this order
		constexpr std::array<std::string_view, 56> kChips{ "oneHanded", "twoHanded", "ranged", "staff", "ammo", "light", "heavy", "clothing", "jewelry", "shield", "head",
			"body", "hands", "feet", "other", "health", "magicka", "stamina", "poison", "otherPotion", "meal", "drink", "book", "note", "map", "spellTome", "ore", "ingot", "gem",
			"soulGem", "pelt", "otherMisc", "people", "animals", "undead", "daedra", "dragons", "automatons", "otherLiving", "merchants", "hirelings", "followers", "story",
			"mainQuests", "factionQuests", "daedricQuests", "sideQuests", "miscQuests", "otherQuests", "trainers", "holds", "towns", "shops", "homes", "dungeons",
			"otherPlaces" };
		static_assert(kChips.size() <= 64, "a chip is a bit of a 64 bit mask");

		consteval std::uint64_t Chip(std::string_view a_key)
		{
			for (std::size_t i = 0; i < kChips.size(); ++i) {
				if (kChips[i] == a_key) {
					return std::uint64_t{ 1 } << i;
				}
			}
			throw "no such chip";
		}

		// body slots as the panel sorts armor into its slots, the first that holds wins
		constexpr std::uint32_t kBodyParts = 0x4;
		constexpr std::uint32_t kHeadParts = 0x3803;
		constexpr std::uint32_t kHandParts = 0x18;
		constexpr std::uint32_t kFeetParts = 0x180;
		constexpr std::uint32_t kNeckParts = 0x20;
		constexpr std::uint32_t kRingParts = 0x40;
		constexpr std::uint32_t kShieldParts = 0x200;
		// neck, finger, circlet
		constexpr std::uint32_t kJewelryParts = 0x20 | 0x40 | 0x1000;

		// the sound of drinking, SkyUI tells a drink from a meal by it
		constexpr RE::FormID kDrinkSound = 0x000B6435;
		constexpr RE::FormID kLeather = 0x000DB5D2;
		constexpr RE::FormID kLeatherStrips = 0x000800E4;
		constexpr RE::FormID kPlayer = 0x00000007;
		// SkyUI 5.2 knows ore and treasure maps by these forms of the game
		constexpr std::array<RE::FormID, 10> kSkyrimOre{ 0x5ACDB, 0x5ACDC, 0x5ACDD, 0x5ACDE, 0x5ACDF, 0x5ACE0, 0x5ACE1, 0x5ACE2, 0x5B2DF, 0x71CF3 };
		constexpr std::array<RE::FormID, 3> kDragonbornOre{ 0x2B06B, 0x17749, 0x195A9 };
		constexpr std::array<RE::FormID, 12> kMaps{ 0xDDEFB, 0xEF07A, 0xF33CD, 0xF33CE, 0xF33CF, 0xF33D0, 0xF33D1, 0xF33D2, 0xF33D3, 0xF33D4, 0xF33D5, 0xF33E0 };
		// a template of a template of a template is as far as the game's own records go
		constexpr int kTemplateDepth = 4;

		// the kind of a living thing by the keywords of its race, the first that holds wins
		struct LivingKind
		{
			std::string_view keyword;
			std::uint64_t chip;
			const char* icon;
		};
		constexpr std::array<LivingKind, 6> kLivingKinds{ { { "ActorTypeDragon", Chip("dragons"), "dragon" }, { "ActorTypeDwarven", Chip("automatons"), "cog" },
			{ "ActorTypeDaedra", Chip("daedra"), "horns" }, { "ActorTypeUndead", Chip("undead"), "skull" }, { "ActorTypeAnimal", Chip("animals"), "paw" },
			{ "ActorTypeNPC", Chip("people"), "hero" } } };

		// Factions of the game every follower and hireling mod puts its people in, so the game's own dialogue offers them
		constexpr RE::FormID kPotentialFollower = 0x0005C84D;
		constexpr RE::FormID kPotentialHireling = 0x000BCC9A;

		// The slots of the body a real piece of clothing or armor can sit in: head, hair, body, hands, forearms, neck,
		// finger, feet, calves, shield, circlet. A piece with no model to drop and none of these is a body part or a
		// token worn in a slot of its own
		constexpr std::uint32_t kWornParts = 0x3FF | 0x1000;

		std::vector<Item> items;
		std::vector<std::vector<RE::TESNPC*>> units;
		std::unordered_map<RE::FormID, std::uint32_t> byForm;
		std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> variantsOf;
		// the places right inside a place, by the place's own entry
		std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> placesIn;
		std::vector<std::string> files;
		std::vector<std::wstring> loweredFiles;
		std::vector<std::array<std::uint32_t, 6>> fileCounts;
		std::unordered_map<const RE::TESFile*, std::uint16_t> fileIndex;
		std::unordered_set<RE::FormID> ores;
		// the skins races and people wear under their armor
		std::unordered_set<const RE::TESForm*> skins;
		std::size_t pseudoByModel = 0;
		std::size_t pseudoBySkin = 0;

		std::unordered_map<const RE::BGSKeyword*, std::wstring> loweredKeywords;
		std::unordered_map<RE::FormID, std::wstring> loweredEffects;
		// what the search by descriptions looks in, kept once the files it comes from are read
		std::unordered_map<RE::FormID, std::wstring> loweredTexts;

		std::string lastKey;
		Found last;

		bool HasKeyword(const RE::TESForm& a_form, std::initializer_list<std::string_view> a_words)
		{
			const auto* keywords = a_form.As<RE::BGSKeywordForm>();
			if (!keywords) {
				return false;
			}
			for (const auto* keyword : keywords->GetKeywords()) {
				const std::string_view id = keyword ? keyword->formEditorID.c_str() : "";
				if (std::ranges::find(a_words, id) != a_words.end()) {
					return true;
				}
			}
			return false;
		}

		std::uint16_t FileOf(const RE::TESForm& a_form)
		{
			const auto* file = a_form.GetFile(0);
			if (const auto known = fileIndex.find(file); known != fileIndex.end()) {
				return known->second;
			}
			const auto index = static_cast<std::uint16_t>(files.size());
			files.emplace_back(file ? std::string(file->GetFilename()) : std::string());
			loweredFiles.push_back(Lowered(files.back()));
			fileIndex.emplace(file, index);
			return index;
		}

		// what a plugin's entries are counted as: items, living things, quests, pseudo-items, places, perks
		std::size_t CountOf(Family a_family)
		{
			switch (a_family) {
			case Family::kLiving:
				return 1;
			case Family::kQuests:
				return 2;
			case Family::kPseudo:
				return 3;
			case Family::kPlaces:
				return 4;
			case Family::kPerks:
				return 5;
			default:
				return 0;
			}
		}

		void Weapon(Item& a_item, const RE::TESObjectWEAP& a_weapon)
		{
			using Type = RE::WEAPON_TYPE;
			switch (a_weapon.weaponData.animationType.get()) {
			case Type::kTwoHandSword:
			case Type::kTwoHandAxe:
				a_item.chips = Chip("twoHanded");
				a_item.icon = "battleaxe";
				break;
			case Type::kBow:
			case Type::kCrossbow:
				a_item.chips = Chip("ranged");
				a_item.icon = "bow";
				break;
			case Type::kStaff:
				a_item.chips = Chip("staff");
				a_item.icon = "staff";
				break;
			default:
				a_item.chips = Chip("oneHanded");
				a_item.icon = "sword";
				break;
			}
		}

		// A piece of armor gets a class and a slot as the panel's chips have them. Clothing that sits only where
		// jewelry sits counts as jewelry unless a keyword of the game says otherwise, as in SkyUI
		void Armor(Item& a_item, const RE::TESObjectARMO& a_armor)
		{
			using ArmorType = RE::BIPED_MODEL::ArmorType;
			const auto parts = static_cast<std::uint32_t>(a_armor.bipedModelData.bipedObjectSlots.underlying());
			if (parts & kShieldParts) {
				a_item.chips = Chip("shield");
				a_item.icon = "shield";
				return;
			}
			switch (a_armor.bipedModelData.armorType.get()) {
			case ArmorType::kLightArmor:
				a_item.chips = Chip("light");
				break;
			case ArmorType::kHeavyArmor:
				a_item.chips = Chip("heavy");
				break;
			default:
				{
					const bool jewelry = HasKeyword(a_armor, { "ArmorJewelry", "VendorItemJewelry" });
					const bool clothing = HasKeyword(a_armor, { "ArmorClothing", "VendorItemClothing" });
					const bool jewelryOnly = parts != 0 && (parts & ~kJewelryParts) == 0;
					a_item.chips = jewelry || (!clothing && jewelryOnly) ? Chip("jewelry") : Chip("clothing");
					break;
				}
			}
			if (parts & kBodyParts) {
				a_item.chips |= Chip("body");
				a_item.icon = "cuirass";
			} else if (parts & kHeadParts) {
				a_item.chips |= Chip("head");
				a_item.icon = "helmet";
			} else if (parts & kHandParts) {
				a_item.chips |= Chip("hands");
				a_item.icon = "gauntlets";
			} else if (parts & kFeetParts) {
				a_item.chips |= Chip("feet");
				a_item.icon = "boots";
			} else {
				// jewelry MARA made slotless, or a ring a mod put on a slot of its own, is told by its keywords
				const bool ring = parts & kRingParts || HasKeyword(a_armor, { "ClothingRing", "gearMaraRing" });
				const bool neck = parts & kNeckParts || HasKeyword(a_armor, { "ClothingNecklace", "gearMaraAmulet" });
				a_item.chips |= Chip("other");
				a_item.icon = ring ? "ring" : (neck ? "amulet" : "cloak");
			}
		}

		// the effect a potion is about, the dearest one
		const RE::EffectSetting* MainEffect(const RE::MagicItem& a_magic)
		{
			const RE::Effect* main = nullptr;
			for (const auto* effect : a_magic.effects) {
				if (effect && effect->baseEffect && (!main || effect->cost > main->cost)) {
					main = effect;
				}
			}
			return main ? main->baseEffect : nullptr;
		}

		void Potion(Item& a_item, const RE::AlchemyItem& a_potion)
		{
			using Value = RE::ActorValue;
			if (a_potion.IsFood()) {
				const bool drink = a_potion.data.consumptionSound && a_potion.data.consumptionSound->GetFormID() == kDrinkSound;
				a_item.family = Family::kFood;
				a_item.chips = drink ? Chip("drink") : Chip("meal");
				a_item.icon = drink ? "mug" : "food";
				return;
			}
			a_item.family = Family::kPotions;
			a_item.icon = "potion";
			if (a_potion.IsPoison()) {
				a_item.chips = Chip("poison");
				a_item.icon = "poison";
				return;
			}
			const auto* effect = MainEffect(a_potion);
			switch (effect ? effect->data.primaryAV : Value::kNone) {
			case Value::kHealth:
			case Value::kHealRate:
				a_item.chips = Chip("health");
				break;
			case Value::kMagicka:
			case Value::kMagickaRate:
				a_item.chips = Chip("magicka");
				break;
			case Value::kStamina:
			case Value::kStaminaRate:
				a_item.chips = Chip("stamina");
				break;
			default:
				a_item.chips = Chip("otherPotion");
				break;
			}
		}

		// A note of the game is a book drawn as a sheet. The letters of Skyrim.esm and of mods keep the type of a book
		// in their data and only their model, Clutter\Books\Note01.nif, says note, and the list of SkyUI calls them
		// notes. A book of the note type counts too (CommonLib's kNoteScroll is -1 of an int, it never meets the byte)
		bool Note(const RE::TESObjectBOOK& a_book)
		{
			constexpr std::uint8_t kNoteType = 0xFF;
			if (a_book.data.type.underlying() == kNoteType) {
				return true;
			}
			std::string model = a_book.GetModel() ? a_book.GetModel() : "";
			std::ranges::transform(model, model.begin(), [](unsigned char a_letter) { return static_cast<char>(std::tolower(a_letter)); });
			const auto file = model.substr(model.find_last_of("\\/") == std::string::npos ? 0 : model.find_last_of("\\/") + 1);
			return file.starts_with("note") || file.starts_with("letter");
		}

		void Book(Item& a_item, const RE::TESObjectBOOK& a_book)
		{
			using BookFlag = RE::OBJ_BOOK::Flag;
			if (a_book.data.flags.any(BookFlag::kTeachesSpell)) {
				a_item.chips = Chip("spellTome");
				a_item.icon = "tome";
			} else if (std::ranges::find(kMaps, a_book.GetFormID()) != kMaps.end()) {
				a_item.chips = Chip("map");
				a_item.icon = "map";
			} else if (Note(a_book) || HasKeyword(a_book, { "VendorItemRecipe" })) {
				a_item.chips = Chip("note");
				a_item.icon = "note";
			} else {
				a_item.chips = Chip("book");
				a_item.icon = "book";
			}
		}

		void Misc(Item& a_item, const RE::TESForm& a_misc)
		{
			const auto id = a_misc.GetFormID();
			if (ores.contains(id)) {
				a_item.chips = Chip("ore");
				a_item.icon = "ore";
			} else if (HasKeyword(a_misc, { "VendorItemOreIngot" })) {
				a_item.chips = Chip("ingot");
				a_item.icon = "ingot";
			} else if (HasKeyword(a_misc, { "VendorItemGem" })) {
				a_item.chips = Chip("gem");
				a_item.icon = "gem";
			} else if (id == kLeather || id == kLeatherStrips || HasKeyword(a_misc, { "VendorItemAnimalHide" })) {
				a_item.chips = Chip("pelt");
				a_item.icon = "pelt";
			} else {
				a_item.chips = Chip("otherMisc");
				a_item.icon = "sack";
			}
		}

		// what the player can't carry stays out: skins of creatures, unplayable weapons, lights fixed to a wall
		bool Carried(const RE::TESBoundObject& a_form)
		{
			if (const auto* weapon = a_form.As<RE::TESObjectWEAP>()) {
				return (weapon->GetFormFlags() & RE::TESObjectWEAP::RecordFlags::kNonPlayable) == 0 && weapon->weaponData.flags.none(RE::TESObjectWEAP::Data::Flag::kNonPlayable);
			}
			if (const auto* armor = a_form.As<RE::TESObjectARMO>()) {
				return (armor->GetFormFlags() & RE::TESObjectARMO::RecordFlags::kNonPlayable) == 0;
			}
			if (const auto* ammo = a_form.As<RE::TESAmmo>()) {
				return ammo->GetRuntimeData().data.flags.none(RE::AMMO_DATA::Flag::kNonPlayable);
			}
			if (const auto* light = a_form.As<RE::TESObjectLIGH>()) {
				return light->CanBeCarried();
			}
			if (const auto* book = a_form.As<RE::TESObjectBOOK>()) {
				return book->data.flags.none(RE::OBJ_BOOK::Flag::kCantTake);
			}
			return true;
		}

		bool Lasting(const RE::TESForm* a_form)
		{
			return a_form && !a_form->IsDeleted() && (a_form->GetFormID() >> 24) != 0xFF;
		}

		// a model to lie in the world as, a piece of armor has one for each sex
		bool HasModel(const RE::TESForm& a_form)
		{
			if (const auto* armor = a_form.As<RE::TESObjectARMO>()) {
				return std::ranges::any_of(armor->worldModels, [](const RE::TESModelTextureSwap& a_model) { return a_model.GetModel() && *a_model.GetModel(); });
			}
			const auto* model = skyrim_cast<const RE::TESModel*>(&a_form);
			return !model || (model->GetModel() && *model->GetModel());
		}

		// The skin of a race, or a piece with no model to drop that sits in none of the slots real pieces use or in nearly
		// all of them at once, a body part or a mark a mod puts on (OStim's Fake Armor takes 27 slots). Any other thing
		// with no model is a token of a script
		bool Pseudo(const RE::TESBoundObject& a_form)
		{
			constexpr int kMarkSlots = 10;
			const auto* armor = a_form.As<RE::TESObjectARMO>();
			if (armor && skins.contains(armor)) {
				pseudoBySkin++;
				return true;
			}
			if (HasModel(a_form)) {
				return false;
			}
			if (armor) {
				const auto parts = static_cast<std::uint32_t>(armor->bipedModelData.bipedObjectSlots.underlying());
				const bool worn = (parts & kWornParts) != 0 && std::popcount(parts) < kMarkSlots;
				if (worn || HasKeyword(*armor, { "ClothingRing", "ClothingNecklace", "gearMaraRing", "gearMaraAmulet" })) {
					return false;
				}
			}
			pseudoByModel++;
			return true;
		}

		// A name as the player reads it, with the tags quests fill in ("<Alias=Home> Furnishings") cut to the word they
		// hold and spaces at the ends dropped. A name that began with a space sorted before every other one
		std::string Shown(std::string_view a_name)
		{
			std::string shown;
			for (std::size_t i = 0; i < a_name.size(); ++i) {
				const auto close = a_name[i] == '<' ? a_name.find('>', i) : std::string_view::npos;
				if (close == std::string_view::npos) {
					shown += a_name[i];
					continue;
				}
				const auto equals = a_name.find('=', i);
				if (equals != std::string_view::npos && equals < close) {
					shown += a_name.substr(equals + 1, close - equals - 1);
				}
				i = close;
			}
			const auto first = shown.find_first_not_of(" \t");
			const auto last = shown.find_last_not_of(" \t");
			shown = first == std::string::npos ? std::string() : shown.substr(first, last - first + 1);
			// a name that is a key of a translation file is shown by the game in the words of the file
			if (shown.starts_with('$')) {
				if (auto words = Translations::Game(shown); !words.empty()) {
					return words;
				}
			}
			return shown;
		}

		bool HasWord(std::string_view a_text, std::string_view a_word)
		{
			for (auto at = a_text.find(a_word); at != std::string_view::npos; at = a_text.find(a_word, at + 1)) {
				const bool starts = at == 0 || !std::isalpha(static_cast<unsigned char>(a_text[at - 1]));
				const bool ends = at + a_word.size() == a_text.size() || !std::isalpha(static_cast<unsigned char>(a_text[at + a_word.size()]));
				if (starts && ends) {
					return true;
				}
			}
			return false;
		}

		// a name of capitals and a number the developers gave a spot of the map they never named, "POI 13"
		bool Numbered(std::string_view a_name)
		{
			std::size_t capitals = 0;
			while (capitals < a_name.size() && std::isupper(static_cast<unsigned char>(a_name[capitals]))) {
				capitals++;
			}
			auto rest = a_name.substr(capitals);
			if (rest.starts_with(' ')) {
				rest.remove_prefix(1);
			}
			return capitals >= 2 && capitals <= 4 && !rest.empty() && std::ranges::all_of(rest, [](unsigned char a_letter) { return std::isdigit(a_letter) != 0; });
		}

		// The words a record carries when it was never meant to be seen: a string the plugin lost, a stand-in some mods
		// name "Report me as a bug if you see me in game", the marks the developers put on their helpers ("Do Not Delete",
		// "Colovian Brandy TEMP", "Lydia Debug Apple", "Dummy Object", "POI 13"), a letter whose name is still "TBD"
		bool Placeholder(std::string_view a_name)
		{
			static constexpr std::array<std::string_view, 7> kMarks{ "uninitialized text", "report me as a bug", "do not delete", "placeholder", "dummy object",
				"load dummy", "trap dummy" };
			std::string lowered(a_name);
			std::ranges::transform(lowered, lowered.begin(), [](unsigned char a_letter) { return static_cast<char>(std::tolower(a_letter)); });
			return std::ranges::any_of(kMarks, [&](std::string_view a_mark) { return lowered.find(a_mark) != std::string::npos; }) || HasWord(lowered, "debug") ||
			       HasWord(lowered, "tbd") || HasWord(a_name, "TEMP") || Numbered(a_name);
		}

		// A quest that only works the game: the handlers, controllers and triggers of a guild or a mod, a quest of the
		// game's radiant lines named after what someone says ("\"Did you find something good?\""). A person can be one
		// too, 3DNPC's "NPC Controller"
		bool Controller(std::string_view a_name)
		{
			std::string lowered(a_name);
			std::ranges::transform(lowered, lowered.begin(), [](unsigned char a_letter) { return static_cast<char>(std::tolower(a_letter)); });
			return HasWord(lowered, "controller");
		}

		bool Working(std::string_view a_name)
		{
			std::string lowered(a_name);
			std::ranges::transform(lowered, lowered.begin(), [](unsigned char a_letter) { return static_cast<char>(std::tolower(a_letter)); });
			return a_name.starts_with('"') || HasWord(lowered, "handler") || Controller(a_name) || HasWord(lowered, "trigger");
		}

		bool Unseen(std::string_view a_name);

		void Add(RE::TESBoundObject* a_form)
		{
			// forms the game made while it runs, brewed potions and enchanted pieces, belong to one save
			if (!Lasting(a_form) || !Carried(*a_form)) {
				return;
			}
			const char* named = a_form->GetName();
			const auto name = Shown(named ? named : "");
			if (Unseen(name)) {
				return;
			}
			Item item{ a_form, name, Lowered(name) };
			item.file = FileOf(*a_form);
			switch (a_form->GetFormType()) {
			case RE::FormType::Weapon:
				item.family = Family::kWeapons;
				Weapon(item, *a_form->As<RE::TESObjectWEAP>());
				break;
			case RE::FormType::Ammo:
				item.family = Family::kWeapons;
				item.chips = Chip("ammo");
				item.icon = "arrows";
				break;
			case RE::FormType::Armor:
				item.family = Family::kArmor;
				Armor(item, *a_form->As<RE::TESObjectARMO>());
				break;
			case RE::FormType::AlchemyItem:
				Potion(item, *a_form->As<RE::AlchemyItem>());
				break;
			case RE::FormType::Scroll:
				item.family = Family::kScrolls;
				item.icon = "scroll";
				break;
			case RE::FormType::Ingredient:
				item.family = Family::kIngredients;
				item.icon = "ingredient";
				break;
			case RE::FormType::Book:
				item.family = Family::kBooks;
				Book(item, *a_form->As<RE::TESObjectBOOK>());
				break;
			case RE::FormType::KeyMaster:
				item.family = Family::kKeys;
				item.icon = "key";
				break;
			case RE::FormType::SoulGem:
				item.family = Family::kMisc;
				item.chips = Chip("soulGem");
				item.icon = "soulgem";
				break;
			default:
				item.family = Family::kMisc;
				Misc(item, *a_form);
				break;
			}
			if (Pseudo(*a_form)) {
				item.family = Family::kPseudo;
				item.chips = 0;
			}
			items.push_back(std::move(item));
		}

		template <class T>
		void AddAll(RE::TESDataHandler& a_data, std::string_view a_step)
		{
			for (auto* form : a_data.GetFormArray<T>()) {
				Trail::Reading(a_step, form);
				Add(form);
			}
		}

		// a name of the kind the editor gives a record, "AudioTemplateSkeleton", one word with capitals inside it
		bool EditorName(std::string_view a_name)
		{
			if (a_name.find(' ') != std::string_view::npos || a_name.size() < 9) {
				return false;
			}
			std::size_t capitals = 0;
			for (std::size_t i = 1; i < a_name.size(); ++i) {
				capitals += std::isupper(static_cast<unsigned char>(a_name[i])) && std::islower(static_cast<unsigned char>(a_name[i - 1])) ? 1 : 0;
			}
			return capitals >= 2;
		}

		// A name no player was meant to see, the same rule for every kind of entry: none, a lost string or a stand-in, a
		// name of the editor, or one of the developers' tests ("Test Vorpal Sword", "TestJeffLTankard")
		bool Unseen(std::string_view a_name)
		{
			return a_name.empty() || Placeholder(a_name) || EditorName(a_name) || Places::Testing(a_name);
		}

		// the kind most records of a living entry have by their race, other when none says
		std::size_t KindOf(const std::vector<RE::TESNPC*>& a_people)
		{
			std::array<std::size_t, kLivingKinds.size()> votes{};
			for (const auto* person : a_people) {
				const auto* race = People::RaceOf(*person);
				for (std::size_t k = 0; race && k < kLivingKinds.size(); ++k) {
					if (HasKeyword(*race, { kLivingKinds[k].keyword })) {
						votes[k]++;
						break;
					}
				}
			}
			const auto most = std::ranges::max_element(votes);
			return *most > 0 ? static_cast<std::size_t>(most - votes.begin()) : kLivingKinds.size();
		}

		// the kinds of quests that tell a story, a radiant errand or a quest of no kind says nothing of who matters
		bool Telling(const RE::TESQuest& a_quest)
		{
			using Type = RE::QUEST_DATA::Type;
			const auto type = a_quest.GetType();
			return type != Type::kNone && type != Type::kMiscellaneous;
		}

		// the people a quest of a story holds by name or by the one reference it is forced on
		std::unordered_set<const RE::TESNPC*> ReadStory(RE::TESDataHandler& a_data)
		{
			using Fill = RE::BGSBaseAlias::FILL_TYPE;
			std::unordered_set<const RE::TESNPC*> story;
			for (auto* quest : a_data.GetFormArray<RE::TESQuest>()) {
				Trail::Reading("index, people of story quests", quest);
				if (!Lasting(quest) || !Telling(*quest)) {
					continue;
				}
				for (auto* alias : quest->aliases) {
					if (!alias || alias->GetVMTypeID() != RE::BGSRefAlias::VMTYPEID) {
						continue;
					}
					const auto* reference = static_cast<RE::BGSRefAlias*>(alias);
					if (alias->fillType.get() == Fill::kUniqueActor && reference->fillData.uniqueActor.uniqueActor) {
						story.insert(reference->fillData.uniqueActor.uniqueActor);
					} else if (alias->fillType.get() == Fill::kForced) {
						const auto forced = reference->fillData.forced.forcedRef.get();
						const auto* base = forced ? forced->GetBaseObject() : nullptr;
						if (const auto* person = base ? base->As<RE::TESNPC>() : nullptr) {
							story.insert(person);
						}
					}
				}
			}
			return story;
		}

		// The class a record trains by. A record that takes its stats from another one has that one's class, a list of
		// records to pick from says nothing until the game picks
		const RE::TESClass* ClassOf(const RE::TESNPC& a_person)
		{
			using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
			const auto* person = &a_person;
			for (int depth = 0; depth < kTemplateDepth && person->baseTemplateForm && person->actorData.templateUseFlags.any(Use::kStats); ++depth) {
				person = person->baseTemplateForm->As<RE::TESNPC>();
				if (!person) {
					return nullptr;
				}
			}
			return person->npcClass;
		}

		// the record whose factions a person has, a record that takes its factions from a template has the template's
		const RE::TESNPC& FactionsHolder(const RE::TESNPC& a_person)
		{
			using Use = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
			const auto* person = &a_person;
			for (int depth = 0; depth < kTemplateDepth && person->baseTemplateForm && person->actorData.templateUseFlags.any(Use::kFactions); ++depth) {
				const auto* next = person->baseTemplateForm->As<RE::TESNPC>();
				if (!next) {
					break;
				}
				person = next;
			}
			return *person;
		}

		// the JobTrainer factions of the game, JobTrainerFaction and one for each skill. The game's dialogue of
		// training asks for them, so a class that teaches makes no trainer alone
		constexpr RE::FormID kFirstTrainerFaction = 0xE3A49;
		constexpr RE::FormID kLastTrainerFaction = 0xE3A5B;

		const RE::TESClass* TrainerClass(const RE::TESNPC& a_person)
		{
			const auto* teacher = ClassOf(a_person);
			if (!teacher || teacher->data.maximumTrainingLevel == 0) {
				return nullptr;
			}
			const auto& factions = FactionsHolder(a_person).factions;
			const bool hired = std::ranges::any_of(factions, [](const RE::FACTION_RANK& a_rank) {
				const auto id = a_rank.faction ? a_rank.faction->GetFormID() : 0;
				return a_rank.rank >= 0 && id >= kFirstTrainerFaction && id <= kLastTrainerFaction;
			});
			return hired ? teacher : nullptr;
		}

		// What people do for the player: trade, train, follow for pay, follow, or matter to a story. Followers of mods join
		// the factions of the game the game's own dialogue looks at, so they count as well. One who can't die and isn't
		// a follower is kept alive by a story
		std::uint64_t RolesOf(const std::vector<RE::TESNPC*>& a_people, const std::unordered_set<const RE::TESNPC*>& a_story)
		{
			std::uint64_t roles = 0;
			bool essential = false;
			for (const auto* person : a_people) {
				for (const auto& rank : FactionsHolder(*person).factions) {
					if (!rank.faction || rank.rank < 0) {
						continue;
					}
					const auto id = rank.faction->GetFormID();
					roles |= id == kPotentialFollower ? Chip("followers") : 0;
					roles |= id == kPotentialHireling ? Chip("hirelings") | Chip("followers") : 0;
					roles |= rank.faction->vendorData.merchantContainer ? Chip("merchants") : 0;
				}
				essential = essential || person->IsEssential();
				roles |= a_story.contains(person) ? Chip("story") : 0;
				roles |= TrainerClass(*person) ? Chip("trainers") : 0;
			}
			if (essential && (roles & Chip("followers")) == 0) {
				roles |= Chip("story");
			}
			return roles;
		}

		// the least any record of the name is kept alive by, a hundred bandits with one essential among them can die
		Guard GuardOf(const std::vector<RE::TESNPC*>& a_people)
		{
			auto least = Guard::kEssential;
			for (const auto* person : a_people) {
				const auto guard = person->IsEssential() ? Guard::kEssential : (person->IsProtected() ? Guard::kProtected : Guard::kNone);
				least = std::min(least, guard);
			}
			return least;
		}

		std::uint64_t QuestChip(const RE::TESQuest& a_quest)
		{
			using Type = RE::QUEST_DATA::Type;
			switch (a_quest.GetType()) {
			case Type::kMainQuest:
			case Type::kDLC01_Vampire:
			case Type::kDLC02_Dragonborn:
				return Chip("mainQuests");
			case Type::kMagesGuild:
			case Type::kThievesGuild:
			case Type::kDarkBrotherhood:
			case Type::kCompanionsQuest:
			case Type::kCivilWar:
				return Chip("factionQuests");
			case Type::kDaedric:
				return Chip("daedricQuests");
			case Type::kSideQuest:
				return Chip("sideQuests");
			case Type::kMiscellaneous:
				return Chip("miscQuests");
			default:
				return Chip("otherQuests");
			}
		}

		// Every quest the journal can show, that is one with a name and an objective to show. Quests that only run the
		// game, a controller or a dialogue holder, have names too but never an objective
		void AddQuests(RE::TESDataHandler& a_data)
		{
			for (auto* quest : a_data.GetFormArray<RE::TESQuest>()) {
				Trail::Reading("index, quests", quest);
				if (!Lasting(quest)) {
					continue;
				}
				bool asks = false;
				for (const auto* objective : quest->objectives) {
					asks = asks || (objective && !objective->displayText.empty());
				}
				if (!asks) {
					continue;
				}
				const char* named = quest->GetName();
				const auto name = Shown(named ? named : "");
				if (Unseen(name) || Working(name)) {
					continue;
				}
				Item item{ quest, name, Lowered(name) };
				item.file = FileOf(*quest);
				item.family = Family::kQuests;
				item.chips = QuestChip(*quest);
				item.icon = "quest";
				items.push_back(std::move(item));
			}
		}

		// A place by the keywords the game sorts its locations with, the first kind that holds wins: a hold, a city, town
		// or village, an inn or a shop, a house, a dungeon or a camp to clear
		std::uint64_t PlaceChip(const RE::BGSLocation& a_location)
		{
			if (HasKeyword(a_location, { "LocTypeHold" })) {
				return Chip("holds");
			}
			if (HasKeyword(a_location, { "LocTypeCity", "LocTypeTown", "LocTypeSettlement" })) {
				return Chip("towns");
			}
			if (HasKeyword(a_location, { "LocTypeInn", "LocTypeStore" })) {
				return Chip("shops");
			}
			if (HasKeyword(a_location, { "LocTypeDwelling", "LocTypeHouse", "LocTypePlayerHouse", "LocTypeFarm" })) {
				return Chip("homes");
			}
			if (HasKeyword(a_location, { "LocTypeDungeon", "LocTypeClearable", "LocTypeDragonLair", "LocTypeGiantCamp", "LocTypeAnimalDen", "LocTypeMilitaryFort",
					"LocTypeBanditCamp", "LocTypeForswornCamp" })) {
				return Chip("dungeons");
			}
			return Chip("otherPlaces");
		}

		// Every location of the game with a name. One with a name of the editor, a test room, a holding cell or a
		// location a script keeps for itself is no place to go
		void AddPlaces(RE::TESDataHandler& a_data)
		{
			for (auto* location : a_data.GetFormArray<RE::BGSLocation>()) {
				Trail::Reading("index, places", location);
				if (!Lasting(location)) {
					continue;
				}
				const char* named = location->GetName();
				const auto name = Shown(named ? named : "");
				if (Unseen(name) || Places::Backstage(name)) {
					continue;
				}
				Item item{ location, name, Lowered(name) };
				item.file = FileOf(*location);
				item.family = Family::kPlaces;
				item.chips = PlaceChip(*location);
				item.icon = "pin";
				items.push_back(std::move(item));
			}
		}

		// Every perk of the skill trees by its first rank, its plugin the first rank's. A tree's perk with a name no
		// player was meant to see is none
		void AddPerks()
		{
			for (const auto& perk : BrowserPerks::All()) {
				auto* first = perk.ranks.front();
				Trail::Reading("index, perks", first);
				const char* named = first->GetName();
				const auto name = Shown(named ? named : "");
				if (Unseen(name)) {
					continue;
				}
				Item item{ first, name, Lowered(name) };
				item.file = FileOf(*first);
				item.family = Family::kPerks;
				item.tree = perk.tree;
				item.icon = "tome";
				items.push_back(std::move(item));
			}
		}

		// A place lies in the nearest place around it that has an entry, a location with no name between them is skipped
		void LinkPlaces()
		{
			for (std::uint32_t i = 0; i < items.size(); ++i) {
				const auto* location = items[i].family == Family::kPlaces ? items[i].form->As<RE::BGSLocation>() : nullptr;
				const auto* around = location ? location->parentLoc : nullptr;
				for (int depth = 0; around && depth < 16; ++depth, around = around->parentLoc) {
					if (const auto known = byForm.find(around->GetFormID()); known != byForm.end()) {
						placesIn[known->second].push_back(i);
						break;
					}
				}
			}
		}

		// People and creatures by their name, a hundred records of bandits are one Bandit. The first record gives
		// the entry its id and its plugin, the kind is what most of them are
		void AddLiving(RE::TESDataHandler& a_data)
		{
			std::unordered_map<std::wstring, std::uint32_t> byName;
			const auto first = items.size();
			for (auto* person : a_data.GetFormArray<RE::TESNPC>()) {
				Trail::Reading("index, people and creatures", person);
				if (!Lasting(person) || person->GetFormID() == kPlayer) {
					continue;
				}
				const char* named = person->GetName();
				const auto name = Shown(named ? named : "");
				if (Unseen(name) || Controller(name)) {
					continue;
				}
				auto lowered = Lowered(name);
				if (const auto known = byName.find(lowered); known != byName.end()) {
					units[items[known->second].unit].push_back(person);
					continue;
				}
				Item item{ person, name, lowered };
				item.file = FileOf(*person);
				item.family = Family::kLiving;
				item.unit = static_cast<std::uint32_t>(units.size());
				units.push_back({ person });
				byName.emplace(std::move(lowered), static_cast<std::uint32_t>(items.size()));
				items.push_back(std::move(item));
			}
			const auto story = ReadStory(a_data);
			for (auto i = first; i < items.size(); ++i) {
				const auto& people = units[items[i].unit];
				const auto kind = KindOf(people);
				items[i].chips = kind < kLivingKinds.size() ? kLivingKinds[kind].chip : Chip("otherLiving");
				items[i].icon = kind < kLivingKinds.size() ? kLivingKinds[kind].icon : "living";
				items[i].chips |= RolesOf(people, story);
				items[i].guard = GuardOf(people);
			}
		}

		// a recipe there makes no new piece, it betters one the player has
		bool Improves(const RE::BGSConstructibleObject& a_recipe)
		{
			return Holders::Improves(a_recipe);
		}

		std::uint32_t PlaceOfForm(const RE::TESForm* a_form)
		{
			const auto found = a_form ? byForm.find(a_form->GetFormID()) : byForm.end();
			return found != byForm.end() ? found->second : kNone;
		}

		// An enchanted piece names the piece it was enchanted from as its template. A piece made at a forge out of one
		// other piece of its kind is a variant of that one, unless a recipe turns them back into each other
		void LinkVariants(RE::TESDataHandler& a_data)
		{
			for (std::uint32_t i = 0; i < items.size(); ++i) {
				const RE::TESForm* base = items[i].form;
				for (int depth = 0; depth < kTemplateDepth && base; ++depth) {
					const RE::TESForm* next = nullptr;
					if (const auto* weapon = base->As<RE::TESObjectWEAP>()) {
						next = weapon->templateWeapon;
					} else if (const auto* armor = base->As<RE::TESObjectARMO>()) {
						next = armor->templateArmor;
					}
					if (!next || next == base) {
						break;
					}
					base = next;
				}
				const auto original = base != items[i].form ? PlaceOfForm(base) : kNone;
				if (original != kNone && original != i) {
					items[i].original = original;
				}
			}
			std::vector<std::pair<std::uint32_t, std::uint32_t>> reworked;
			std::set<std::pair<std::uint32_t, std::uint32_t>> made;
			for (const auto* recipe : a_data.GetFormArray<RE::BGSConstructibleObject>()) {
				if (!recipe || Improves(*recipe) || !recipe->createdItem) {
					continue;
				}
				const auto created = PlaceOfForm(recipe->createdItem);
				if (created == kNone || (items[created].family != Family::kWeapons && items[created].family != Family::kArmor)) {
					continue;
				}
				std::uint32_t from = kNone;
				std::uint32_t pieces = 0;
				for (std::uint32_t c = 0; c < recipe->requiredItems.numContainerObjects; ++c) {
					const auto* entry = recipe->requiredItems.containerObjects[c];
					const auto* part = entry ? entry->obj : nullptr;
					if (part && (part->GetFormType() == RE::FormType::Weapon || part->GetFormType() == RE::FormType::Armor)) {
						pieces++;
						from = part->GetFormType() == recipe->createdItem->GetFormType() ? PlaceOfForm(part) : kNone;
					}
				}
				if (pieces == 1 && from != kNone && from != created) {
					reworked.emplace_back(created, from);
					made.emplace(created, from);
				}
			}
			for (const auto& [created, from] : reworked) {
				if (!made.contains({ from, created }) && items[created].original == kNone && items[from].original == kNone) {
					items[created].original = from;
				}
			}
			// a variant of a variant hangs under the first original
			for (auto& item : items) {
				for (int depth = 0; depth < kTemplateDepth && item.original != kNone && items[item.original].original != kNone; ++depth) {
					item.original = items[item.original].original;
				}
			}
			for (std::uint32_t i = 0; i < items.size(); ++i) {
				if (items[i].original != kNone) {
					items[items[i].original].variants++;
					variantsOf[items[i].original].push_back(i);
				}
			}
		}

		void Build()
		{
			static const auto work = Profile::Work("browser index");
			const Profile::Timer timer{ work };
			const auto start = std::chrono::steady_clock::now();
			auto* data = RE::TESDataHandler::GetSingleton();
			Trail::Step(std::format("index of {} plugins begins", data->files.size()));
			for (const auto id : kSkyrimOre) {
				ores.insert(id);
			}
			for (const auto id : kDragonbornOre) {
				if (const auto form = data->LookupFormID(id, "Dragonborn.esm"); form != 0) {
					ores.insert(form);
				}
			}
			for (const auto* race : data->GetFormArray<RE::TESRace>()) {
				if (race && race->skin) {
					skins.insert(race->skin);
				}
			}
			for (const auto* person : data->GetFormArray<RE::TESNPC>()) {
				if (person && person->skin) {
					skins.insert(person->skin);
				}
			}
			AddAll<RE::TESObjectWEAP>(*data, "index, weapons");
			AddAll<RE::TESAmmo>(*data, "index, ammunition");
			AddAll<RE::TESObjectARMO>(*data, "index, armor");
			AddAll<RE::AlchemyItem>(*data, "index, potions and food");
			AddAll<RE::ScrollItem>(*data, "index, scrolls");
			AddAll<RE::IngredientItem>(*data, "index, ingredients");
			AddAll<RE::TESObjectBOOK>(*data, "index, books");
			AddAll<RE::TESKey>(*data, "index, keys");
			AddAll<RE::TESObjectMISC>(*data, "index, misc");
			AddAll<RE::TESSoulGem>(*data, "index, soul gems");
			AddAll<RE::TESObjectLIGH>(*data, "index, lights");
			AddLiving(*data);
			AddQuests(*data);
			AddPlaces(*data);
			AddPerks();
			Trail::Step("index, sorting and variants");
			std::ranges::sort(items, [](const Item& a_one, const Item& a_other) {
				return a_one.lowered != a_other.lowered ? a_one.lowered < a_other.lowered : a_one.form->GetFormID() < a_other.form->GetFormID();
			});
			for (std::uint32_t i = 0; i < items.size(); ++i) {
				byForm.emplace(items[i].form->GetFormID(), i);
				if (items[i].unit != kNone) {
					for (const auto* person : units[items[i].unit]) {
						byForm.emplace(person->GetFormID(), i);
					}
				}
				// every rank of a perk leads to its entry
				if (const auto* perk = items[i].family == Family::kPerks ? BrowserPerks::Of(*items[i].form) : nullptr) {
					for (const auto* rank : perk->ranks) {
						byForm.emplace(rank->GetFormID(), i);
					}
				}
			}
			LinkVariants(*data);
			LinkPlaces();
			fileCounts.assign(files.size(), {});
			for (const auto& item : items) {
				fileCounts[item.file][CountOf(item.family)]++;
			}
			const auto took = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			const auto quests = std::ranges::count_if(items, [](const Item& a_item) { return a_item.family == Family::kQuests; });
			const auto places = std::ranges::count_if(items, [](const Item& a_item) { return a_item.family == Family::kPlaces; });
			const auto perks = std::ranges::count_if(items, [](const Item& a_item) { return a_item.family == Family::kPerks; });
			logger::info(
				"the item browser knows {} entries ({} living, {} quests, {} places, {} perks, {} variants, {} pseudo-items: {} skins, {} with no model) of {} plugins, made in {:.1f} ms",
				items.size(), units.size(), quests, places, perks, std::ranges::count_if(items, [](const Item& a_item) { return a_item.original != kNone; }),
				pseudoBySkin + pseudoByModel, pseudoBySkin, pseudoByModel, files.size(), took);
			std::size_t told = 0;
			for (const auto& item : items) {
				if (item.family == Family::kPseudo && told++ < 60) {
					logger::info("   pseudo-item {} [{:08X}] of {}", item.name, item.form->GetFormID(), files[item.file]);
				}
			}
		}

		const std::wstring& KeywordText(const RE::BGSKeyword& a_keyword)
		{
			auto known = loweredKeywords.find(&a_keyword);
			if (known == loweredKeywords.end()) {
				known = loweredKeywords.emplace(&a_keyword, Lowered(a_keyword.formEditorID.c_str())).first;
			}
			return known->second;
		}

		bool KeywordHolds(const Item& a_item, const std::wstring& a_text)
		{
			const RE::TESForm* form = a_item.form;
			// a person goes by the keywords of the race as much as by its own
			if (const auto* person = form->As<RE::TESNPC>(); person && People::RaceOf(*person)) {
				const auto* race = People::RaceOf(*person)->As<RE::BGSKeywordForm>();
				if (race && std::ranges::any_of(race->GetKeywords(), [&](const RE::BGSKeyword* a_keyword) { return a_keyword && KeywordText(*a_keyword).find(a_text) != std::wstring::npos; })) {
					return true;
				}
			}
			const auto* keywords = form->As<RE::BGSKeywordForm>();
			if (!keywords) {
				return false;
			}
			return std::ranges::any_of(keywords->GetKeywords(), [&](const RE::BGSKeyword* a_keyword) { return a_keyword && KeywordText(*a_keyword).find(a_text) != std::wstring::npos; });
		}

		// the effects of an item never change, only what the player found of an ingredient does. A trainer gives
		// training, "$archery" finds who trains archery
		std::wstring EffectText(const Item& a_item)
		{
			std::wstring text;
			if (const auto training = TrainingOf(a_item)) {
				text += Lowered(training->skill) + L'\n';
			}
			for (const auto& effect : Effects(a_item, false)) {
				if (effect.known) {
					text += Lowered(effect.name) + L'\n';
				}
			}
			return text;
		}

		bool EffectHolds(const Item& a_item, const std::wstring& a_text)
		{
			if (a_item.family == Family::kIngredients) {
				return EffectText(a_item).find(a_text) != std::wstring::npos;
			}
			const auto id = a_item.form->GetFormID();
			auto known = loweredEffects.find(id);
			if (known == loweredEffects.end()) {
				known = loweredEffects.emplace(id, EffectText(a_item)).first;
			}
			return known->second.find(a_text) != std::wstring::npos;
		}

		bool TextsRead()
		{
			return ItemTexts::Ready() && BrowserQuestLinks::Ready();
		}

		// The description of a thing, the words of its effects as the page shows them, the objectives and the journal
		// entries of a quest, the description of every rank of a perk
		std::wstring TextOf(const Item& a_item)
		{
			std::string text(ItemTexts::Of(a_item.form->GetFormID()));
			if (const auto* perk = a_item.family == Family::kPerks ? BrowserPerks::Of(*a_item.form) : nullptr) {
				for (std::size_t rank = 1; rank < perk->ranks.size(); ++rank) {
					text += '\n';
					text += ItemTexts::Of(perk->ranks[rank]->GetFormID());
				}
			}
			for (const auto& effect : Effects(a_item)) {
				text += '\n';
				text += effect.words;
			}
			if (auto* quest = a_item.form->As<RE::TESQuest>()) {
				for (const auto& objective : BrowserQuests::ObjectivesOf(*quest)) {
					text += '\n';
					text += objective.text;
				}
				text += '\n';
				text += BrowserQuestLinks::TextsOf(*quest);
			}
			return Lowered(text);
		}

		// A thing's text waits for its description to be read, a quest's for its journal entries. Meanwhile a thing
		// finds nothing and a quest only by its objectives, and neither is kept
		bool TextHolds(const Item& a_item, const std::wstring& a_text)
		{
			auto* quest = a_item.form->As<RE::TESQuest>();
			if (quest ? !BrowserQuestLinks::Ready() : !ItemTexts::Ready()) {
				if (!quest) {
					return false;
				}
				std::string objectives;
				for (const auto& objective : BrowserQuests::ObjectivesOf(*quest)) {
					objectives += objective.text;
					objectives += '\n';
				}
				return Lowered(objectives).find(a_text) != std::wstring::npos;
			}
			auto known = loweredTexts.find(a_item.form->GetFormID());
			if (known == loweredTexts.end()) {
				known = loweredTexts.emplace(a_item.form->GetFormID(), TextOf(a_item)).first;
			}
			return known->second.find(a_text) != std::wstring::npos;
		}

		bool Holds(const Item& a_item, const Term& a_term, Within a_within)
		{
			switch (a_term.kind) {
			case Term::Kind::kMod:
				return loweredFiles[a_item.file].find(a_term.text) != std::wstring::npos;
			case Term::Kind::kKeyword:
				return KeywordHolds(a_item, a_term.text);
			case Term::Kind::kEffect:
				return EffectHolds(a_item, a_term.text);
			default:
				switch (a_within) {
				case Within::kTexts:
					return TextHolds(a_item, a_term.text);
				case Within::kBoth:
					return a_item.lowered.find(a_term.text) != std::wstring::npos || TextHolds(a_item, a_term.text);
				default:
					return a_item.lowered.find(a_term.text) != std::wstring::npos;
				}
			}
		}

		bool Matches(const Item& a_item, const Query& a_query)
		{
			if (a_query.file && a_item.file != *a_query.file) {
				return false;
			}
			if (a_query.textures && !BrowserTextures::Changes(*a_query.textures, a_item)) {
				return false;
			}
			return std::ranges::all_of(a_query.terms, [&](const Term& a_term) { return a_term.text.empty() || Holds(a_item, a_term, a_query.within) != a_term.negated; });
		}

		QuestProgress ProgressOf(const RE::TESQuest& a_quest)
		{
			switch (BrowserQuests::StateOf(a_quest)) {
			case 2:
				return QuestProgress::kGoing;
			case 3:
				return QuestProgress::kDone;
			default:
				return QuestProgress::kUnfinished;
			}
		}

		// pseudo-items show in their own category alone
		bool Chosen(const Item& a_item, const Query& a_query)
		{
			const bool family = a_query.family ? a_item.family == *a_query.family : a_item.family != Family::kPseudo;
			if (!family || (a_item.chips & a_query.chips) != a_query.chips) {
				return false;
			}
			if (a_query.tree) {
				return a_item.family == Family::kPerks && a_item.tree == *a_query.tree;
			}
			if (a_query.progress) {
				const auto* quest = a_item.form->As<RE::TESQuest>();
				return quest && ProgressOf(*quest) == *a_query.progress;
			}
			if (!a_query.trains) {
				return true;
			}
			const auto training = TrainingOf(a_item);
			return training && training->value == *a_query.trains;
		}

		std::string KeyOf(const Query& a_query)
		{
			// a star put on or taken off counts the favorites anew
			auto key = std::format("{}|{}|{}|{}|{}", a_query.family ? static_cast<int>(*a_query.family) : -1, a_query.chips, a_query.file ? static_cast<int>(*a_query.file) : -1,
				a_query.favorites, BrowserFavorites::Version());
			// the things of a texture mod grow while the models are read
			if (a_query.textures) {
				key += std::format("|t{}:{}", *a_query.textures, BrowserTextures::Progress().first);
			}
			if (a_query.trains) {
				key += std::format("|s{}", static_cast<int>(*a_query.trains));
			}
			if (a_query.tree) {
				key += std::format("|r{}", *a_query.tree);
			}
			// in the progression mode each thing the character meets counts the rows anew
			if (Knowledge::On()) {
				key += std::format("|k{}", Knowledge::Version());
			}
			// a search by descriptions finds more once the files are read
			if (a_query.within != Within::kNames) {
				key += std::format("|w{}{}", static_cast<int>(a_query.within), TextsRead() ? 1 : 0);
			}
			// the player's quests move on while the game runs, their progress is a part of the key
			if (a_query.progress) {
				std::size_t states = 0;
				for (const auto& item : items) {
					if (const auto* quest = item.family == Family::kQuests ? item.form->As<RE::TESQuest>() : nullptr) {
						states = states * 31 + static_cast<std::size_t>(BrowserQuests::StateOf(*quest)) + 1;
					}
				}
				key += std::format("|p{}:{:x}", static_cast<int>(*a_query.progress), states);
			}
			for (const auto& term : a_query.terms) {
				key += std::format("|{}{}", static_cast<int>(term.kind), term.negated ? "-" : "+");
				for (const auto letter : term.text) {
					key += std::format("{:x},", static_cast<std::uint32_t>(letter));
				}
			}
			return key;
		}
	}

	std::string_view FamilyName(Family a_family)
	{
		const auto at = static_cast<std::size_t>(a_family);
		return at < kFamilyNames.size() ? kFamilyNames[at] : std::string_view();
	}

	std::optional<Family> FamilyNamed(std::string_view a_name)
	{
		for (std::size_t i = 0; i < kFamilyNames.size(); ++i) {
			if (kFamilyNames[i] == a_name) {
				return static_cast<Family>(i);
			}
		}
		return std::nullopt;
	}

	std::uint64_t ChipNamed(std::string_view a_key)
	{
		for (std::size_t i = 0; i < kChips.size(); ++i) {
			if (kChips[i] == a_key) {
				return std::uint64_t{ 1 } << i;
			}
		}
		return 0;
	}

	std::vector<std::string_view> ChipKeys(std::uint64_t a_chips)
	{
		std::vector<std::string_view> keys;
		for (std::size_t i = 0; i < kChips.size(); ++i) {
			if (a_chips & (std::uint64_t{ 1 } << i)) {
				keys.push_back(kChips[i]);
			}
		}
		return keys;
	}

	const std::vector<Item>& Items()
	{
		if (items.empty()) {
			Build();
		}
		return items;
	}

	const std::vector<std::vector<RE::TESNPC*>>& Units()
	{
		Items();
		return units;
	}

	const Item* Find(RE::FormID a_form)
	{
		Items();
		const auto found = byForm.find(a_form);
		return found != byForm.end() ? &items[found->second] : nullptr;
	}

	std::uint32_t PlaceOf(const Item& a_item)
	{
		return static_cast<std::uint32_t>(&a_item - items.data());
	}

	std::string_view FileName(std::uint16_t a_file)
	{
		return a_file < files.size() ? std::string_view(files[a_file]) : std::string_view();
	}

	std::optional<std::uint16_t> FileNamed(std::string_view a_name)
	{
		Items();
		for (std::size_t i = 0; i < files.size(); ++i) {
			if (_stricmp(files[i].c_str(), std::string(a_name).c_str()) == 0) {
				return static_cast<std::uint16_t>(i);
			}
		}
		return std::nullopt;
	}

	std::vector<std::uint32_t> VariantsOf(std::uint32_t a_original)
	{
		Items();
		const auto found = variantsOf.find(a_original);
		return found != variantsOf.end() ? found->second : std::vector<std::uint32_t>{};
	}

	std::vector<std::uint32_t> PlacesIn(const RE::BGSLocation& a_location)
	{
		const auto* entry = Find(a_location.GetFormID());
		const auto found = entry ? placesIn.find(PlaceOf(*entry)) : placesIn.end();
		return found != placesIn.end() ? found->second : std::vector<std::uint32_t>{};
	}

	std::vector<File> Files(std::string_view a_words)
	{
		Items();
		const auto words = Lowered(a_words);
		// in the progression mode a plugin counts what the character met of it, one with nothing met isn't listed
		const bool met = Knowledge::On();
		std::vector<std::array<std::uint32_t, 6>> known;
		if (met) {
			known.assign(files.size(), {});
			for (const auto& item : items) {
				if (Knowledge::Known(item)) {
					known[item.file][CountOf(item.family)]++;
				}
			}
		}
		std::vector<File> found;
		for (std::size_t i = 0; i < files.size(); ++i) {
			if (!files[i].empty() && loweredFiles[i].find(words) != std::wstring::npos) {
				const auto& counts = met ? known[i] : fileCounts[i];
				if (met && std::ranges::all_of(counts, [](std::uint32_t a_count) { return a_count == 0; })) {
					continue;
				}
				found.push_back({ files[i], counts[0], counts[1], counts[2], counts[3], counts[4], counts[5] });
			}
		}
		std::ranges::sort(found, [](const File& a_one, const File& a_other) {
			const auto one = a_one.items + a_one.living + a_one.quests + a_one.places + a_one.perks;
			const auto other = a_other.items + a_other.living + a_other.quests + a_other.places + a_other.perks;
			if (one != other) {
				return one > other;
			}
			if (a_one.pseudo != a_other.pseudo) {
				return a_one.pseudo > a_other.pseudo;
			}
			return _stricmp(std::string(a_one.name).c_str(), std::string(a_other.name).c_str()) < 0;
		});
		return found;
	}

	std::string_view Stem(std::string_view a_file)
	{
		const auto dot = a_file.rfind('.');
		if (dot == std::string_view::npos) {
			return a_file;
		}
		const auto ending = a_file.substr(dot);
		const bool plugin = _stricmp(std::string(ending).c_str(), ".esp") == 0 || _stricmp(std::string(ending).c_str(), ".esm") == 0 ||
		                    _stricmp(std::string(ending).c_str(), ".esl") == 0;
		return plugin ? a_file.substr(0, dot) : a_file;
	}

	std::optional<Training> TrainingOf(const Item& a_item)
	{
		if (a_item.unit == kNone) {
			return std::nullopt;
		}
		for (const auto* person : Units()[a_item.unit]) {
			const auto* teacher = TrainerClass(*person);
			const auto skill = teacher ? static_cast<RE::ActorValue>(static_cast<int>(RE::ActorValue::kOneHanded) + static_cast<int>(teacher->data.teaches.get())) : RE::ActorValue::kNone;
			const auto* info = teacher ? RE::ActorValueList::GetActorValueInfo(skill) : nullptr;
			const char* name = info ? info->GetFullName() : nullptr;
			if (name && *name) {
				return Training{ name, teacher->data.maximumTrainingLevel, skill };
			}
		}
		return std::nullopt;
	}

	std::span<RE::TESNPC* const> SameName(const RE::TESNPC& a_person)
	{
		const auto* entry = Find(a_person.GetFormID());
		return entry && entry->unit != kNone ? std::span<RE::TESNPC* const>(Units()[entry->unit]) : std::span<RE::TESNPC* const>();
	}

	// An original shows for what matches. A variant shows only when its original doesn't, a search for an effect only
	// an enchantment has then finds the enchanted pieces themselves. A starred variant shows among the favorites anyway
	const Found& Search(const Query& a_query)
	{
		const auto& all = Items();
		auto key = KeyOf(a_query);
		if (key == lastKey) {
			return last;
		}
		static const auto work = Profile::Work("browser search");
		const Profile::Timer timer{ work };
		if (a_query.within != Within::kNames) {
			ItemTexts::Start();
			BrowserQuestLinks::Start();
		}
		last = {};
		std::vector<char> matched(all.size(), 0);
		for (std::uint32_t i = 0; i < all.size(); ++i) {
			matched[i] = Matches(all[i], a_query) && Knowledge::Known(all[i]) ? 1 : 0;
		}
		for (std::uint32_t i = 0; i < all.size(); ++i) {
			const auto& item = all[i];
			if (!matched[i]) {
				continue;
			}
			if (BrowserFavorites::Has(item.form->GetFormID())) {
				last.favorites++;
				if (a_query.favorites) {
					last.items.push_back(i);
				}
			}
			if (item.original != kNone && matched[item.original]) {
				continue;
			}
			last.perFamily[static_cast<std::size_t>(item.family)]++;
			last.all += item.family != Family::kPseudo ? 1 : 0;
			if (!a_query.favorites && Chosen(item, a_query)) {
				last.items.push_back(i);
			}
		}
		last.reading = a_query.textures && (!BrowserTextures::Ready() || BrowserTextures::Progress().first < BrowserTextures::Progress().second);
		last.readingTexts = a_query.within != Within::kNames && !TextsRead();
		lastKey = std::move(key);
		return last;
	}

	std::wstring Lowered(std::string_view a_text)
	{
		constexpr std::uint32_t kUtf8 = 65001;
		constexpr std::uint32_t kAnsi = 0;
		constexpr std::uint32_t kStrict = 0x8;
		constexpr std::uint32_t kLowerCase = 0x100;
		if (a_text.empty()) {
			return {};
		}
		const auto length = static_cast<std::int32_t>(a_text.size());
		// the game's strings are UTF-8, an old plugin may still carry a string of the system's code page
		std::uint32_t page = kUtf8;
		auto wide = REX::W32::MultiByteToWideChar(page, kStrict, a_text.data(), length, nullptr, 0);
		if (wide <= 0) {
			page = kAnsi;
			wide = REX::W32::MultiByteToWideChar(page, 0, a_text.data(), length, nullptr, 0);
		}
		std::wstring text(static_cast<std::size_t>(std::max(wide, 0)), L'\0');
		REX::W32::MultiByteToWideChar(page, page == kUtf8 ? kStrict : 0, a_text.data(), length, text.data(), wide);
		std::wstring lowered(text.size(), L'\0');
		const auto done = REX::W32::LCMapStringEx(L"", kLowerCase, text.data(), static_cast<std::int32_t>(text.size()), lowered.data(), static_cast<std::int32_t>(lowered.size()), nullptr, nullptr, 0);
		return done > 0 ? lowered : text;
	}

	// "[SURV=Restore 220 points of Hunger.]" is said only while Survival Mode is on, the game leaves it out otherwise
	std::string SurvivalWords(std::string_view a_words)
	{
		static const auto* survival = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ModeEnabled");
		const bool on = survival && survival->value != 0.0F;
		std::string said;
		for (std::size_t at = 0; at < a_words.size();) {
			const auto open = a_words.find("[SURV=", at);
			const auto close = open == std::string_view::npos ? std::string_view::npos : a_words.find(']', open);
			if (close == std::string_view::npos) {
				said += a_words.substr(at);
				break;
			}
			said += a_words.substr(at, open - at);
			if (on) {
				said += a_words.substr(open + 6, close - open - 6);
			}
			at = close + 1;
		}
		const auto last = said.find_last_not_of(' ');
		return last == std::string::npos ? std::string() : said.substr(0, last + 1);
	}

	bool Unshown(const RE::TESForm& a_form, std::string_view a_name)
	{
		return Unseen(a_name) || (a_form.Is(RE::FormType::Quest) && Working(a_name)) || (a_form.Is(RE::FormType::NPC) && Controller(a_name));
	}

	std::vector<Effect> Effects(const Item& a_item, bool a_words)
	{
		std::vector<Effect> lines;
		const RE::MagicItem* magic = nullptr;
		auto* form = a_item.form;
		if (a_item.unit != kNone) {
			return lines;
		}
		if (const auto* enchantable = form->As<RE::TESEnchantableForm>()) {
			magic = enchantable->formEnchanting;
		} else if (const auto* book = form->As<RE::TESObjectBOOK>()) {
			if (book->data.flags.any(RE::OBJ_BOOK::Flag::kTeachesSpell)) {
				magic = book->data.teaches.spell;
				if (magic && magic->GetName() && *magic->GetName()) {
					lines.push_back({ magic->GetName(), "" });
				}
			} else if (book->data.flags.any(RE::OBJ_BOOK::Flag::kAdvancesActorValue)) {
				const auto* skill = RE::ActorValueList::GetActorValueInfo(book->data.teaches.actorValueToAdvance);
				if (skill && skill->GetFullName() && *skill->GetFullName()) {
					lines.push_back({ skill->GetFullName(), "" });
				}
				return lines;
			}
		} else {
			magic = form->As<RE::MagicItem>();
		}
		if (!magic) {
			return lines;
		}
		const auto* ingredient = form->As<RE::IngredientItem>();
		for (std::uint32_t i = 0; i < magic->effects.size(); ++i) {
			const auto* effect = magic->effects[i];
			const auto* base = effect ? effect->baseEffect : nullptr;
			if (!base || base->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				continue;
			}
			// what the player hasn't found of an ingredient stays unknown, as in the game's own menu
			if (ingredient && (ingredient->gamedata.knownEffectFlags & (1U << i)) == 0) {
				lines.push_back({ "", "", false });
				continue;
			}
			const auto& item = effect->effectItem;
			const auto words = a_words ? SurvivalWords(Stats::Words(base->magicItemDescription.c_str(), item.magnitude, static_cast<float>(item.duration))) : std::string();
			Effect line{ base->GetFullName() ? base->GetFullName() : "", words, true, effect->conditions.head != nullptr };
			// a mod that lists one effect twice shows it once
			if (std::ranges::none_of(lines, [&](const Effect& a_had) { return a_had.name == line.name && a_had.words == line.words && a_had.conditional == line.conditional; })) {
				lines.push_back(std::move(line));
			}
		}
		return lines;
	}
}
