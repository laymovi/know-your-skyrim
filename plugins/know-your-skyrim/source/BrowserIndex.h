#pragma once

// Every item of the load order the player can carry and every named living thing, sorted by name, with the
// category and subcategories the panel's strip of chips has, and the plugin it comes from. Made on the first look of
// the item browser and kept while the game runs, records don't change after the data is loaded
namespace BrowserIndex
{
	enum class Family : std::uint8_t
	{
		kWeapons,
		kArmor,
		kPotions,
		kScrolls,
		kFood,
		kIngredients,
		kBooks,
		kKeys,
		kMisc,
		kLiving,
		kQuests,
		kPseudo,
		kPlaces,
		kPerks,
		kCount
	};
	inline constexpr std::size_t kFamilies = static_cast<std::size_t>(Family::kCount);
	inline constexpr std::uint32_t kNone = 0xFFFFFFFF;

	// what keeps a living thing alive: nothing, protected (only the player can kill it), essential (no one can)
	enum class Guard : std::uint8_t
	{
		kNone,
		kProtected,
		kEssential
	};

	// the names the browser's movie knows the categories by
	std::string_view FamilyName(Family a_family);
	std::optional<Family> FamilyNamed(std::string_view a_name);

	// a bit for every subcategory chip of the panel, by its key there. 0 for a key it doesn't know
	std::uint64_t ChipNamed(std::string_view a_key);
	std::vector<std::string_view> ChipKeys(std::uint64_t a_chips);

	// An entry of the browser. A living thing is every person or creature of the load order with one name, the first
	// of them stands for all and gives the entry its id. An enchanted or reworked piece is a variant of the piece it
	// was made from, the rows show the original and the original counts its variants. A quest with a name is an entry
	// too. A pseudo-item is a thing the game keeps as an item that isn't one: a body part worn in a slot of its own,
	// the skin of a race, a token of a script with no model. A place is a location of the game with a name, a city,
	// a house, a dungeon. A perk is every perk of a skill tree, its ranks one entry (BrowserPerks)
	struct Item
	{
		RE::TESForm* form = nullptr;
		std::string name;
		std::wstring lowered;
		std::uint64_t chips = 0;
		std::uint16_t file = 0;
		Family family = Family::kMisc;
		const char* icon = "";
		std::uint32_t original = kNone;
		std::uint32_t variants = 0;
		// the place in Units() of a living thing
		std::uint32_t unit = kNone;
		Guard guard = Guard::kNone;
		// the tree of a perk, a chip of Perks of its own beside the bits of chips
		std::uint8_t tree = 0;
	};

	// how far the player is with a quest as the browser's chips sort it: not begun, waiting or stopped, going, done
	enum class QuestProgress : std::uint8_t
	{
		kUnfinished,
		kGoing,
		kDone
	};

	// what the words of a search look in: the names, the descriptions (a book's text, the words of effects, the
	// journal of a quest), or either
	enum class Within : std::uint8_t
	{
		kNames,
		kTexts,
		kBoth
	};

	struct Term
	{
		enum class Kind
		{
			kName,
			kMod,
			kKeyword,
			kEffect
		};

		Kind kind = Kind::kName;
		std::wstring text;
		bool negated = false;
	};

	struct Query
	{
		std::optional<Family> family;
		// every chip asked for has to hold
		std::uint64_t chips = 0;
		std::vector<Term> terms;
		// one plugin only, by its file name
		std::optional<std::uint16_t> file;
		// the entries the player starred, of every category
		bool favorites = false;
		// the things whose textures a mod that only changes textures changes, by its place in BrowserTextures
		std::optional<std::uint16_t> textures;
		// only who trains this skill, a chip "skill:<actor value>" of the browser
		std::optional<RE::ActorValue> trains;
		// only quests the player is this far with, the chips of the second line of Quests
		std::optional<QuestProgress> progress;
		// only the perks of one tree, a chip of Perks
		std::optional<std::uint8_t> tree;
		Within within = Within::kNames;
	};

	struct Found
	{
		// places in Items(), in their order
		std::vector<std::uint32_t> items;
		// how many the terms leave in each category and in all of them, chips left out
		std::array<std::uint32_t, kFamilies> perFamily{};
		std::uint32_t all = 0;
		std::uint32_t favorites = 0;
		// the models of the things are still read, more of them come
		bool reading = false;
		// the descriptions are still read from the plugin files, the words of a search by them find more later
		bool readingTexts = false;
	};

	// made on the first call
	const std::vector<Item>& Items();
	const std::vector<std::vector<RE::TESNPC*>>& Units();
	// by the form id of an item, or of any person or creature of a living entry
	const Item* Find(RE::FormID a_form);
	std::uint32_t PlaceOf(const Item& a_item);
	std::string_view FileName(std::uint16_t a_file);
	std::optional<std::uint16_t> FileNamed(std::string_view a_name);

	// the variants of an original, in the order of the rows
	std::vector<std::uint32_t> VariantsOf(std::uint32_t a_original);

	// The plugins whose file name holds the words with how many items, living things, quests, pseudo-items, places and
	// perks each gives, the plugins that give the most first and the ones with pseudo-items alone last
	struct File
	{
		std::string_view name;
		std::uint32_t items;
		std::uint32_t living;
		std::uint32_t quests;
		std::uint32_t pseudo;
		std::uint32_t places;
		std::uint32_t perks;
	};
	std::vector<File> Files(std::string_view a_words);

	// a file name without its .esp, .esm or .esl
	std::string_view Stem(std::string_view a_file);

	// the people and creatures of the living entry a person belongs to, the tile tries their faces in turn
	std::span<RE::TESNPC* const> SameName(const RE::TESNPC& a_person);

	// The skill a trainer of a living entry teaches and the level it teaches up to, of its first record that trains
	struct Training
	{
		std::string skill;
		std::uint8_t upTo = 0;
		RE::ActorValue value = RE::ActorValue::kNone;
	};
	std::optional<Training> TrainingOf(const Item& a_item);

	// the places with an entry that lie right inside a place, in the order of the rows
	std::vector<std::uint32_t> PlacesIn(const RE::BGSLocation& a_location);

	// The last search is kept, asking again for another page of it costs nothing
	const Found& Search(const Query& a_query);

	// lower case for any language the game's strings come in, for comparing
	std::wstring Lowered(std::string_view a_text);

	// A name no player was meant to see, the same rule as for the entries: none, a lost string or a stand-in, a name
	// of the editor, a developers' test, a quest that only works the game. Pages leave out people, chests and quests
	// of such names
	bool Unshown(const RE::TESForm& a_form, std::string_view a_name);

	// The effects of an item one to a line, in the words of the game with their strength put in: an enchantment, a
	// potion, a scroll, the spell a tome teaches. An ingredient tells only the effects the player has found. The search
	// needs the names alone, a_words false leaves the words out
	struct Effect
	{
		std::string name;
		std::string words;
		bool known = true;
		// it works only while its conditions hold, a level or a set worn
		bool conditional = false;
	};
	std::vector<Effect> Effects(const Item& a_item, bool a_words = true);
}
