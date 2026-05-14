#pragma once

#include "page_main.hpp"
#include "page_account.hpp"
#include "page_host.hpp"
#include "page_dedigamer.hpp"
#include "page_about.hpp"
#include "../../framework/framework.hpp"

namespace layout {
	inline constexpr float colWidth = settings::menu_main_column_width_px;
	inline constexpr float colGap = settings::menu_main_column_gap_px;
	inline constexpr float leftPull = 10.f;
	inline constexpr float rightExtend = 8.f;
	inline constexpr float leftWidth = colWidth + leftPull;
	inline constexpr float rightWidth = colWidth + rightExtend;
}

namespace menu_pages {
	void RenderMainPage() noexcept;
	void RenderAccountPage() noexcept;
	void RenderHostPage() noexcept;
	void RenderDedigamerPage() noexcept;
	void RenderAboutPage() noexcept;
}
