---
area: pantallas-e-interfaz
source_files:
  - legacy/client/CODIGO/frmConnect.frm
  - legacy/client/CODIGO/frmCrearPersonaje.frm
  - legacy/client/CODIGO/frmMain.frm
  - legacy/client/CODIGO/FrmEstadisticas.frm
  - legacy/client/CODIGO/frmComerciar.frm
  - legacy/client/CODIGO/frmBanco.frm
  - legacy/client/CODIGO/frmGuildLeader.frm
tags: [ui, interfaz, formularios, frm, cliente, pantallas]
last_updated: 2026-09-06
---

## Resumen
El cliente original de Argentum Online posee más de 45 formularios (`.frm`) de Visual Basic 6 que definen la interfaz gráfica de usuario, pantallas de autenticación, renderizado de mapas y ventanas interactivas.

## Hallazgos

- **Pantallas Principal y de Login**:
  - `frmConnect.frm`: Pantalla de inicio de sesión donde el usuario ingresa nombre, contraseña, selecciona servidor y accede al registro de nuevos personajes.
  > Fuente: `legacy/client/CODIGO/frmConnect.frm`
  - `frmCrearPersonaje.frm`: Creación de personajes (selección de raza, clase, distribución de atributos tirados con dados, género, cabeza y ciudad de origen).
  > Fuente: `legacy/client/CODIGO/frmCrearPersonaje.frm`
  - `frmMain.frm`: Pantalla principal del juego. Alberga el Viewport de renderizado del mundo (17x13 tiles), la grilla de inventario, el libro de hechizos, barras de estado (HP, Maná, Estamina, Experiencia), barra de chat y mini-mapa.
  > Fuente: `legacy/client/CODIGO/frmMain.frm`

- **Ventanas Interactivas y Menús**:
  - `frmComerciar.frm` / `frmComerciarUsu.frm`: Interfaz de compra/venta con comerciantes NPC y comercio directo entre jugadores.
  > Fuente: `legacy/client/CODIGO/frmComerciar.frm`
  - `frmBanco.frm`: Interfaz del depósito bancario para guardar oro y objetos.
  > Fuente: `legacy/client/CODIGO/frmBanco.frm`
  - `frmEstadisticas.frm`: Panel detallado con los atributos, reputación (facciones), asignación de puntos de skill y estadísticas de combate del personaje.
  > Fuente: `legacy/client/CODIGO/FrmEstadisticas.frm`
  - `frmGuildLeader.frm` / `frmGuildMember.frm`: Gestión de clanes (solicitudes de ingreso, lista de miembros, noticias).
  > Fuente: `legacy/client/CODIGO/frmGuildLeader.frm`

## Lógica y Datos Extraídos

### Inventario de Pantallas Principales del Cliente

| Archivo Formularios | Nombre de Pantalla | Propósito y Acciones Principales del Jugador |
| :--- | :--- | :--- |
| `frmConnect.frm` | Conectar / Login | Ingreso de credenciales, selección de servidor, botón de crear personaje. |
| `frmCrearPersonaje.frm` | Crear Personaje | Selección de raza, clase, tirada de dados de atributos, selección de ciudad. |
| `frmMain.frm` | Juego Principal (HUD) | Renderizado de mapa, movimiento, ataque, uso de ítems, casteo de hechizos, chat. |
| `frmComerciar.frm` | Comercio NPC | Compra y venta de objetos en tiendas de NPCs. |
| `frmBanco.frm` | Banco / Bóveda | Depósito y retiro de objetos u oro en el banco del juego. |
| `FrmEstadisticas.frm` | Estadísticas / Skills | Visualización de stats y distribución de puntos de habilidades. |
| `frmGuildLeader.frm` | Panel de Clan | Administración de clan por parte del líder (aceptar/rechazar miembros, expulsar). |
| `frmOpciones.frm` | Opciones de Juego | Configuración de volumen de audio WAV/MP3, resolución y teclas personalizadas. |

## Preguntas Abiertas
- La interfaz en C++ requerirá reimplementar los formularios de VB6 utilizando una librería UI moderna (e.g. ImGui, SDL_gui o Dear ImGui) manteniendo la distribución exacta de botones y coordenadas visuales de las imágenes `.bmp` subyacentes.
