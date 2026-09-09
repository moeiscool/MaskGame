#!/usr/bin/env python3
"""Cross-check the walkthrough data tables against the C++ they are loaded into.

A data table is imported by name at editor time, so a typo in an enum entry or a
reference to a region that does not exist produces an empty field at runtime
rather than an error. This script is the check that turns those into failures:
it parses the enums out of MaskGameTypes.h and the cycle constants out of
CycleClock.h, then validates every row of every CSV against them.

Run directly, or through ctest as the DataTables suite.
"""

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "Content" / "Data"
TYPES_HEADER = ROOT / "Source" / "MaskGame" / "Core" / "MaskGameTypes.h"

failures: list[str] = []
checks = 0


def check(condition: bool, message: str) -> bool:
    global checks
    checks += 1
    if not condition:
        failures.append(message)
    return condition


def parse_enum(source: str, name: str) -> list[str]:
    """Pull the entry names out of a UENUM body."""
    match = re.search(rf"enum class {name} : uint8\s*\{{(.*?)\}};", source, re.DOTALL)
    if not match:
        raise SystemExit(f"could not find enum {name} in {TYPES_HEADER}")

    entries = []
    for line in match.group(1).splitlines():
        line = line.split("//")[0].strip()
        if not line or line.startswith("/*"):
            continue
        entry = re.match(r"([A-Za-z_]\w*)", line)
        if entry:
            entries.append(entry.group(1))
    return entries


def read_table(name: str) -> list[dict[str, str]]:
    path = DATA / name
    if not path.exists():
        raise SystemExit(f"missing data table: {path}")
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


TIME_PATTERN = re.compile(r"\(Day=(\d+),Hour=(\d+),Minute=(\d+)\)")

# Kept in step with MaskGame::FCycleClock: the cycle opens at 06:00 on day one
# and the day counter turns at 06:00, so hours before dawn belong to the tail of
# their own day.
MINUTES_PER_DAY = 24 * 60
MINUTES_PER_CYCLE = 3 * MINUTES_PER_DAY
CYCLE_START_HOUR = 6


def to_elapsed(value: str, context: str) -> int | None:
    match = TIME_PATTERN.fullmatch(value.strip())
    if not check(match is not None, f"{context}: '{value}' is not a (Day=,Hour=,Minute=) triple"):
        return None
    day, hour, minute = (int(g) for g in match.groups())
    check(1 <= day <= 3, f"{context}: day {day} is outside the three-day cycle")
    check(0 <= hour < 24, f"{context}: hour {hour} is out of range")
    check(0 <= minute < 60, f"{context}: minute {minute} is out of range")
    hour_in_day = (hour - CYCLE_START_HOUR) % 24
    return (day - 1) * MINUTES_PER_DAY + hour_in_day * 60 + minute


def main() -> int:
    source = TYPES_HEADER.read_text()
    masks = set(parse_enum(source, "EMaskType")) - {"Count"}
    songs = set(parse_enum(source, "ESongType"))
    echoes = set(parse_enum(source, "EEchoType")) - {"Count"}
    forms = set(parse_enum(source, "EMaskForm")) - {"Count"}

    regions = read_table("DT_Regions.csv")
    dungeons = read_table("DT_Dungeons.csv")
    mask_rows = read_table("DT_Masks.csv")
    song_rows = read_table("DT_Songs.csv")
    quests = read_table("DT_Quests.csv")
    schedules = read_table("DT_Schedules.csv")

    region_ids = {row["RegionId"] for row in regions}

    # ---- Regions ----
    seen_regions: set[str] = set()
    chapters: set[int] = set()
    for row in regions:
        rid = row["RegionId"]
        check(rid not in seen_regions, f"duplicate region id '{rid}'")
        seen_regions.add(rid)
        check(row["RequiredForm"] in forms, f"region '{rid}': unknown form '{row['RequiredForm']}'")
        chapter = int(row["Chapter"])
        check(1 <= chapter <= 13, f"region '{rid}': chapter {chapter} out of range")
        chapters.add(chapter)
        check(bool(row["DisplayName"].strip()), f"region '{rid}' has no display name")
        check(bool(row["Summary"].strip()), f"region '{rid}' has no summary")

    check(chapters == set(range(1, 14)),
          f"the region table should cover all thirteen chapters; missing {sorted(set(range(1, 14)) - chapters)}")

    # ---- Dungeons ----
    seen_echoes: set[str] = set()
    for row in dungeons:
        did = row["DungeonId"]
        check(row["RegionId"] in region_ids, f"dungeon '{did}': unknown region '{row['RegionId']}'")
        check(row["PrimaryForm"] in forms, f"dungeon '{did}': unknown form '{row['PrimaryForm']}'")
        check(row["Echo"] in echoes, f"dungeon '{did}': unknown echo '{row['Echo']}'")
        check(row["Echo"] not in seen_echoes, f"echo '{row['Echo']}' is claimed by more than one dungeon")
        seen_echoes.add(row["Echo"])
        check(int(row["RoomCount"]) > 0, f"dungeon '{did}' has no rooms")
        check(int(row["SmallKeys"]) <= int(row["RoomCount"]),
              f"dungeon '{did}' hides more keys than it has rooms to hide them in")

    check(seen_echoes == echoes - {"None"},
          f"every echo needs exactly one dungeon; missing {sorted(echoes - {'None'} - seen_echoes)}")

    # ---- Masks ----
    listed_masks = [row["Mask"] for row in mask_rows]
    for row in mask_rows:
        mask = row["Mask"]
        check(mask in masks, f"mask table lists unknown mask '{mask}'")
        check(row["RegionId"] in region_ids, f"mask '{mask}': unknown region '{row['RegionId']}'")
        check(row["GrantFlag"].startswith("perm."),
              f"mask '{mask}': grant flag '{row['GrantFlag']}' must be permanent or a rewind takes the mask back")
        check(bool(row["HowToEarn"].strip()), f"mask '{mask}' does not say how it is earned")
        check(bool(row["Description"].strip()), f"mask '{mask}' has no description")

    check(len(listed_masks) == len(set(listed_masks)), "a mask appears twice in the mask table")
    missing_masks = masks - {"None"} - set(listed_masks)
    check(not missing_masks, f"masks with no table row: {sorted(missing_masks)}")

    # ---- Songs ----
    listed_songs = [row["Song"] for row in song_rows]
    for row in song_rows:
        check(row["Song"] in songs, f"song table lists unknown song '{row['Song']}'")
        check(row["TaughtInRegion"] in region_ids,
              f"song '{row['Song']}': unknown region '{row['TaughtInRegion']}'")
        check(bool(row["Effect"].strip()), f"song '{row['Song']}' has no effect text")

    check(len(listed_songs) == len(set(listed_songs)), "a song appears twice in the song table")
    missing_songs = songs - {"None"} - set(listed_songs)
    check(not missing_songs, f"songs with no table row: {sorted(missing_songs)}")

    # ---- Quests ----
    quest_ids: set[str] = set()
    completion_flags: set[str] = set()
    rewarded_masks: set[str] = set()
    for row in quests:
        qid = row["QuestId"]
        check(qid not in quest_ids, f"duplicate quest id '{qid}'")
        quest_ids.add(qid)
        check(row["RegionId"] in region_ids, f"quest '{qid}': unknown region '{row['RegionId']}'")
        check(row["RewardMask"] in masks, f"quest '{qid}': unknown reward mask '{row['RewardMask']}'")
        check(row["bRewardsHeartFragment"].lower() in ("true", "false"),
              f"quest '{qid}': bRewardsHeartFragment must be true or false")

        flag = row["CompletionFlag"]
        check(bool(flag), f"quest '{qid}' has no completion flag and could be finished every cycle")
        check(flag not in completion_flags, f"completion flag '{flag}' is used by more than one quest")
        completion_flags.add(flag)
        check(flag.startswith("perm."),
              f"quest '{qid}': completion flag '{flag}' must be permanent or the rewind undoes the quest")

        if row["RewardMask"] != "None":
            check(row["RewardMask"] not in rewarded_masks,
                  f"mask '{row['RewardMask']}' is awarded by more than one quest")
            rewarded_masks.add(row["RewardMask"])

        start = to_elapsed(row["WindowStart"], f"quest '{qid}' WindowStart")
        end = to_elapsed(row["WindowEnd"], f"quest '{qid}' WindowEnd")
        if start is not None and end is not None:
            check(start < end, f"quest '{qid}': window starts at {start} and ends at {end}")
            check(end <= MINUTES_PER_CYCLE, f"quest '{qid}': window ends after the moon lands")

    # ---- Schedules ----
    row_names: set[str] = set()
    actors_with_entries: set[str] = set()
    gated_flags: set[str] = set()
    for row in schedules:
        name = row["Name"]
        check(name not in row_names, f"duplicate schedule row name '{name}'")
        row_names.add(name)
        check(row["RegionId"] in region_ids,
              f"schedule '{name}': unknown region '{row['RegionId']}'")
        check(bool(row["Activity"].strip()), f"schedule '{name}' has no activity text")
        actors_with_entries.add(row["ActorId"])

        start = to_elapsed(row["Start"], f"schedule '{name}' Start")
        end = to_elapsed(row["End"], f"schedule '{name}' End")
        if start is not None and end is not None:
            # FScheduleTable treats an end at or before the start as "runs to
            # impact". That fallback is there for robustness, not for authoring:
            # relying on it in data is almost always a day number written one out,
            # because the day counter turns at 06:00 rather than at midnight.
            check(start < end,
                  f"schedule '{name}': window runs {start} to {end}. An overnight window "
                  f"ends at 06:00 of the NEXT day number; a window ending at midnight "
                  f"ends at 00:00 of its OWN day number.")

        if row["RequiredFlag"]:
            gated_flags.add(row["RequiredFlag"])

    # Every quest names the NPC whose page it lives on, so that NPC needs a
    # timetable or the player has nowhere to go and find them.
    for row in quests:
        check(row["ActorId"] in actors_with_entries,
              f"quest '{row['QuestId']}' names actor '{row['ActorId']}', who has no schedule entries")

    # A schedule branch gated on a flag no quest ever sets is dead content.
    for flag in sorted(gated_flags):
        check(flag in completion_flags,
              f"schedule branch waits on flag '{flag}', which no quest ever sets")

    print(f"DataTables: {checks} checks, {len(failures)} failures")
    for failure in failures:
        print(f"    FAIL {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
