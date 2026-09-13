# Reglas del Proyecto — Argentum Online C++

- **Idioma y Tono**: Todas las respuestas al usuario, explicaciones, documentación, comentarios y mensajes de commit DEBEN estar redactados obligatoriamente en **español rioplatense** (usando voseo: *vos*, *acordate*, *tenés*, *podés*, etc.).
- **Mensajes de Commit**:
  - Usar obligatoriamente el formato **Conventional Commits** (`type(scope): descripción`) redactados en **español rioplatense** usando el verbo en **imperativo sin tilde** (ejemplos: `agrega`, `implementa`, `porta`, `corrige`, `actualiza`, `formaliza`).
  - **Convención de Scopes**:
    - Para código fuente y lógica de la aplicación, usar el componente macro como scope (`server`, `client`, `net`, `build`, etc.): `feat(server): ...`, `refactor(server): ...`, `fix(client): ...`.
    - Para documentación o tests especializados, se prefiere un scope específico del subsistema o área temática: `docs(conventions): ...`, `docs(securityip): ...`, `test(fixtures): ...`.
  - **Mención Obligatoria del Módulo**: En todo commit que porte, agregue, modifique o documente un módulo, **el nombre del módulo legacy / C++ correspondiente DEBE figurar explícitamente en el mensaje** (ejemplos históricos: `feat(server): agrega clsClan y modGuilds...`, `feat(server): implementa modulo FileIO...`, `feat(server): porta clsByteQueue a C++...`, `feat(server): porta SecurityIp con control anti-flood...`).
- **Prohibición de Rutas Absolutas y Esquema `file://`**:
  - Está **estrictamente prohibido** utilizar el esquema `file://` o incluir rutas absolutas del sistema de archivos del usuario (por ejemplo `c:\Users\...`, `c:/Users/...`).
  - Todas las referencias a archivos, directorios y código en las respuestas al usuario, planes, artefactos y documentación DEBEN realizarse obligatoriamente mediante **rutas relativas limpias** respecto a la raíz del repositorio (ejemplo: `src/server/Protocol.hpp`, `docs/implementation/16-protocol-breakdown.md`) formateadas como texto plano o código en línea (`` `ruta/al/archivo` ``), sin enlaces con esquemas locales.
