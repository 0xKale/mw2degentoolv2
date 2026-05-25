#include "functions.hpp"
#include "iw4structs.hpp"
#include "offsets.hpp"
namespace iw4 {
    template<typename R, typename... A>
    R call(std::uintptr_t addr, A... args)
    {
        return reinterpret_cast<R(__cdecl*)(A...)>(addr)(args...);
    }
    void* cg()
    {
        return reinterpret_cast<void*>(offsets::CG);
    }
    std::uint32_t getViewmodelWeaponIndex()
    {
        return call<std::uint32_t>(offsets::BG_GetViewmodelWeaponIndex, cg());
    }
    const char* getWeaponNameRaw(std::uint32_t weaponIndex)
    {
        auto* def = call<weapon_complete_def_t*>(offsets::BG_GetWeaponCompleteDef, weaponIndex);
        if (!def || !def->name || !def->name[0])
            return nullptr;
        return def->name;
    }
    const char* getCurrentWeaponName()
    {
        return getWeaponNameRaw(getViewmodelWeaponIndex());
    }
}
