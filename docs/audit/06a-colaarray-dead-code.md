---
area: estructura-del-proyecto
source_files:
  - legacy/server/Codigo/cColaArray.cls
  - legacy/server/Codigo/frmMain.frm
  - legacy/server/Codigo/Declares.bas
  - legacy/server/SERVER.VBP
  - legacy/server/Changelog-server.txt
tags: [auditoria, capa-0, ccolaarray, codigo-muerto, exclusion, vb6, cpp]
last_updated: 2026-09-08
---

# Auditoría de Módulo Muerto: cColaArray.cls (Exclusión del Port C++)

Este documento constituye el registro permanente de auditoría técnica sobre la clase `legacy/server/Codigo/cColaArray.cls` (`CColaArray`), fundamentando su **exclusión explícita** del proceso de porting a C++.

---

## Resumen

- **`cColaArray.cls` (`CColaArray`)**: Es una clase de VB6 que implementa un búfer circular (ring buffer) de cadenas de texto de capacidad configurable (por defecto 300 elementos).
- **Diagnóstico de Auditoría**: La clase es **100% código muerto, inalcanzable e incompilable**.
- **Decisión de Porting**: **EXCLUIDO.** No se generará ningún equivalente C++ (`src/server/cColaArray.hpp` ni `src/server/cColaArray.cpp`). Su exclusión queda registrada formalmente para evitar que futuros colaboradores o herramientas intenten portarla o "corregirla".

---

## Hallazgos e Investigación de Auditoría

### 1. Rastreo de Ocurrencias Literales en el Repositorio

Se realizó una búsqueda exhaustiva en la totalidad del repositorio (incluyendo fuentes `.bas`, `.cls`, `.frm`, `.frx`, proyectos `.vbp`, archivos de trabajo `.vbw` y documentación).

* **Registros de proyecto**:
  * [legacy/server/SERVER.VBP:L31](legacy/server/SERVER.VBP#L31): `Class=CColaArray; Codigo\cColaArray.cls`
  * [legacy/server/SERVER.vbw:L29](legacy/server/SERVER.vbw#L29): `CColaArray = 0, 0, 0, 0, C`
* **Definición de clase**:
  * [legacy/server/Codigo/cColaArray.cls:L9](legacy/server/Codigo/cColaArray.cls#L9): `Attribute VB_Name = "CColaArray"`
* **Único punto de referencia en el código fuente**:
  * [legacy/server/Codigo/frmMain.frm:L1034](legacy/server/Codigo/frmMain.frm#L1034): `Set UserList(NewIndex).CommandsBuffer = New CColaArray`

---

### 2. Análisis del Bloque de Compilación Condicional

Al analizar la única referencia en [frmMain.frm:L995-L1035](legacy/server/Codigo/frmMain.frm#L995-L1035), se constató que la rutina `TCPServ_NuevaConn` está totalmente aislada dentro de una directiva de compilación condicional:

```vb
#If UsarQueSocket = 3 Then
Private Sub TCPServ_NuevaConn(ByVal ID As Long)
    ...
    Set UserList(NewIndex).CommandsBuffer = New CColaArray
    ...
End Sub
#End If
```

* En el archivo principal de proyecto [SERVER.VBP:L84](legacy/server/SERVER.VBP#L84), las constantes de compilación activas son:
  ```ini
  CondComp="UsarQueSocket = 1 : ConUpTime = 1"
  ```
* Dado que `UsarQueSocket` equivale a `1` en el entorno oficial del servidor, el compilador de VB6 **ignora y omite por completo** todo el bloque `#If UsarQueSocket = 3 Then`.

---

### 3. Incompatibilidad de Estructura e Incompilabilidad

En el caso hipotético de que un desarrollador modificase `SERVER.VBP` para compilar con `UsarQueSocket = 3`:

1. El compilador de VB6 se detendría con un error fatal de compilación en la línea 1034 de `frmMain.frm`: *"Method or data member not found"*.
2. El miembro `.CommandsBuffer` **no existe** dentro del `Type User` definido en [Declares.bas:L1186-L1265](legacy/server/Codigo/Declares.bas#L1186-L1265).
3. No existe ningún otro `Type` o `Class` en todo el codebase que contenga una propiedad o campo llamado `CommandsBuffer`.

---

### 4. Verificación de Invocaciones Dinámicas / Binding Tardío

Se descartó cualquier posible invocación indirecta:
* **`CallByName`**: No existe ninguna llamada a `CallByName` en todo el codebase del servidor.
* **Acceso por `Object` / `Variant`**: No existen variables genéricas que invoquen `.MaxElems`, `.Push` o `.Pop` sobre instancias de `CColaArray`.
* **Uso de `.MaxElems`**: La propiedad `MaxElems` aparece **exclusivamente** dentro de la definición de `cColaArray.cls` ([cColaArray.cls:L109-L113](legacy/server/Codigo/cColaArray.cls#L109-L113)).

---

### 5. Contexto Histórico del Código Legacy

La presencia de esta clase se explica analizando el historial de cambios registrado en [Changelog-server.txt](legacy/server/Changelog-server.txt):

* **27/04/2006**: *Se incluyó la clase clsByteQueue utilizada en el nuevo protocolo. (Maraxus).*
* **10/01/2007**: *Todo rastro de ColaSalida ha sido borrado y remplazado por el outgoingData Buffer en los casos necesarios (Tavo).*

En las versiones antiguas de Argentum Online (pre-0.12.x), los comandos salientes/entrantes de cada socket cliente se encolaban mediante `CColaArray` asignados al campo `CommandsBuffer` del usuario. Cuando los desarrolladores refactorizaron el protocolo de red hacia `clsByteQueue` (`incomingData` y `outgoingData`), eliminaron el campo `CommandsBuffer` del `Type User`, pero dejaron huérfanos el archivo `cColaArray.cls` y el bloque `#If UsarQueSocket = 3` no compilado en `frmMain.frm`.

---

## Conclusión y Regla de Porting

1. **`cColaArray.cls` NO se portará a C++**.
2. No debe crearse ningún archivo `src/server/cColaArray.hpp` ni `src/server/cColaArray.cpp`.
3. Esta exclusión **NO afecta** a los otros dos módulos de cola de Capa 0:
   - [ModCola.cls](legacy/server/Codigo/ModCola.cls) (`cCola`): Se utiliza activamente para el sistema de soporte y peticiones `/AYUDA` (`Ayuda As New cCola`).
   - [Queue.bas](legacy/server/Codigo/Queue.bas) (`Queue`): Se utiliza activamente para el algoritmo de Pathfinding BFS de NPCs (`tVertice`).
