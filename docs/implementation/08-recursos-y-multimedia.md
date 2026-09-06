---
area: recursos-y-multimedia
status: not-started
audit_reference: docs/audit/08-recursos-y-multimedia.md
tags: [recursos, multimedia, graficos, audio, wav, mp3, ind, bmp]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Motor Gráfico y Multimedia C++**: Adoptar SDL2 / SDL_mixer (o SFML) para el renderizado acelerado por hardware y la gestión de recursos de audio WAV y MP3 en el nuevo cliente C++.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Garantizar la correcta conversión/lectura de los archivos de índice `.ind` binarios sin alterar las animaciones de 4 direcciones.

## Tareas
- [ ] Implementar la clase `ResourceManager` en C++ para cargar índices `.ind` y spritesheets.
- [ ] Implementar la clase `AudioManager` utilizando `SDL_mixer` para efectos de sonido WAV y música MP3.

## Archivos de Código Relacionados
- `legacy/client/CODIGO/TileEngine.bas`
- `legacy/client/CODIGO/clsAudio.cls`
