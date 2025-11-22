# Asymptomagical - Multiplayer Systems & Hex-Grid Tech Sample (UE5, C++)

Asymptomagical is a UE5 multiplayer group project.
My main focus in this codebase is **networked gameplay architecture**, especially a **replicated hex-grid built on Instanced Static Meshes**, plus a **deep C++ Gameplay Ability System (GAS) integration** and clean session/user infrastructure.

The original goal was a small multiplayer prototype; the implementation evolved into a showcase of scalable UE patterns.

- **Engine / Stack:** Unreal Engine 5 (C++ + Blueprints), GAS, CommonUI, Online Subsystem  
- **Focus areas:** multiplayer-ready systems, bandwidth-efficient replication, reusable GAS framework, editor-friendly workflows

---

## Core concepts

- **Replicated Hexagonal Grid using Fast Array delta replication**
  - Server-authoritative tile state replicated via `FFastArraySerializer`
  - Rendering through `UInstancedStaticMeshComponent` with per-tile data driving visuals
- **GAS as the backbone for character actions and stats**
  - PlayerState-owned ASC for players, pawn-owned ASC for AI
  - Data-driven `AbilitySet` pattern for loadouts
- **Decoupled user/session management via GameInstanceSubsystems**
  - Clean facade over Online Subsystem (create/find/join sessions)
  - Async user/login init separated from gameplay
- **Modern UI framework using CommonUI**
  - Activatable widgets + widget stacks for robust navigation/input routing

---

## Architecture highlights

### 1) Hex Grid + Network-Optimized ISM Replication (`HexagonalGrid/`)
The hex grid is managed by a single server-authoritative actor, avoiding one-actor-per-tile overhead.

**Key idea:** tile state lives in a replicated fast array, so clients receive **only deltas** for tiles that change.
This keeps bandwidth low and updates scalable even for large boards.

- **`AHexGrid`**
  - Owns the authoritative tile array (`FTileDataArray`) replicated using `FFastArraySerializer`
  - Exposes gameplay entry points like `SetTagsOnTile(TileIndex, NewTags)`  
  - Editor-side generation utilities (`CallInEditor`) for rapid iteration:  
    `CreateHexGrid()`, `RaiseRim()`, `RandomizeHeight()`, `Clear()`
- **Rendering**
  - Uses **engine `UInstancedStaticMeshComponent`** (`ISMC`) to render all tiles as instances
  - Visual state (materials/colors) is driven by replicated tile data + tags on clients
- **Replication lifecycle (example)**
  1. Ability targets a tile → calls `SetTagsOnTile()` on server  
  2. Tile data mutates in `TileArray`  
  3. Fast Array computes a delta and replicates only that tile’s change  
  4. Clients update local tile data and patch the corresponding ISMC instance

> Note: `UHexInstancedStaticMeshComponent` exists as a thin placeholder from an earlier design;
> the current implementation uses the standard ISMC directly for clarity and stability.

---

### 2) Gameplay Ability System (GAS) (`AbilitySystem/`)
GAS is integrated in C++ as a reusable, multiplayer-safe foundation.

- **Custom GAS spine**
  - `AsymAbilitySystemComponent` – project ASC hub  
  - `AsymGameplayAbility` – shared base for costs, cooldowns, activation patterns  
  - `AsymAttributeSet` – replicated stats with clamping + effect handling  
  - `AsymAbilitySet` – data asset bundling loadouts (abilities/effects/attributes)
- **ASC ownership best practice**
  - **Players:** ASC on `AsymPlayerState` for persistence across respawns  
  - **AI:** ASC on `AsymCharacter` (pawn-owned) for simplicity.

---

### 3) EsotericUser / EsotericSession Plugins (`Plugins/EsotericUser/`)
These subsystems handle identity + sessions as pure infrastructure, independent of gameplay.
(The User plugin is highly inspired by Lyra's common-user plugin)

The “Esoteric” naming is inherited from a previous project where this plugin started, but the implementation here
is generic so that it works for Asymptomagical’s multiplayer flow.

- **`UEsotericUserSubsystem`**
  - GameInstance-lifetime user/login state
  - Async init action to avoid blocking the game thread
- **`UEsotericSessionSubsystem`**
  - Clean facade over OSS session APIs
  - UI-friendly methods for create/find/join

---

### 4) CommonUI Framework (`UI/`)
Uses Unreal’s modern UI stack model.

- `UAsymActivatableWidget` / `UAsymActivatableWidgetStack`
  - Modular screens pushed/popped with correct input routing
  - Supports keyboard/mouse/gamepad cleanly

---

## Additional tooling note (not included)
Alongside this project I prototyped a separate **ISM/HISM instance editing tool** in a custom editor mode.  
It’s excluded from this public snapshot because it’s being cleaned up and stabilized as a standalone editor utility.

---

## Credits / contributions
- **Programming & systems:** Nicolas Martin  
- **Design:** Vladimir Eck, Alexander Kharkovski
(Team development was done in Perforce; this repo is a curated public snapshot.)
