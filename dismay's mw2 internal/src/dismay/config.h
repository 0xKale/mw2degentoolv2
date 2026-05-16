#pragma once

#include <string>

namespace config {
	std::string GetConfigPath();
	void Save();
	void Load();
	void ApplyToGame();
}
