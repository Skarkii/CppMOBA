# CppMOBA

A multiplayer MOBA prototype built with C++23, raylib, and ENet.

## Features

- **Client-server architecture** over UDP (ENet) with reliable commands and unreliable state snapshots at ~30 tps
- **5 champions**: Barbarian, Knight, Mage, Ranger, Rogue — each with unique stats (HP, AD, range, move speed)
- **3D rendering** with raylib: GLB models, isometric camera, world-space health bars, grid floor
- **Combat**: right-click to move or attack, range validation, projectiles, team-based targeting (Blue vs Red)
- **HUD**: health/mana bars, Q/W/E/R ability slots with cooldown display, stat panel
- **Pause menu** with settings: fullscreen, FPS cap, volume, keybind customization
- **Debug overlay** (compile with `-DMOBA_DEBUG_OVERLAY=ON`): FPS, ping, jitter, packet loss, throughput
- **Lua scripting** (sol2): framework in place for data-driven ability definitions (see `assets/abilities/arrow.lua`)

## Project Structure

```
src/
  shared/    # Networking (ENet wrapper), protocol, serialization, champion data
  server/    # Authoritative game loop, player state, tick updates
  client/    # Rendering, input, HUD, menus, debug overlay
assets/
  champions/ # GLB 3D models
  animations/# Animation rigs
  abilities/ # Lua ability scripts
  fonts/     # UI font (Phantom.otf)
```

## Building

### Nix (recommended)

Dev shell (build with cmake yourself):
```sh
nix develop
cmake -B build -G Ninja
cmake --build build -j$(nproc)
```

Standalone build:
```sh
nix build
./result/bin/MOBA_server
./result/bin/MOBA_client
```

### CMake (manual)

Requires: CMake 3.25+, C++23 compiler, and raylib's system dependencies (X11/Wayland, OpenGL).
Dependencies (ENet, raylib, Lua, sol2) are fetched automatically via FetchContent.

```sh
cmake -B build -G Ninja
cmake --build build -j$(nproc)
./build/MOBA_server
./build/MOBA_client
```

## Controls

| Input | Action |
|-------|--------|
| Right click | Move / Attack target |
| B | Recall (3s channel) |
| X | Show attack range |
| Scroll wheel | Zoom camera |
| Escape | Pause menu |
| F3 | Debug overlay (if compiled in) |

## Networking

- **Port**: 8000
- **Protocol**: versioned, two-channel UDP — reliable for commands, unreliable for state
- **Auth**: token-based handshake with protocol version check
- Server is authoritative; clients send movement/attack/recall commands
