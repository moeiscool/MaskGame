# Setup: from nothing to playing

This guide assumes you have never used Unreal Engine. Follow it top to bottom
and you will end up walking around the game with a controller or a keyboard.

Budget about **90 minutes**, most of which is downloads and one long compile.
The steps that actually need your attention take maybe fifteen minutes.

**Contents**

1. [What you are installing, and why](#1-what-you-are-installing-and-why)
2. [Install the tools](#2-install-the-tools)
3. [Get the code](#3-get-the-code)
4. [Build it once](#4-build-it-once)
5. [Open the editor](#5-open-the-editor)
6. [Import the six data tables](#6-import-the-six-data-tables)
7. [Make a level](#7-make-a-level)
8. [Press Play](#8-press-play)
9. [Controls](#9-controls)
10. [Your first ten minutes](#10-your-first-ten-minutes)
11. [Console commands](#11-console-commands)
12. [When something goes wrong](#12-when-something-goes-wrong)
13. [Making a standalone build](#13-making-a-standalone-build)
14. [Playing together, and on a phone](#14-playing-together-and-on-a-phone)

---

## 1. What you are installing, and why

Three things, and it helps to know what each one is for:

| Thing | What it is | Why you need it |
| --- | --- | --- |
| **Epic Games Launcher** | Epic's app store | The only supported way to install Unreal Engine |
| **Unreal Engine 5.5** | The game engine — editor, renderer, physics | Runs the game |
| **A C++ compiler** | Visual Studio on Windows, Xcode on Mac | This project is written in C++, so it has to be compiled before the editor can open it |

That last row is the part people miss. A project with C++ in it is not just
opened — it is **built** first. If you skip the compiler, the editor will refuse
to open the project with a message about missing modules.

---

## 2. Install the tools

### Windows

**a. Visual Studio 2022 — install this first.**

Unreal needs the compiler present *before* it installs, or it will not wire
itself up to it.

1. Download the free **Community** edition from
   <https://visualstudio.microsoft.com/downloads/>.
2. Run the installer. You will land on a screen of "Workloads" — these are
   bundles of tools. Tick exactly these two:
   - **Game development with C++**
   - **Desktop development with C++**
3. On the right-hand panel under *Installation details*, make sure these are
   ticked (they usually are by default):
   - **MSVC v143 - VS 2022 C++ x64/x86 build tools**
   - **Windows 11 SDK** (any version)
4. Install. It is roughly 10 GB and takes a while.

> You never need to *open* Visual Studio. Unreal drives it in the background.
> Installing it is the whole job.

**b. Epic Games Launcher and Unreal Engine 5.5.**

1. Download from <https://www.unrealengine.com/en-US/download> and install.
2. Make a free Epic account and sign in.
3. Click the **Unreal Engine** tab on the left, then the **Library** tab
   along the top.
4. Click the **+** next to *Engine Versions*, pick **5.5.x** from the dropdown,
   and click **Install**.
5. Before it starts, click **Options** and make sure these are ticked:
   - **Engine Source** (not strictly required, but makes debugging possible)
   - **Starter Content** — leave this ticked, it is small and harmless
6. Install. It is about 60 GB and will take a long time.

**c. Git.**

Download from <https://git-scm.com/download/win> and install with all defaults.

### macOS

1. Install **Xcode** from the Mac App Store (large; leave it running).
2. Open a Terminal and run `sudo xcode-select --install`, then open Xcode once
   and accept the licence prompt. Unreal will not build until you have.
3. Install the **Epic Games Launcher** from
   <https://www.unrealengine.com/en-US/download> and install **Unreal Engine
   5.5** through Library → **+** → Install, as in the Windows steps above.
4. Git comes with the Xcode command line tools you installed in step 2.

> Apple Silicon Macs run this fine. There is no extra step.

### Linux

There is no Epic Games Launcher for Linux. Instead:

1. Install a compiler and Git: `sudo apt install clang build-essential git`
   (or your distribution's equivalent).
2. Download the **Linux Unreal Engine 5.5** archive from
   <https://www.unrealengine.com/en-US/linux> and unpack it somewhere with
   plenty of room — say `~/UnrealEngine`.
3. Run `~/UnrealEngine/Setup.sh` once, then
   `~/UnrealEngine/GenerateProjectFiles.sh`.

Throughout the rest of this guide, wherever it says "the engine folder", that
means wherever you unpacked it.

---

## 3. Get the code

Open a terminal (on Windows: **Git Bash**, installed with Git) and run:

```sh
git clone https://github.com/moeiscool/MaskGame.git
cd MaskGame
git checkout claude/zelda-majoras-mask-ue5-jv39rg
```

You now have a folder called `MaskGame` with a `MaskGame.uproject` file in it.
That `.uproject` is the thing you will eventually double-click.

---

## 4. Build it once

You can let the editor do this, or do it yourself. **Doing it yourself is
better**, because if something goes wrong you get a readable error instead of a
dialog box that says "the project failed to compile".

### Windows

Open Git Bash in the `MaskGame` folder and run this as one line:

```sh
"/c/Program Files/Epic Games/UE_5.5/Engine/Build/BatchFiles/Build.bat" \
    MaskGameEditor Win64 Development -project="$PWD/MaskGame.uproject" -waitmutex
```

If you installed the engine somewhere else, change the path to match.

### macOS

```sh
"/Users/Shared/Epic Games/UE_5.5/Engine/Build/BatchFiles/Mac/Build.sh" \
    MaskGameEditor Mac Development -project="$PWD/MaskGame.uproject"
```

### Linux

```sh
~/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh \
    MaskGameEditor Linux Development -project="$PWD/MaskGame.uproject"
```

**What to expect.** The first build compiles a lot and takes **10 to 40 minutes**
depending on your machine. You want the last line to say:

```
Total time in Parallel executor: ...
Build succeeded.
```

If it does not, jump to [section 12](#12-when-something-goes-wrong).

---

## 5. Open the editor

Double-click `MaskGame.uproject`.

The first open takes several minutes — Unreal is compiling shaders. There is a
progress bar. Let it finish.

**You will probably see a warning that the default map could not be found.**
That is expected and harmless: the project is configured to open a level called
`L_Vespera` that does not exist yet, because you are about to make it in
[section 7](#7-make-a-level). Dismiss it. You will be looking at an empty grey
editor.

### A one-minute tour of what you are looking at

| Panel | Where | What it is |
| --- | --- | --- |
| **Viewport** | The big 3D area in the middle | The level you are editing |
| **Content Browser** | Bottom, or **Window → Content Browser** | Every asset in the project — think of it as the project's file explorer |
| **Outliner** | Top right | A list of everything in the current level |
| **Details** | Bottom right | Settings for whatever you have selected |
| **Output Log** | **Window → Output Log** | The engine talking to you. Open this now and leave it open |

Right-drag in the viewport to look around; `W A S D` while right-dragging to fly.

---

## 6. Import the six data tables

The game's content — the thirteen chapters, the temples, the masks, the songs,
the quests, the NPC timetables — lives in `Content/Data` as spreadsheet files.
Unreal needs each one imported once.

**Do this six times, once per row of the table below.**

1. In the **Content Browser**, click the **+ Add** button → **Import to /Game...**
2. Navigate to the `Content/Data` folder inside your `MaskGame` checkout and
   pick the CSV file.
3. A dialog appears. Set:
   - **Import As**: `DataTable`
   - **Choose DataTable Row Type**: the struct named in the table below
4. Click **Apply**.
5. The new asset appears in the Content Browser. **Drag it into a folder called
   `Data`** if it did not land there — create one by right-clicking in the
   Content Browser → **New Folder**.

| CSV file | Row type to choose | What it holds |
| --- | --- | --- |
| `DT_Regions.csv` | `RegionTableRow` | The 13 chapters as places |
| `DT_Dungeons.csv` | `DungeonTableRow` | The 4 temples and their guardians |
| `DT_Masks.csv` | `MaskTableRow` | All 24 masks |
| `DT_Songs.csv` | `SongTableRow` | All 10 songs |
| `DT_Quests.csv` | `QuestTableRow` | 31 side quests and their time windows |
| `DT_Schedules.csv` | `ScheduleTableRow` | 79 entries of who is where, when |

**The names must match exactly.** The project looks for `/Game/Data/DT_Regions`
and so on. If an import produced `DT_Regions_CSV` or landed in the wrong folder,
rename or move it.

**Now check they are wired up.** Go to **Edit → Project Settings**, and in the
left-hand list find **Game → Mask Game**. You should see six slots, each already
filled in with the matching table. If any slot is empty, click it and pick the
table from the dropdown.

Save everything: **Ctrl+S**, or **File → Save All**.

---

## 7. Make a level

The project has no levels yet. You are going to make an empty one — the game
builds its own world into it at runtime.

1. **File → New Level**, then choose **Empty Level**.
2. In the **Place Actors** panel (**Window → Place Actors** if it is not
   showing), search for `Player Start` and drag one into the viewport.
3. Select the Player Start you just placed. In the **Details** panel on the
   right, find **Transform → Location** and type in exactly:
   - **X** `0`
   - **Y** `0`
   - **Z** `200`

   That is the middle of the first chapter's town square, two metres up.
4. **File → Save Current Level As**. Create a folder called `Maps` and save it
   as `L_Vespera`. The full path must end up as `/Game/Maps/L_Vespera`.

> **Do I need to add a light?** No. The game spawns and drives its own sun,
> because the sun's position *is* the clock. If you add your own directional
> light, the game will adopt yours instead of spawning one.

> **Do I need to set the game mode?** No. It is already set project-wide.

---

## 8. Press Play

Click the **▶ Play** button in the toolbar, or press **Alt+P**.

**What should happen:** you drop onto a grey platform under a low morning sun,
with a tall obelisk off to one side, a chest, and a couple of enemies wandering
about. Off in the distance, more platforms — those are the other twelve
chapters. Walk east (the open side of every region) to reach them.

**Check the Output Log.** You should see something like:

```
LogMaskGame: Loaded 79 schedule entries.
LogMaskGame: Sky ready: 4 actors spawned, the rest adopted from the level.
LogMaskGame: Greybox world built: 13 regions, 4 dungeons, 107 actors.
```

The counts come straight from your data tables, so they will match the numbers
above unless you have edited the CSVs.

If you see `The region table is empty; nothing to build`, go back to
[section 6](#6-import-the-six-data-tables) — the tables did not import.

Press **Escape** to stop playing.

---

## 9. Controls

Plug a controller in before you press Play and it just works — Xbox, PlayStation
and most generic pads are all handled by the engine. You can switch between
controller and keyboard mid-game.

| Action | Keyboard | Controller |
| --- | --- | --- |
| Move | `W A S D` | Left stick |
| Look | Mouse | Right stick |
| Jump | `Space` | **A** / **Cross** |
| Interact | `E` | **B** / **Circle** |
| Attack | Left mouse | **X** / **Square** |
| Form action (hold) | `Left Shift` | **Right trigger** |
| Lock on / release | `Q` | **Left bumper** |
| Draw or put away ocarina | `Tab` | **Y** / **Triangle** |
| Mask slots 1–4 | `1` `2` `3` `4` | **D-pad** up / right / down / left |
| Take mask off | `0` | **View** / **Share** |

On a phone or tablet the same actions appear as on-screen buttons; see
[section 14](#14-playing-together-and-on-a-phone).

**While the ocarina is out**, the controls above are put away and the same
buttons play notes instead:

| Note | Keyboard | Controller |
| --- | --- | --- |
| Up, Down, Left, Right | Arrow keys | D-pad |
| A | `Space` | **A** / **Cross** |

Time stands still while the ocarina is drawn, so you can take as long as you
like over a song.

**About lock-on.** Press it near an enemy and the camera holds them; your
character then faces them and circles instead of turning to face where it is
walking. Press again, walk far enough away, or kill the target, and it releases.

**Stick feel not right?** Open `Config/DefaultInput.ini`. Dead zones and
sensitivity are the `AxisConfig` lines at the top; turn speed and Y inversion
are properties on `AMaskCharacter` (`GamepadTurnRate`, `GamepadLookUpRate`,
`bInvertGamepadLookY`).

---

## 10. Your first ten minutes

The world is a greybox, so there is nothing to look at — but every system is
live. Here is a tour that shows you each of them:

1. **Watch the clock.** One in-game hour takes 45 real seconds, so a full
   three-day cycle is 54 minutes. Watch the sun climb, cross and set. Night is
   18:00 to 06:00.
2. **Open the chest** in the first region (walk up, press Interact). It contains
   the magic meter, which the transformed forms need for their abilities.
3. **Touch the owl statue** near a region's corner. That saves your game and
   records the place.
4. **Get a mask.** Open the console with **`** (backtick) and type
   `MaskGame.GiveAllMasks`, then press **1**, **2** or **3** to put one on. Feel
   the difference: the Boulderkin is slow but rolls at more than double speed on
   the right trigger; the Sapling is quick and fragile.
5. **Try to wear a regular mask while transformed.** Press **4** while you are a
   Boulderkin. It refuses — a regular mask needs a face of its own to sit on.
6. **Play a song.** Type `MaskGame.LearnAllSongs`, then draw the ocarina and
   play the Hymn of Return: **Up, Left, Right, Up, Left, Right**. The cycle
   restarts at the first dawn — and you keep the masks.
7. **See what a rewind takes.** Before rewinding, note your rupee count. After,
   it is zero. Masks stay, money does not.
8. **Jump to the end.** Type `MaskGame.SetTime 3 0`. You are now in the final
   hours, and the sky goes red as the clock runs down.
9. **See the town's timetable.** Type `MaskGame.Where` and read the Output Log:
   it prints who is standing in each region at this exact minute. Change the
   time and run it again — people move.

---

## 11. Console commands

Press **`** (backtick, above Tab) while playing to open the console. Type
`MaskGame.` to see them all listed.

| Command | What it does |
| --- | --- |
| `MaskGame.GiveAllMasks` | Grants all 24 masks |
| `MaskGame.GiveMask <name>` | Grants one, e.g. `MaskGame.GiveMask Tideborn` |
| `MaskGame.LearnAllSongs` | Learns all 10 songs |
| `MaskGame.SetTime <day> <hour> [minute]` | Moves the clock, e.g. `MaskGame.SetTime 3 0` |
| `MaskGame.Rewind` | Restarts the cycle, keeping what is permanent |
| `MaskGame.Where` | Prints the time and who is in each region right now |

Two engine commands worth knowing: `stat fps` shows the frame rate, and
`ShowDebug` toggles debug overlays.

---

## 12. When something goes wrong

### "The following modules are missing or built with a different engine version: MaskGame"

The C++ has not been compiled. Click **Yes** to rebuild if offered; if that
fails, close the editor and run the build command from
[section 4](#4-build-it-once) yourself so you can read the actual error.

### The build fails with errors about a missing compiler or SDK

On Windows, the Visual Studio workloads from [section 2](#2-install-the-tools)
are missing. Re-run the Visual Studio Installer, click **Modify**, and tick
**Game development with C++** and **Desktop development with C++**.

On macOS, run `sudo xcode-select --install`, then open Xcode once and accept the
licence.

### The screen is black when I press Play

Check the Output Log for `Sky ready:`. If it is missing, the game mode is not
running — open **Window → World Settings** and confirm **GameMode Override** is
either *None* (which uses the project default) or `MaskGameMode`.

### I fall forever / I spawn inside a wall

The Player Start is in the wrong place. Select it in the Outliner and set its
location to exactly `0, 0, 200` as in [section 7](#7-make-a-level).

### "The region table is empty; nothing to build"

The data tables are not imported, or not assigned. Redo
[section 6](#6-import-the-six-data-tables), paying attention to the asset names
and the **Project Settings → Game → Mask Game** check at the end.

### My controller does nothing

Confirm the operating system sees it first — on Windows, search for
"Set up USB game controllers" and check it registers there. Then make sure the
game viewport has focus: click inside it once. Note that in the editor, some
controller buttons are captured by the editor itself unless you are playing in
**New Editor Window** mode (the dropdown next to the Play button).

### The editor is very slow

The first run after a build compiles shaders. It settles down. If it stays slow,
lower the viewport quality: the **Settings** dropdown in the viewport toolbar →
**Engine Scalability Settings** → **Medium**.

---

## 13. Making a standalone build

Once it runs in the editor you can package a version that runs on its own. The
easiest way is the script:

```sh
python3 Scripts/package.py --platform win64      # or mac, linux, android, ios
```

A first package takes 20–40 minutes and lands in `Build/<Platform>`. You get a
folder with an executable in it that needs no engine installed.

Android and iOS need their own SDKs installed first, and a Mac or iOS build has
to be made on a Mac. All of that is in
[`Docs/Packaging.md`](Docs/Packaging.md) — including the one step people get
wrong on Android, which is letting Unreal's `SetupAndroid` script pick the SDK
and NDK versions instead of choosing them by hand.

## 14. Playing together, and on a phone

### Split-screen

On Windows, macOS and Linux, plug in a second controller and press **A** or
**Start** on it. The screen splits and a second player joins, up to four.

Everyone shares one cycle, one set of masks and one notebook — the three days
belong to all of them, and any player can play the Hymn of Return to send
everybody back to the first dawn. One player going down does not end the cycle;
that only happens once nobody is left standing.

Joining is deliberately by button press rather than by plugging a pad in, so a
controller you left connected does not silently halve your screen.

### Touch

Android and iOS builds draw their own on-screen controls. You can try them on
your desktop build without a touchscreen — press **`** and type:

```
MaskGame.Touch.Force 1
```

The movement stick is *floating*: it appears wherever your left thumb lands in
the left of the screen rather than at a fixed spot, so you never have to look
down to find it. Looking is a swipe on the right. Drawing the ocarina swaps the
action buttons for the five notes, in the same positions.

Type `MaskGame.Touch.Force 0` to go back to the default.

---

## Appendix: running the tests without Unreal

The game's rules — the clock, the ocarina, what a rewind keeps, the forms, the
NPC schedules — are written as plain C++ with no engine dependency, so they can
be built and run without any of the above:

```sh
cmake -S Tests/Standalone -B build/tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

Seven suites, 4,852 assertions, about a second. If you are changing the rules,
the data tables or the packaging flags, this is the fast loop — you do not need
to open the editor at all.
