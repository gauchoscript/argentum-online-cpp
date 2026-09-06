---
area: assets
source_files:
  - legacy/client/CODIGO/GameIni.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/client/CODIGO/clsAudio.cls
tags: [assets, graphics, audio, directdraw, directsound, directmusic]
last_updated: 2026-09-05
---

## Summary
Graphic assets are stored as Windows Bitmaps (`.bmp`) indexed by binary `.ind` files. Audio uses WAV files for sound effects via DirectSound 7, MIDI files for music via DirectMusic 7, and MP3 files via DirectShow (`quartz.dll`).

## Findings

- **Graphic Sprites & Animations**:
  - `LoadGrhData` reads binary `graficos.ind`. Single-frame graphics (`NumFrames = 1`) define source crop rectangles (`sX, sY, pixelWidth, pixelHeight`) in `<FileNum>.bmp`. Multi-frame graphics (`NumFrames > 1`) define an array of frame IDs and a playback speed.
  > Source: `legacy/client/CODIGO/TileEngine.bas`, function `LoadGrhData`
  - Directional character equipment/body animations (4 directions: N, E, S, W) are loaded from `.ind` files in `legacy/client/INIT/`: `Cabezas.ind`, `Cascos.ind`, `Personajes.ind`, `Escudos.ind`, `Armas.ind`.
  > Source: `legacy/client/CODIGO/TileEngine.bas`, functions `CargarCabezas`, `CargarCascos`, `CargarCuerpos`

- **Sound Effects (WAV)**:
  - Stored in `legacy/client/WAV/<SoundID>.wav`.
  - Loaded & played via `PlayWave` using DirectSound 7 (`DirectSoundBuffer7`).
  > Source: `legacy/client/CODIGO/clsAudio.cls`, function `PlayWave`
  - 3D spatial panning updated relative to player coordinates by `MoveListener`.
  > Source: `legacy/client/CODIGO/clsAudio.cls`, function `MoveListener`

- **Music Tracks (MIDI & MP3)**:
  - MIDI files in `legacy/client/MIDI/` played via DirectMusic 7 (`DirectMusicPerformanceCreate`, `DirectMusicLoaderCreate`).
  > Source: `legacy/client/CODIGO/clsAudio.cls`, function `PlayMIDI`
  - MP3 files in `legacy/client/MP3/` played via COM DirectShow (`FilgraphManager` in `quartz.dll`).
  > Source: `legacy/client/CODIGO/clsAudio.cls`, function `MusicMP3Play`

## Extracted Logic / Data

### Media Subsystem Architecture

```
Graphics (.bmp + graficos.ind) --> DirectDraw 7 SurfaceDB (TileEngine.bas)
Sound Effects (.wav) -----------> DirectSound 7 (clsAudio.cls -> PlayWave)
MIDI Music (.mid) --------------> DirectMusic 7 (clsAudio.cls -> PlayMIDI)
MP3 Music (.mp3) ---------------> DirectShow / quartz.dll (clsAudio.cls -> MusicMP3Play)
```

## Open Questions
- Legacy DirectX 7 APIs (DirectDraw 7, DirectSound 7, DirectMusic 7) are obsolete in modern C++; need migration to SDL2/SFML/FMOD or DirectX 11/12 with OpenAL/SDL_mixer.
