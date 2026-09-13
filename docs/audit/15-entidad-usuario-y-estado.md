---
area: entidad-usuario-y-estado
source_files:
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/TCP.bas
  - legacy/server/Codigo/Clases.dat
tags: [usuario, userlist, estados, clases, razas, atributos, experiencia, niveles]
last_updated: 2026-09-13
---

# Macro-Área 15: Entidad Usuario y Estado

Este documento define la visión arquitectónica transversal y el alcance de la entidad central del jugador (`UserList`), el ciclo de vida de la sesión, la máquina de estados del personaje, el sistema de razas, clases, progresión de niveles y persistencia de atributos en Argentum Online v0.13.0.

---

## 1. Resumen

La entidad de usuario constituye el modelo de datos principal alrededor del cual giran todos los subsistemas del juego:
- **Estructura Central `UserList` (`Declares.bas`)**: Vector de tamaño fijo (`MaxUsers`) que almacena la información completa en memoria de cada cliente conectado: estadísticas vitales, coordenadas espaciales, flags de estado, inventario, libro de hechizos, reputación faccionaria y descriptores de socket de red.
- **Ciclo de Vida de la Conexión (`Modulo_UsUaRiOs.bas` / `TCP.bas`)**: Ingreso del jugador, validación de credenciales, carga del archivo del personaje (`.chr`), asociación de slot, mantenimiento de la sesión, temporizadores de salida segura (comando `/SALIR` de 10 segundos) y deslogueo/persistencia final en disco.
- **Máquina de Estados del Personaje**: Estados vitales (vivo vs. muerto en forma de espíritu/fantasma), navegación (a pie, navegando en barca, montando a caballo), meditación para regeneración acelerada de maná, sigilo y combate.
- **Progresión, Clases y Razas**: Escala de 50 niveles de experiencia, asignación de puntos de vida, maná y energía por nivel según la clase (guerrero, mago, paladín, asesino, clérigo, etc.) y modificadores raciales de fuerza, agilidad, inteligencia, constitución y carisma.

---

## 2. Alcance Arquitectónico

### A. Estructura `User` y Gestión de Ranuras
- **Indexación y Acceso Global**: Acceso por `UserIndex` (1 a `MaxUsers`). Mantenimiento de listas de usuarios activos para optimizar bucles de actualización.
- **Persistencia en Formato `.chr`**: Serialización de propiedades de cuenta, inventario, hechizos aprendidos, reputación faccionaria y posición espacial al desconectar o ante guardados periódicos (*WorldSave*).

### B. Ciclo de Vida y Máquina de Estados (`Modulo_UsUaRiOs.bas`)
- **Manejo de la Muerte**:
  - Transición a cuerpo de fantasma (`Body = iCuerpoMuerto`), supresión de colisiones de bloqueo físico y pérdida de objetos no protegidos (drop al suelo).
  - Resurrección por sacerdotes en templos o hechizos de resurrección de jugadores aliados.
- **Navegación y Transporte**:
  - Cambio de sprite corporal al abordar embarcaciones o cabalgaduras.
  - Validación de celdas de agua navegables según posesión de barca en el inventario.
- **Desconexión Segura e Inmediata**:
  - Salida voluntaria con cuenta regresiva de 10 segundos para prevenir deslogueos tácticos en combate (*combat logging*).
  - Cierre forzoso de conexión por desconexión de socket de red o timeout de inactividad.

### C. Sistema de Atributos, Razas y Clases
- **Atributos Principales**: Fuerza, Agilidad, Inteligencia, Carisma y Constitución, inicializados durante la creación del personaje mediante tirada de dados.
- **Modificadores de Raza**: Ajustes inherentes a Humanos, Elfos, Elfos Oscuros, Enanos y Gnomos.
- **Habilidades y Skills**: Asignación de puntos en habilidades profesionales y de combate (tácticas, combate con armas, magia, apuñalar, tala, minería, etc.).

---

## 3. Módulos y Subsistemas Involucrados

| Módulo Legacy | Responsabilidad Principal | Informe de Detalle |
| :--- | :--- | :--- |
| `Modulo_UsUaRiOs.bas` | Ciclo de vida del usuario, conexión, desconexión, muerte, resurrección y progresión. | Futura auditoría `15a-modulo-usuarios-detalle.md` |
| `Declares.bas` | Declaración de la estructura `User`, `UserFlags`, `UserStats` y constantes asociadas. | [`01-estructura-del-proyecto.md`](01-estructura-del-proyecto.md) |
| `TCP.bas` | Manejo de sockets de red y mapeo entre socket y `UserIndex`. | [`02c-tcp-detalle.md`](02c-tcp-detalle.md) |
| `Clases.dat` | Tabla de configuración de clases, vida por nivel y modificadores de daño. | [`06-formatos-de-datos.md`](06-formatos-de-datos.md) |
