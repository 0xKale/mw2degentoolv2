# DISMAY'S DEGEN TOOL — MW2 Internal

C++ DLL for **Call of Duty: Modern Warfare 2** multiplayer (`iw4mp.exe`) using **ImGui**, **DirectX 9**, and **MinHook**. Legit-focused tweaks (sensitivity, visuals, hosting, profile tools) — no wallhacks, aimbot or anything of that sort.

Successor to [mw2degentool](https://github.com/0xKale/mw2degentool).

![Menu](https://i.ibb.co/VWxn9FfY/ezgif-3af80ec641f41fbe.gif)

## Controls

| Key | Action |
|-----|--------|
| `INSERT` | Toggle menu |

## Menu

Tabs: **Main**, **Account**, **Host**, **Dedigamer**, **About**

- Notification system

### Main

- Text chat on/off
- Mouse 1:1 (FOV-min style sens)
- Iron sight intervention
- Sensitivity read/write
- FPS, FOV, and map size sliders
- Disconnect and in-game console command box
- DLC toggle and custom port (for VPN / port forwarding)
- Force team change
- In-game live stats overlay — toggle on/off; while in a match (not spectating) shows your name, kills, assists, deaths, kill streak, K/D ratio, FPS, ping, and local time
- Custom crosshair (color, outline, T-style, center dot, scale)
- Visual toggles: sun, camos, fog, bullets, movie mode, clear glass, ping text
- Fullbright presets (Invert, Normal, Super, Slight, Dullish)
- View model offset (gun X / Y / Z) and reset

### Account

- Prestige and rank sliders (send to profile)
- Unlock all (use in private match for full lobby unlock / spinning skull)
- Gold Deagle loadout classes
- Profile stats editor (wins, K/D, time played, etc.) — load and send

### Host

- Server command console (`s` / `c` / `f` / `J` / `M` / `n` prefixes documented in-menu)
- Per-client buttons, load player names, broadcast to all clients
- Map and gamemode picker, max players, change map, fast restart
- Lock lobby, match settings, start match
- FFA team fix and host hotkeys
- Lobby tweaks: unlimited ammo, unlimited time/score, sprint scale
- 360 prone/ladder cap, knockback and back-speed scale
- Depatch bounces (normal / easy) and depatch elevators

### Dedigamer

- Live server browser (players, map, gametype, uptime)
- Refresh, disconnect, reconnect to last join
- One-click join

### About

- Load / save config (`dismay_config.ini` next to the DLL)
- Menu accent color
- Credits and links

## Build

1. Install the [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812) and set the `DXSDK_DIR` environment variable.
2. Install **Visual Studio 2022** with the **Desktop development with C++** workload and **Windows 10/11 SDK**.
3. Open `dismays mw2 internal.sln` in Visual Studio.
4. Select **Release \| Win32** (x86) and build.

Output: `Build/mw2degentool.dll`

**VS Code / CLI:** run the default build task (`msbuild release x86`) or:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "dismays mw2 internal.sln" /p:Configuration=Release /p:Platform=x86
```

Inject `mw2degentool.dll` into `iw4mp.exe`.

## Requirements

| | |
|---|---|
| Output | DLL (`mw2degentool.dll`) |
| Platform | **Win32** (x86) — matches the game |
| Configuration | Release only (in solution) |
| Windows SDK | 10.0 |
| Toolset | v143 (VS 2022) |
| C++ standard | C++20 |
| Character set | Multi-Byte |
| DirectX | DirectX 9 + legacy DirectX SDK (`DXSDK_DIR`) |

## Layout

```
dismay's mw2 internal/
  src/
    dllmain.cpp
    menu/              — ImGui menu, DX9 hook, pages
    menu/pages/        — Main, Account, Host, Dedigamer, About
    dismay/            — features, config, dedigamer API
    game/              — hooks, offsets, structs
    framework/         — custom UI widgets
  ext/                 — imgui, minhook, fonts
Build/                 — compiled DLL
```

Config saves to `dismay_config.ini` in the same folder as the injected DLL.

## Screenshots

![Main tab](https://i.ibb.co/vxMyYRFx/iw4mp-Qt19yoauo-C.jpg)

![Account tab](https://i.ibb.co/GfWxv73r/iw4mp-o-Ybxsvbst-R.png)

![Host tab](https://i.ibb.co/3m5KYVhj/iw4mp-VKb-FTxe-Ejm.jpg)

![Dedigamer tab](https://i.ibb.co/mCpvTFCT/iw4mp-d-Km-NODb-Nmm.png)

![IronSight0](https://i.ibb.co/WWWFYLBS/iw4mp-Vika-AP5-Rj-J.jpg)

![IronSight1](https://i.ibb.co/GfQJDwgc/iw4mp-Os0q7d-WOWv.jpg)
