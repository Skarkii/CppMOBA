# CppMOBA

A multiplayer MOBA prototype built with C++23, raylib, ENet and Lua.

## Features

- **Client-server** over UDP (ENet): the server is authoritative and runs at 30 ticks per second
- **5 champions**: Barbarian, Knight, Mage, Ranger, Rogue, defined in `src/shared/champions.hpp`
- **Lua abilities** (sol2): each ability is a file in `assets/abilities/` with its numbers, visuals, icon and an `onHit` hook
- **Combat**: basic attacks (melee and homing projectiles), skillshots, cooldowns, mana
- **Map**: three lanes, river and jungle, with a grid file the server uses to block movement into walls
- **Gold**: passive income and kill rewards
- **HUD**: health and mana bars, ability slots with icons and cooldowns, gold, stats
- **Chat** (team and `/all`), with kill messages
- **Pause menu**: fullscreen, VSync, FPS cap, volume, keybinds
- **Debug overlay** (`-DMOBA_DEBUG_OVERLAY=ON`): FPS, ping, packet loss, throughput

## Project structure

```
src/
  shared/    # Networking, protocol, serialization, champions, abilities, map grid
  server/    # Tick loop, players, projectiles, message handling
  client/    # Rendering, input, HUD, chat, menus
assets/
  abilities/ # Lua ability scripts
  champions/ # Champion models
  maps/      # Map model, grid and positions
  models/    # Towers, nexus, minions, monsters (not used in game yet)
  icons/     # Ability icons
  fonts/
```

## Building

Dependencies (ENet, raylib, Lua, sol2) are downloaded automatically by CMake. Requires CMake 3.25+ and a C++23 compiler.

### Windows (Visual Studio)

Open the folder in Visual Studio, pick the `x64-debug` preset and build. The executables end up in `out/build/x64-debug/`.

### Linux (CMake)

Also needs raylib's system dependencies (X11 or Wayland, OpenGL).

```sh
cmake -B build -G Ninja
cmake --build build
```

### Nix

```sh
nix develop   # dev shell, then build with cmake as above
nix build     # or a standalone build in ./result/bin
```

## Running

Start the server, then one client per player:

```sh
MOBA_server
MOBA_client 100
MOBA_client 101
```

The number is the player's **token**. There is no lobby yet, so the players of a match are hardcoded on the server in `src/server/match.cpp`, each with a token, name, champion and team:

| Token | Champion | Team |
|-------|----------|------|
| 100 | Barbarian | Blue |
| 101 | Ranger | Red |

A client must pass one of these tokens, otherwise the server rejects it. To change who plays, edit the `players` array and `playerCount` (in `match.hpp`) and rebuild.

- `MOBA_server -REQUIREPLAYERS` waits for all players to connect before the game starts (or 10 seconds, whichever comes first).
- The client connects to `127.0.0.1:8000`. Both programs load `assets/` from the folder they are started in; the build copies it next to the executables.

## Controls

| Input | Action |
|-------|--------|
| Right click | Move / attack target |
| Q W E R | Abilities, aimed at the cursor |
| B | Recall (3 seconds) |
| X (hold) | Show attack range |
| Scroll wheel | Zoom |
| Enter | Chat (`/all` for all chat) |
| Escape | Pause menu |
| F3 | Debug overlay (if compiled in) |
