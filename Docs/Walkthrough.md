# The walkthrough, chapter by chapter

The game's structure follows the thirteen chapters of the Zelda Dungeon
*Majora's Mask* walkthrough: four temples with an exploration or collection
chapter between each pair, and the moon at the end. Names, cast and copy are
this project's own; what is borrowed is the shape.

The walkthrough itself is at
<https://www.zeldadungeon.net/majoras-mask-walkthrough/>. It could not be
fetched from this project's build environment, whose egress policy blocks that
host, so its chapter list was recovered from search results instead. Anyone
filling in more detail should read the original directly.

## Chapters and regions

Each chapter is one row of `Content/Data/DT_Regions.csv`.

| # | Chapter | Region id | Region | Form it wants |
| --- | --- | --- | --- | --- |
| 1 | First three days | `bellwether.town` | Bellwether Town | Traveller |
| 2 | Southern swamp | `mirefen.swamp` | Mirefen Swamp | Sapling |
| 3 | First temple | `marshwood.approach` | Marshwood Rise | Sapling |
| 4 | Collection | `bellwether.outskirts` | Bellwether Outskirts | Traveller |
| 5 | The mountain | `frostcrown.pass` | Frostcrown Pass | Boulderkin |
| 6 | Second temple | `frostcrown.summit` | Frostcrown Summit | Boulderkin |
| 7 | Thaw, ranch and barrow | `thaw.fields` | Thawing Fields | Traveller |
| 8 | The bay | `tidebreak.coast` | Tidebreak Coast | Tideborn |
| 9 | Third temple | `tidebreak.reef` | Tidebreak Reef | Tideborn |
| 10 | The canyon | `ashen.canyon` | Ashen Canyon | Traveller |
| 11 | Well and keep | `ashen.keep` | Ashen Keep | Traveller |
| 12 | Fourth temple | `sunkenspire.base` | Sunken Spire | Traveller |
| 13 | The moon | `moon.field` | The Moon | Traveller |

## Temples

One row each in `DT_Dungeons.csv`. Beating a temple frees its echo; all four
echoes are what the Oath of Concord needs to be answered.

| Chapter | Temple | Form | Echo | Guardian | Item |
| --- | --- | --- | --- | --- | --- |
| 3 | Marshwood Temple | Sapling | Marshwood | Bogblade Dancer | Hero's Bow |
| 6 | Frostcrown Temple | Boulderkin | Frostcrown | Ironhoof | Fire Arrow |
| 9 | Tidebreak Temple | Tideborn | Tidebreak | Maw of the Reef | Ice Arrow |
| 12 | Sunken Spire Temple | Traveller | SunkenSpire | The Twinned Worm | Light Arrow |

## The five forms

Defined in `Source/MaskGame/Rules/MaskRules.cpp`, not in the character class.

| Form | Given by | Moves like |
| --- | --- | --- |
| Traveller | — | Sword, shield, bow. The only face a regular mask will sit on. |
| Sapling | Sapling Mask | Light and fragile. Skips across water, launches from flower pads, cannot swim. |
| Boulderkin | Boulderkin Mask | Heavy. Rolls at more than double running speed, ignores heat, walks the bottom of any water. |
| Tideborn | Tideborn Mask | Swims, breathes underwater, throws finned blades. |
| Wrath | Wrath Mask | Eight times the damage and a quarter taken. Refused outside a boss's arena. |

## Masks

Twenty-four, all in `DT_Masks.csv` with the chapter and region they come from
and a line on how they are earned. Four are transformation masks; the other
twenty are worn over the traveller's own face.

## Songs

Ten, in `DT_Songs.csv`. Note sequences live in `SongMatcher.cpp`, and a test
enforces that no song's notes occur inside another's — otherwise the shorter one
would fire mid-performance and swallow the longer.

The Hymn of Return has three readings, which is the whole time system in one
song:

- **Forwards** — back to the first dawn. Masks, songs, echoes, hearts,
  equipment and the bank survive; the purse, the pouches, the temple keys and
  every non-permanent flag do not.
- **Backwards** — time runs at a third speed for the rest of the cycle.
- **Doubled** — skip to the next dawn or dusk.

## Side quests

Thirty-one in `DT_Quests.csv`, each with the cycle window it can be done in and
the permanent flag it sets. The three-day windows are what make the notebook
worth keeping: the bomb bag thief only strikes on the first night, the
pawnbroker only opens between midnight and dawn, the herd is only taken on the
second night, and the two halves of the couple are only in the same room on the
last night.

`DT_Schedules.csv` gives every one of those NPCs a timetable across the whole
cycle, including branches gated on quest flags, so that finishing an errand
visibly changes where somebody spends their day.
