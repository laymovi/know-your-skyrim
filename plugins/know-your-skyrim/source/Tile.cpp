#include "PCH.h"

#include "Tile.h"

#include "Functions.h"
#include "Inspect.h"
#include "ItemView.h"
#include "Scene.h"

namespace Tile
{
	namespace
	{
		using Functions::Flag;
		using Functions::Number;
		using Functions::Params;

		// the place ItemView keeps the tile's model under, it shows the browser's only model
		constexpr std::int32_t kTile = 0;

		// ShowTile(formId, centerX, centerY, size) puts the model of an entry into the tile, an item as it lies on the
		// ground, a person's face or a creature's body. ShowTile() takes it away. Tells whether a model shows. Places
		// are shares of the screen from the left and from the top, the size a share of its height
		struct ShowTile
		{
			static void Run(Params& a_params)
			{
				std::vector<ItemView::Slot> tile;
				if (a_params.argCount == 4) {
					const auto& args = a_params.args;
					tile.push_back({ kTile, static_cast<RE::FormID>(args[0].GetNumber()), Number(args[1]), Number(args[2]), false, Number(args[3]) });
				}
				const auto start = std::chrono::steady_clock::now();
				const bool shown = !ItemView::ShowSlots(tile).empty();
				if (!tile.empty()) {
					logger::info("tile model {:08X} {} in {:.1f} ms", tile.front().item, shown ? "shown" : "missing",
						std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
				}
				if (a_params.retVal) {
					a_params.retVal->SetBoolean(shown);
				}
			}
		};

		// Cover(left, top, right, bottom) in shares of the screen, a list of the movie lies there over the tile. Cover()
		// lifts it
		struct Cover
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount == 4) {
					const auto& args = a_params.args;
					Scene::Cover({ Number(args[0]), Number(args[1]), Number(args[2]), Number(args[3]) });
				} else {
					Scene::Uncover();
				}
			}
		};

		// ShowPreview(shown): the browser alone stands where the game shows the model of the selected item, the model
		// steps out while it's open
		struct ShowPreview
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount == 1) {
					Scene::ShowPreview(Flag(a_params.args[0]));
					Scene::KeepPreview();
				}
			}
		};

		// Inspect(formId, centerX, centerY, size), tells whether the item could be shown
		struct InspectBegin
		{
			static void Run(Params& a_params)
			{
				bool shown = false;
				if (a_params.argCount == 4) {
					const auto& args = a_params.args;
					shown = Inspect::Begin(static_cast<RE::FormID>(args[0].GetNumber()), false, Number(args[1]), Number(args[2]), Number(args[3]));
				}
				if (a_params.retVal) {
					a_params.retVal->SetBoolean(shown);
				}
			}
		};

		// InspectTurn(yawDegrees, pitchDegrees)
		struct InspectTurn
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount == 2) {
					Inspect::Turn(Number(a_params.args[0]), Number(a_params.args[1]));
				}
			}
		};

		// InspectZoom(factor)
		struct InspectZoom
		{
			static void Run(Params& a_params)
			{
				if (a_params.argCount == 1) {
					Inspect::Zoom(Number(a_params.args[0]));
				}
			}
		};

		struct InspectEnd
		{
			static void Run(Params&)
			{
				Inspect::End();
			}
		};
	}

	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root)
	{
		Functions::Add<ShowTile>(a_view, a_root, "ShowTile");
		Functions::Add<Cover>(a_view, a_root, "Cover");
		Functions::Add<ShowPreview>(a_view, a_root, "ShowPreview");
		Functions::Add<InspectBegin>(a_view, a_root, "Inspect");
		Functions::Add<InspectTurn>(a_view, a_root, "InspectTurn");
		Functions::Add<InspectZoom>(a_view, a_root, "InspectZoom");
		Functions::Add<InspectEnd>(a_view, a_root, "InspectEnd");
	}

	void MenuFrame()
	{
		Scene::KeepPreview();
	}

	void MenuClosed()
	{
		Inspect::End();
		ItemView::Clear();
		Scene::Uncover();
		Scene::Hide(false);
		Scene::ShowPreview(true);
		Scene::KeepPreview();
	}
}
