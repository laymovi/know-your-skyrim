#pragma once

#include "PluginSources.h"

namespace GameArchives
{
	class Files;
}

// What the plugin files tell of quests that the game doesn't keep in memory: the stages with their journal entries,
// which entry finishes or fails the quest and the quest it names next, the scripts of the quest, of its aliases and of
// its dialogue with the values their properties are set to, and the fragments each stage runs. A later plugin that
// changes a quest or a line of its dialogue wins, as in the game. Read on a thread of the browser's from the files only
namespace QuestRecords
{
	struct Property
	{
		enum class Type : std::uint8_t
		{
			kNone = 0,
			kObject = 1,
			kText = 2,
			kInteger = 3,
			kFloat = 4,
			kBool = 5,
			kObjects = 11
		};

		Type type = Type::kNone;
		RE::FormID form = 0;
		// an alias of the quest form, -1 for a form itself
		std::int16_t alias = -1;
		std::int32_t number = 0;
		std::string text;
		// the forms of an array of objects
		std::vector<RE::FormID> forms;
	};

	// a script attached to a quest, an alias or a line of dialogue, with what its plugin sets its properties to
	struct Instance
	{
		std::string script;
		std::unordered_map<std::string, Property> properties;
	};

	struct Entry
	{
		bool finishes = false;
		bool fails = false;
		std::string text;
		RE::FormID next = 0;
	};

	struct Stage
	{
		std::uint16_t index = 0;
		bool startUp = false;
		bool shutDown = false;
		std::vector<Entry> entries;
	};

	struct Alias
	{
		std::uint32_t id = 0;
		bool location = false;
		std::uint32_t flags = 0;
		RE::FormID forcedReference = 0;
		RE::FormID uniqueActor = 0;
		RE::FormID created = 0;
		RE::FormID forcedLocation = 0;
		std::vector<Instance> scripts;
	};

	struct Quest
	{
		RE::FormID id = 0;
		std::string editorID;
		std::vector<Stage> stages;
		std::vector<Instance> scripts;
		// the fragment script of the stages and the stages that run each of its functions, lower case
		std::string fragments;
		std::unordered_map<std::string, std::vector<std::uint16_t>> fragmentStages;
		std::vector<Alias> aliases;
		// the scripts of the lines of its dialogue
		std::vector<Instance> dialogue;
	};

	struct Read
	{
		std::unordered_map<RE::FormID, Quest> quests;
		std::size_t files = 0;
		std::size_t infos = 0;
		std::size_t packed = 0;
		std::size_t unread = 0;
		std::size_t texts = 0;
	};

	// a_language as the game's strings files name it, "ENGLISH"
	Read ReadAll(const std::vector<PluginSources::Source>& a_sources, const GameArchives::Files& a_files, std::string_view a_language);
}
