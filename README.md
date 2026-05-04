# Emotion Star Journal

A Qt desktop journal that turns each emotional memory into a star.

## Current v1 Features

- Create a journal entry with title, date, emotion, intensity, energy, and note
- Visualize entries as stars on an emotional sky map
- Click the archive list or a star to inspect a saved memory
- Edit an existing memory and update the corresponding star
- Delete a saved memory from the sky
- Start with demo entries so the scene looks alive immediately
- Save entries locally as JSON and load them automatically next time

## Tech Stack

- C++
- Qt 6 Widgets
- CMake
- MinGW

## Build

Use the helper script in this repository:

```bat
build.bat
```

Or configure manually with Qt 6.8.3 and MinGW 13.1.

## Two-Day Roadmap

### Day 1

- Finish the core star journal experience
- Complete local persistence
- Make the UI stable enough for demo and Git upload

### Day 2

- Add richer visual polish
- Add filtering or statistics
- Prepare report screenshots and Bilibili demo script

## Repo Advice

Recommended commit rhythm:

1. `init: set up Qt project skeleton`
2. `feat: build emotion entry and star map journal`
3. `style: polish UI and prepare submission materials`
