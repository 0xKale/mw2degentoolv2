#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Define Position ONCE at the top so variables.h can see it
struct Position {
    float x, y, z, viewx, viewy;
};

#define G_LOBBYDATA     0x10F91E8
#define PARTYSESSION_P  0x10F5A18
#define MAX_CLIENTS 18

enum scriptType_e {
    SCRIPT_NONE = 0, SCRIPT_OBJECT = 1, SCRIPT_STRING = 2,
    SCRIPT_VECTOR = 4, SCRIPT_FLOAT = 5, SCRIPT_INTEGER = 6
};

struct VariableValue {
    union {
        void* entity;
        float number;
        unsigned short string;
        float* vector;
        int integer;
    };
    scriptType_e type;
};

typedef void(__cdecl* Notify_fp)(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue* top);
typedef void(__cdecl* SV_GameSendServerCommand_t)(int clientNum, int reliable, char* command);
typedef const char* (__cdecl* ConvertToString_t)(unsigned int stringValue);
typedef unsigned int(__cdecl* GetSelf_t)(unsigned int threadId);
typedef void(__cdecl* Cbuf_AddText_t)(int localClientNum, const char* text);
typedef void(__cdecl* OpenMenu_t)(int localClientNum, const char* menuName);
typedef void(__cdecl* StartMatch_t)(void* party, int localControllerIndex);
typedef void(__cdecl* SV_SpawnServer_t)(char* name, int, int);
typedef void(__cdecl* BalanceTeams_t)(void* PartyData_s_party);
typedef void(__cdecl* MapRestart_t)(int fastRestart, int unknown);
typedef void(__cdecl* Cbuf_AddCall_t)(int a1, void* a2);

extern Notify_fp oNotify;
extern SV_GameSendServerCommand_t SV_GameSendServerCommand;
extern ConvertToString_t ConvertToString;
extern GetSelf_t GetSelf;
extern Cbuf_AddText_t Cbuf_AddText;
extern OpenMenu_t OpenMenu;
extern StartMatch_t Host_StartMatch;
extern SV_SpawnServer_t SV_SpawnServer;
extern BalanceTeams_t BalanceTeams;
extern MapRestart_t MapRestart;
extern Cbuf_AddCall_t Cbuf_AddCall;

namespace engine {
    const uintptr_t BASE_ADDRESS = 0x1B0E15C;
    const uintptr_t PLAYER_STRUCT_SIZE = 0x366C;
    const uintptr_t PLAYER_INDEX_ADDR = 0x9047FA0;

    const uintptr_t OFF_POS_X = 0x00;
    const uintptr_t OFF_POS_Y = 0x04;
    const uintptr_t OFF_POS_Z = 0x08;
    const uintptr_t OFF_FORCE_Y = 0x0C;
    const uintptr_t OFF_FORCE_X = 0x10;
    const uintptr_t OFF_FORCE_Z = 0x14;
    const uintptr_t OFF_LEAN = 0x3C;
    const uintptr_t OFF_ANGLES_Y = 0x44;
    const uintptr_t OFF_ANGLES_X = 0x48;
    const uintptr_t OFF_ROLL = 0x4C;
    const uintptr_t OFF_NAME = 0x3200;
    const uintptr_t OFF_TEAM = 0x31C0;

    template <typename T>
    inline T Read(uintptr_t address) { return *(T*)address; }

    template <typename T>
    inline void Write(uintptr_t address, T value) { *(T*)address = value; }

    uintptr_t GetPlayerBase(int index);
    int       GetLocalIndex();
    void      SetForce(int index, float x, float y, float z);
    void      SetLocation(int index, float x, float y, float z, float viewx, float viewy);
    Position  GetLocation(int index);

    void InstallHooks();
    void RemoveHooks();
    void* BuildTrampoline(void* target, size_t len);
    void PlaceHook(void* target, void* detour, size_t len);
    void __cdecl Notify_hk(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue* top);
}