#pragma once

// The item browser: every item, person, quest and pseudo-item of the load order by category, search and the plugin it
// comes from, one page of rows at a time, and the details of one entry with where it can be got. Nothing is read
// before the browser is first opened. The movie reaches these as skse.plugins.KnowYourSkyrim.<name>
namespace Browser
{
	void AddFunctions(RE::GFxMovieView& a_view, RE::GFxValue& a_root);
}
