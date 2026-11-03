#include "PCH.h"

#include "Subject.h"

// The models and words taken from Inventory Reforged ask whom its panel shows. Its trades with followers are its own,
// the browser always shows things to the player
namespace Subject
{
	RE::NiPointer<RE::Actor> Get()
	{
		return RE::NiPointer<RE::Actor>(RE::PlayerCharacter::GetSingleton());
	}
}
