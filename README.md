# Asymptomagical - Multiplayer Systems & Hex-Grid Tech Sample (UE5, C++)

<div align="center">
  <img src="Docs/gridutility.gif" alt="Hex grid editor utility GIF" width="85%" />
  <br/>
  <em>In-editor hex grid generation and edit workflow.</em>
</div>

Asymptomagical is a UE5 multiplayer group project.  
My main focus in this codebase is **networked gameplay architecture**, especially a **replicated hex-grid built on Instanced Static Meshes**, alongside a **deep C++ Gameplay Ability System (GAS) integration** and clean session/user infrastructure.

- **Engine / Stack:** Unreal Engine 5 (C++ & Blueprints), GAS, CommonUI, Online Subsystem  
- **Focus areas:** multiplayer-ready systems, bandwidth-efficient replication, reusable GAS framework, editor-friendly workflows

> **Note on assets:** The `/Content` folder is intentionally not included in this public snapshot.  
> The project uses licensed assets (e.g., marketplace/Fab content) and the full content directory is too large to distribute here.  
> This repository is therefore focused on the **C++ systems and architecture**; it is not directly runnable without providing your own assets.

**Project demos (YouTube):**  
Short clips showcasing key systems from this project.

- **[Hex grid editor utility / ISM instance workflow](https://www.youtube.com/watch?v=33LPMI9CQf8)** – in-editor grid generation and per instance transform modification.
- **[FastArray grid replication + gameplay tags demo](https://www.youtube.com/watch?v=5apv5UkOmcA)** – delta replication of tiles with tag-driven visual/state updates.

---

## Core concepts

- **Replicated Hexagonal Grid using Fast Array delta replication**
  - Server-authoritative tile state replicated via `FFastArraySerializer`
  - Rendering through `UInstancedStaticMeshComponent`, driven by per-tile data
- **GAS as the backbone for character actions and stats**
  - PlayerState-owned ASC for players, pawn-owned ASC for AI
  - Data-driven `AbilitySet` pattern for loadouts
- **Decoupled user/session management via GameInstanceSubsystems**
  - Clean interface over Online Subsystem (create/find/join sessions)
  - Async user/login init separated from gameplay
- **Modern UI framework using CommonUI**
  - Activatable widgets + widget stacks for robust navigation/input routing

---

## Architecture highlights

### 1) Hex Grid + Network-Optimized ISM Replication (`HexagonalGrid/`)
The hex grid is managed by a single server-authoritative actor, avoiding one-actor-per-tile overhead.

**Key idea:** tile state lives in a replicated fast array, so clients receive **only deltas** for tiles that change.  
This keeps bandwidth low and scalable even for large boards.

- **`AHexGrid`**
  - Owns the authoritative tile array (`FTileDataArray`) replicated using `FFastArraySerializer`
  - Exposes gameplay entry points like `SetTagsOnTile(TileIndex, NewTags)`  
  - Editor generation utilities (`CallInEditor`) for rapid iteration:  
    `CreateHexGrid()`, `RaiseRim()`, `RandomizeHeight()`, `Clear()`
- **Rendering**
  - Uses engine **`UInstancedStaticMeshComponent`** (`ISMC`) to render all tiles as instances
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
  - **AI:** ASC on `AsymCharacter` (pawn-owned) for simplicity

---

### 3) EsotericUser / EsotericSession Plugins (`Plugins/EsotericUser/`)
These subsystems handle identity + sessions as pure infrastructure, independent of gameplay (inspired by Lyra’s Common User pattern).
The name prefix comes from another project I initially developed the plugins for.

- **`UEsotericUserSubsystem`**
  - GameInstance-lifetime user/login state
  - Async init action to avoid blocking the game thread
- **`UEsotericSessionSubsystem`**
  - Clean interface over OSS session APIs
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

Team development was done in Perforce; this repository is a curated public snapshot.
