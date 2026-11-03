#pragma once

// The page of a place: who is in it now, alive and dead, by the places right inside it, who lives there as the
// records of the place tell, and the places inside it. Where an actor is comes from Places, the same as the "Now in"
// line of a person's page, so the two always agree
namespace BrowserPlaces
{
	void Fill(RE::GFxMovie& a_movie, RE::GFxValue& a_object, const RE::BGSLocation& a_place, bool a_whole);

	// whether the player is in the place or somewhere inside it
	bool PlayerIn(const RE::BGSLocation& a_place);

	// whether this is the place the player is in, and no place around it
	bool PlayerRightIn(const RE::BGSLocation& a_place);

	// the id of the entry of the place the player is in, the nearest one up with an entry, 0 for none
	RE::FormID PlayerPlace();
}
