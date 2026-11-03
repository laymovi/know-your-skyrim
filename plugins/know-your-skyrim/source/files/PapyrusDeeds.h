#pragma once

// What a compiled Papyrus script (pex) does to the player and to other quests, read from its byte code without
// running it: AddItem on the player, a spell or a word of power taught, a stage of another quest set. The values a call
// takes are kept as symbols, a property of the script, a number, the alias of a property and what it holds. The same
// script runs for many quests with other properties, binding the symbols to forms is the caller's, per quest
namespace PapyrusDeeds
{
	struct Symbol
	{
		enum class Kind : std::uint8_t
		{
			kPlayer,
			kOwnQuest,
			kSelf,
			kProperty,
			kNumber,
			kText,
			kAliasRef,
			kValueOf,
			kPropertyOf,
			kQuestNamed,
			kFormFromFile
		};

		Kind kind = Kind::kNumber;
		std::int32_t number = 0;
		// a property, a text, an editor id or a plugin, in lower case
		std::string name;
		// the symbol this one is taken of, the alias of an alias' reference or the global of its value
		std::int32_t of = -1;
	};

	enum class Deed : std::uint8_t
	{
		kAddItem,
		kRemoveItem,
		kAddSpell,
		kAddPerk,
		kAddShout,
		kTeachWord,
		kIncrementSkill,
		kAdvanceSkill,
		kAddPerkPoints,
		kModActorValue,
		kSetStage,
		kStart,
		kStop,
		kComplete
	};

	// a call of the script, every value it may take as symbols: what it's called on and its arguments in order
	struct Call
	{
		Deed deed = Deed::kAddItem;
		std::uint32_t function = 0;
		std::vector<std::int32_t> target;
		std::vector<std::vector<std::int32_t>> arguments;
	};

	struct Script
	{
		// the script it extends, lower case, the functions of that one run for it too
		std::string parent;
		// lower case names, a fragment of a quest stage is found by its function's name
		std::vector<std::string> functions;
		std::vector<Symbol> symbols;
		std::vector<Call> calls;
		// what a number property holds when the plugin sets nothing, by the property's lower case name
		std::unordered_map<std::string, std::int32_t> numbers;
	};

	std::optional<Script> Read(std::span<const std::byte> a_pex);
}
