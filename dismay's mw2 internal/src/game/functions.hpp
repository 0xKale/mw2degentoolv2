#pragma once

#include <cstdint>

namespace iw4 {

    std::uint32_t getViewmodelWeaponIndex();
    const char* getWeaponNameRaw(std::uint32_t weaponIndex);
    const char* getCurrentWeaponName();
}
