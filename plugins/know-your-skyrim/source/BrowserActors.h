#pragma once

#include "BrowserIndex.h"

// The actors in the world of a living entry, for the cheats and for where one is now
namespace BrowserActors
{
	std::vector<RE::Actor*> Of(const BrowserIndex::Item& a_entry);
	RE::Actor* Nearest(const std::vector<RE::Actor*>& a_actors, bool a_dead);
}
