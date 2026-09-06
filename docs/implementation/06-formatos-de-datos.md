---
area: formatos-de-datos
status: not-started
audit_reference: docs/audit/06-formatos-de-datos.md
tags: [datos, formatos, ini, chr, map, dat, binario]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Parsers Binarios y de INI en C++**: Crear parsers de alto rendimiento en C++ (`IniReader`/`IniWriter` y `MapBinaryLoader`) compatibles campo por campo con los formatos legacy de VB6.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Asegurar que el padding y alineación de bytes (*struct alignment*) en la lectura de mapas `.map` sea idéntico a las declaraciones de VB6.

## Tareas
- [ ] Implementar `IniReader` para parsear `.chr`, `.dat` y `Server.ini`.
- [ ] Implementar `MapLoader` para leer el stream binario de 100x100 tiles de los archivos `.map`.

## Archivos de Código Relacionados
- `legacy/server/Codigo/FileIO.bas`
- `legacy/server/Codigo/ModMapIO.bas`
- `legacy/client/CODIGO/TileEngine.bas`
