# Migration Conventions — Structural Port Policy

This project is a structural (transliteration) port of a legacy VB6 codebase 
to C++, not a re-architecture. The goal is maximum familiarity for developers 
who already know the original VB6 codebase, not idiomatic "modern" C++ design.

## Naming Policy
For every file, class, function, and variable, apply this fallback order:
1. Preserve the exact legacy VB6 identifier, if one exists and is a valid 
   C++ identifier.
2. If no legacy identifier exists (something new, required only by C++ or 
   by a library we introduced), name it in Rioplatense Spanish.
3. If the Rioplatense term would be regionally unclear, fall back to a more 
   broadly understood Latin American Spanish term.

Exception: if a legacy identifier collides with a reserved C++ keyword 
(e.g. New, Class, Public), skip directly to step 2/3 of this same chain — 
do not fall back to English.

## Structure Policy
Mirror the legacy .bas/.cls/.frm module grouping as closely as possible — 
one legacy module maps to one C++ file or file pair with the same name. 
Where this isn't possible (e.g. a .frm file mixes UI and logic in a way 
with no direct C++/SFML equivalent), group by the same conceptual boundary 
instead, using standard .h/.cpp conventions.

## What is NOT covered by this policy
Library and tooling choices (standalone Asio, SFML, SQLite, TGUI, nlohmann/json, 
doctest, CMake/vcpkg) are unavoidable technology substitutions for VB6-specific 
APIs (Winsock, DirectX, flat files) and are not subject to the "preserve 
legacy structure" rule — only the game logic's own organization is.

## Testing Policy
All unit tests across this entire project use **doctest** as the single testing 
framework. It was chosen specifically for its minimal compile-time overhead 
given the large number of modules being ported.

## Known necessary exception
Networking/concurrency: the legacy server already supports multiple 
simultaneous player connections. Faithfully porting that existing capability 
(not redesigning it) remains a requirement of this port, even though the 
underlying networking library (standalone Asio) is necessarily different from the 
original Winsock implementation.
