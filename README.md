# MaskGame

An Unreal Engine 5 action-adventure built around a three-day clock, four bodies
worn as masks, and a moon that lands whether or not you were ready.

The structure follows the thirteen-chapter shape of the Zelda Dungeon *Majora's
Mask* walkthrough — town, swamp, temple, interlude, mountain, temple, thaw, bay,
temple, canyon, keep, temple, moon — with its own cast, places and names, so the
project is distributable under its own licence rather than being a clone
carrying someone else's trademarks. `Docs/Walkthrough.md` maps every chapter,
temple, mask and song onto the source it came from.

## Status

The systems, the content tables and a runtime greybox are in. There are no
authored levels, meshes or audio yet: dropping `MaskGameMode` into an empty map
builds all thirteen regions and four temples out of the data tables at startup
and lets you walk them.

## Requirements

- Unreal Engine 5.5
- A C++ toolchain for your platform (Visual Studio 2022, Xcode, or clang)

## Building the game

```sh
# Windows
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" \
    MaskGameEditor Win64 Development -project="%CD%\MaskGame.uproject" -waitmutex

# Linux / macOS
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" \
    MaskGameEditor Linux Development -project="$PWD/MaskGame.uproject"
```

Then open `MaskGame.uproject`. On first open the editor will offer to build the
module; say yes.

The data tables are shipped as CSV under `Content/Data` and need importing once —
see `Docs/DataTables.md`, which lists the row struct each file maps to.

## Running the rules tests

The game's rules live in `Source/MaskGame/Rules` as plain C++ with no engine
dependency, so they build and test without Unreal installed:

```sh
cmake -S Tests/Standalone -B build/tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

Six suites, 4,780 assertions: the cycle clock, the ocarina matcher, the
progression rules, the mask and form tables, the NPC schedule table, and a
validator that cross-checks every row of every CSV against the enums in
`MaskGameTypes.h`.

## Controls

The greybox is bound through the fallback mappings in `Config/DefaultInput.ini`.

| Input | Action |
| --- | --- |
| `WASD` / mouse | Move and look |
| `Space` | Jump |
| `E` | Interact |
| `Left mouse` | Attack |
| `Left shift` | The current form's special action |
| `1`–`4` | Mask quick-slots |
| `0` | Take the mask off |
| `Tab` | Draw or put away the ocarina |
| Arrow keys, `Space` | The five ocarina notes, while it is drawn |

Time holds still while the ocarina is out.

## How it fits together

```
Source/MaskGame/
  Rules/        Engine-free game rules. No UObjects, no FString, no includes
                from Engine. Compiled into both the game module and the
                standalone tests, which is what makes them testable at all.
  Core/         Types bridging Rules to UObject land, the game instance that
                owns progression, the save game, the game mode that runs the
                cycle, project settings.
  Time/         UCycleSubsystem: turns the clock's movement into events.
  Masks/        Which mask is on, and whether it is allowed to be.
  Character/    The player, taking all its movement numbers from the form tables.
  Songs/        The ocarina.
  Quests/       The data tables, the schedules, and the notebook.
  World/        Statues, chests, temple gates, and the greybox generator.
  AI/           Enemies and temple guardians.
Content/Data/   The walkthrough, as six CSV data tables.
Tests/          The rules suites and the data validator.
Docs/           Chapter mapping and data table import steps.
```

Three rules hold the design together:

1. **The rewind is defined in one place.** `FProgressionState::ResetForNewCycle`
   decides what three days can and cannot take away. A flag named `perm.*`
   survives; nothing else does. Adding a field means choosing a side there.
2. **Forms are data.** Every movement, damage and ability number lives in
   `MaskRules.cpp`. Nothing in the character class hard-codes what a body feels
   like.
3. **Anything time-dependent listens to `UCycleSubsystem`.** Nothing counts its
   own seconds, so slowing time, skipping to dusk and rewinding all work
   everywhere for free.

## Licence

MIT. See `LICENSE`.
