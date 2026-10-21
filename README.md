# Random Level (Geode mod)

Targets Geode SDK 5.11.2 / Geometry Dash 2.2081. Adds a button (orange "Harder" face) to the
bottom row of the main menu.

## Random Level

- **Min / Max** – difficulty range, Auto through Extreme Demon.
- **Length** – Tiny, Short, Medium, Long, XL, Plat. All ticked (or none) means any length.
- **Rated only** / **Skip completed**.
- **Roll!** – opens the page of a random online level matching the filters.

Each roll picks one tier from the range with equal odds (GD's search can't mix demon and
non-demon tiers), reads the result count, then jumps to a random page and a random level on it.

## Sightreading Practice

Opened with the **Sightreading** button on the Random Level popup.

- **Difficulty 1–10** – controls spacing, spike counts, gap sizes, how far openings move, and
  whether orb jumps appear.
- **Length** – Short (3 sections), Medium (5), Long (8), Very Long (12).
- **Gamemodes** – Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing.
- **Speed** – 0.5x, 1x, 2x, 3x, 4x.

Each section picks a random allowed gamemode and speed (no back-to-back repeats when you allow
more than one mode). Spacing scales with speed so reaction time stays similar. Every section ends
by steering you into the next portal (gates step down to the floor, ball/spider sections finish
with a ceiling strip), so portals can't be skipped.

Levels are created in memory only, and quitting goes straight back to the main menu.

All tuning numbers are at the top of `src/SightreadGenerator.cpp`.

**Not yet play-tested in game.** The generator follows GD's level-string format and standard
physics distances, but expect to adjust spacing and gap sizes after a few runs.

## Getting a .geode file

### Option A: GitHub Actions (no local setup)

1. Create a free GitHub account and a new repository.
2. Upload the contents of this folder (Add file → Upload files). Include the hidden
   `.github` folder; if your browser hides it, create `.github/workflows/multi-platform.yml`
   by hand and paste its contents.
3. Open the **Actions** tab. The "Build Geode Mod" workflow runs automatically (about 5–10 minutes).
4. Open the finished run and download **Build Output**. Unzip it to get
   `jing.random-level.geode`, which contains Windows, Mac, iOS and Android builds.

### Option B: build locally

Install the Geode CLI and SDK (https://docs.geode-sdk.org/getting-started/), then in this folder:

```
geode build
```

### Installing

Put `jing.random-level.geode` in Geometry Dash's `geode/mods` folder (or open it with
Geode's "Install from file" option) and restart the game.

## Files

- `src/main.cpp` – main-menu button, plus quit hooks for generated levels.
- `src/RandomLevelPopup.*` – online search (difficulty, length, rated, completed filters).
- `src/SightreadingPopup.*` – Sightreading Practice settings and launch.
- `src/SightreadGenerator.*` – the layout generator (plain C++, no Geode dependencies).
- `src/UiHelpers.hpp` – shared checkbox / stepper widgets.
