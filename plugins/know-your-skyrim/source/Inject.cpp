#include "PCH.h"

#include "Inject.h"

namespace Inject
{
	namespace
	{
		constexpr auto kScript = "KnowYourSkyrim/browser.swf";
		constexpr auto kHolder = "knowYourSkyrimScript";
		constexpr auto kMenu = "inventorymenu.swf"sv;
	}

	void IntoMenu(RE::GFxMovieView& a_view)
	{
		const auto* definition = a_view.GetMovieDef();
		const char* url = definition ? definition->GetFileURL() : nullptr;
		if (!url) {
			return;
		}
		std::string file(url);
		std::ranges::transform(file, file.begin(), [](unsigned char a_char) { return static_cast<char>(std::tolower(a_char)); });
		if (!file.ends_with(kMenu)) {
			return;
		}
		RE::GFxValue root;
		if (!a_view.GetVariable(&root, "_root") || !root.IsDisplayObject()) {
			logger::warn("no root in {}, the browser stays out", url);
			return;
		}
		RE::GFxValue holder;
		if (!root.CreateEmptyMovieClip(&holder, kHolder)) {
			logger::warn("no room for the browser in {}", url);
			return;
		}
		const RE::GFxValue path(kScript);
		holder.Invoke("loadMovie", nullptr, &path, 1);
		logger::info("the browser script is loading into {}", url);
	}
}
