# Importing the data tables

The game's content lives in `Content/Data` as CSV so that it stays diffable and
can be validated outside the editor. Unreal needs each file imported once as a
`UDataTable` against the right row struct.

## Import steps

For each file below, in the Content Browser: **Import** → pick the CSV → choose
**DataTable** → set **Row Struct** to the struct named here → import to
`/Game/Data` keeping the file's name.

| CSV | Row struct | Rows | Holds |
| --- | --- | --- | --- |
| `DT_Regions.csv` | `RegionTableRow` | 13 | One per walkthrough chapter, with its greybox footprint |
| `DT_Dungeons.csv` | `DungeonTableRow` | 4 | The temples, their guardians and their echoes |
| `DT_Masks.csv` | `MaskTableRow` | 24 | Every mask, where it comes from and how it is earned |
| `DT_Songs.csv` | `SongTableRow` | 10 | Every song and what it does |
| `DT_Quests.csv` | `QuestTableRow` | 31 | Side quests, their cycle windows and their rewards |
| `DT_Schedules.csv` | `ScheduleTableRow` | 79 | Where each NPC is, hour by hour, across three days |

The imported assets are then wired up under **Project Settings → Game → Mask
Game**, which writes them into `Config/DefaultGame.ini`. The paths there already
point at `/Game/Data/DT_*`, so importing with the names above is enough.

## Editing the tables

Edit the CSV, not the imported asset, and re-import. Then run the validator:

```sh
python3 Tests/validate_data.py
```

It parses the enums out of `Source/MaskGame/Core/MaskGameTypes.h` and checks
every row against them and against each other: unknown enum entries, references
to regions that do not exist, masks awarded by two different quests, quest flags
that are not permanent, schedule windows written backwards, and NPCs who are
named by a quest but have no timetable. It runs as the `DataTables` suite under
ctest alongside the C++ tests.

## The one thing that catches everybody

Windows are half-open `[Start, End)` in cycle-elapsed minutes, and **the day
counter turns at 06:00, not at midnight**. A night belongs to the day whose
evening it began, so:

- An overnight window ends at `06:00` of the **next** day number:
  `(Day=1,Hour=18)` → `(Day=2,Hour=6)`.
- A window running only until midnight ends at `00:00` of its **own** day
  number: `(Day=1,Hour=6)` → `(Day=1,Hour=0)`.

Getting that backwards produces a window that ends before it starts, which
`FScheduleTable` silently treats as running to the moment of impact. The
validator rejects it instead.
