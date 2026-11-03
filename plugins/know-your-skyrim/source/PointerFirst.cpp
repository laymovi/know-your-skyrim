#include "PCH.h"

#include "PointerFirst.h"

namespace PointerFirst
{
	namespace
	{
		struct Area
		{
			float left = 0.0f;
			float top = 0.0f;
			float right = 0.0f;
			float bottom = 0.0f;
		};

		Area area;
		// a press that began on the browser keeps the mouse until its release, wherever that lands
		bool pressed = false;

		bool Over(RE::GFxMovieView& a_movie, const RE::GFxMouseEvent& a_mouse)
		{
			RE::GViewport port;
			a_movie.GetViewport(&port);
			if (port.width <= 0 || port.height <= 0) {
				return false;
			}
			const float x = a_mouse.x / static_cast<float>(port.width);
			const float y = a_mouse.y / static_cast<float>(port.height);
			return x >= area.left && x < area.right && y >= area.top && y < area.bottom;
		}

		bool Taken(RE::GFxMovieView& a_movie, const RE::GFxEvent& a_event)
		{
			using Type = RE::GFxEvent::EventType;
			const auto type = a_event.type;
			if (type != Type::kMouseMove && type != Type::kMouseDown && type != Type::kMouseUp && type != Type::kMouseWheel) {
				return false;
			}
			const bool over = area.right > area.left && Over(a_movie, static_cast<const RE::GFxMouseEvent&>(a_event));
			const bool taken = over || pressed;
			if (type == Type::kMouseDown && over) {
				pressed = true;
			} else if (type == Type::kMouseUp) {
				pressed = false;
			}
			return taken;
		}

		struct ProcessMessage
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::InventoryMenu* a_menu, RE::UIMessage& a_message)
			{
				const auto* data = a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent ? static_cast<RE::BSUIScaleformData*>(a_message.data) : nullptr;
				const auto* event = data ? data->scaleformEvent : nullptr;
				if (event && a_menu->uiMovie && Taken(*a_menu->uiMovie, *event)) {
					a_menu->uiMovie->HandleEvent(*event);
					return RE::UI_MESSAGE_RESULTS::kHandled;
				}
				return original(a_menu, a_message);
			}

			static inline REL::Relocation<decltype(thunk)> original;
		};
	}

	void Install()
	{
		constexpr std::size_t kProcessMessage = 4;
		REL::Relocation<std::uintptr_t> table{ RE::InventoryMenu::VTABLE[0] };
		ProcessMessage::original = table.write_vfunc(kProcessMessage, ProcessMessage::thunk);
	}

	void SetArea(float a_left, float a_top, float a_right, float a_bottom)
	{
		area = { a_left, a_top, a_right, a_bottom };
	}

	void MenuClosed()
	{
		area = {};
		pressed = false;
	}
}
