---
area: recursos-y-multimedia
source_files:
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/client/CODIGO/clsSurfaceManager.cls
  - legacy/client/CODIGO/clsAudio.cls
tags: [recursos, multimedia, graficos, audio, wav, mp3, ind, bmp]
last_updated: 2026-09-06
---

## Resumen
Los recursos gráficos del cliente legacy consisten en imágenes Bitmaps (`.bmp`) indexadas por archivos `.ind`, mientras que el audio utiliza efectos WAV invocados por opcodes de red y música ambiental en formato MIDI y MP3.

## Hallazgos

- **Motor de Renderizado y Gráficos (`.bmp`)**:
  - Ubicación: `legacy/client/Graficos/<GrhID>.bmp` o empaquetados mediante índices de gráficos `Graficos.ind`.
  - Transparencia: Utilizan mascara de color clave RGB magenta (`RGB(255, 0, 255)`) para transparencias de sprites y tilesets.
  > Fuente: `legacy/client/CODIGO/TileEngine.bas`, función `InitGrh`

- **Índices de Animación (`.ind`)**:
  - `Graficos.ind`, `Body.ind`, `Head.ind`, `Weapon.ind`, `Shield.ind`, `Helmet.ind`.
  - Estructura: Definen la animación por dirección (Norte, Este, Sur, Oeste), cantidad de cuadros por animación, velocidad de cuadro y coordenadas de corte `(X, Y, Width, Height)` sobre la imagen BMP.
  > Fuente: `legacy/client/CODIGO/TileEngine.bas`, procedimiento `CargarGrhData`

- **Subsistema de Audio (Efectos y Música)**:
  - `clsAudio.cls`: Wrapper de DirectSound / DirectMusic para la reproducción de WAVs y pistas musicales.
  - Efectos WAV (`legacy/client/WAV/<WaveID>.wav`): Se reproducen localmente cuando el servidor envía el paquete `ServerPacketID.PlayWave` (e.g. golpes, hechizos, pasos).
  > Fuente: `legacy/client/CODIGO/clsAudio.cls`, procedimiento `PlayWave`
  - Música MIDI/MP3 (`legacy/client/MIDI/` y `legacy/client/MP3/`): Se reproducen según el mapa en el que se encuentra el jugador o el comando `PlayMP3`.
  > Fuente: `legacy/client/CODIGO/clsAudio.cls`, procedimiento `PlayMP3`

## Lógica y Datos Extraídos

### Matriz de Assets Multimedia

| Tipo de Recurso | Formato de Archivo | Directorio en Cliente | Método de Indexación / Carga | Disparador de Reproducción |
| :--- | :--- | :--- | :--- | :--- |
| **Sprites / Tiles** | `.bmp` | `legacy/client/Graficos/` | `Graficos.ind` (`CargarGrhData`) | Renderizado continuo del loop de mapa |
| **Equipamiento** | `.ind` | `legacy/client/INIT/` | `Body.ind`, `Head.ind`, `Weapon.ind` | Cambio de equipo recibido por red |
| **Efectos de Sonido** | `.wav` | `legacy/client/WAV/` | ID Numérico binario | Paquete `ServerPacketID.PlayWave` |
| **Música Ambiental**| `.mp3` / `.mid`| `legacy/client/MP3/`, `MIDI/` | Archivo por mapa (`Map.dat`) | Cambio de zona / paquete `PlayMP3` |

## Preguntas Abiertas
- La migración a C++ con SDL2/SFML permitirá reemplazar la mascara magenta `RGB(255, 0, 255)` con canales alpha nativos en PNG o procesadores de texturas de GPU.
