---
area: protocolo-de-red
status: excluded
source_files:
  - legacy/server/Codigo/clsAntiMassClon.cls
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/frmMain.frm
  - legacy/server/SERVER.VBP
tags: [auditoria, clsantimassclon, codigo-muerto, exclusion, seguridad-alkon, dead-code, vb6, cpp]
last_updated: 2026-09-11
---

# Auditoría Técnica Anexo: `clsAntiMassClon.cls` (Exclusión del Port C++)

Este documento formaliza la auditoría técnica exhaustiva del módulo [`legacy/server/Codigo/clsAntiMassClon.cls`](../../legacy/server/Codigo/clsAntiMassClon.cls) de Argentum Online v0.13.0, estableciendo y fundamentando la **decisión estratégica de exclusión** del port a C++, en estricta consonancia con el precedente fijado para `cColaArray.cls` ([`docs/audit/06a-colaarray-dead-code.md`](06a-colaarray-dead-code.md)).

---

## 1. Resumen Ejecutivo y Decisión de Porting

- **Módulo auditado**: `clsAntiMassClon.cls` (85 líneas de código en Visual Basic 6).
- **Propósito teórico original**: Prevenir la creación masiva y automatizada de múltiples personajes desde una misma dirección IP, fijando un tope de 15 personajes por IP (`MaximoPersonajesPorIP = 15`) dentro de la ventana temporal entre cada autoguardado del mundo (*WorldSave*).
- **Diagnóstico de auditoría**: **100% código muerto, inoperante y estructuralmente incompilable** bajo la configuración de producción de Argentum Online v0.13.0.
  1. En [`legacy/server/SERVER.VBP:76`](../../legacy/server/SERVER.VBP#L76), la directiva de compilación condicional oficial es `CondComp="UsarQueSocket = 1 : ConUpTime = 1"`. La bandera `SeguridadAlkon` **no está definida** (evalúa a `False`).
  2. La única instrucción de toda la clase que da de alta un elemento en la colección de seguimiento (`m_coleccion.Add oIp`) está encapsulada dentro del bloque `#If SeguridadAlkon Then` ([Líneas 58-63](../../legacy/server/Codigo/clsAntiMassClon.cls#L58-L63)). Al no compilarse este bloque, la colección jamás recibe elementos (`Count == 0`).
  3. Como resultado directo, el método `MaxPersonajes` **siempre retornaba `False`**, produciendo un bypass total e incondicional de la restricción en producción.
  4. El tipo/clase referenciado `UserIpAdress` ([Línea 59](../../legacy/server/Codigo/clsAntiMassClon.cls#L59)) **no existe en ninguna parte de los repositorios ni del servidor ni del cliente**. Si un operador hubiera intentado habilitar `SeguridadAlkon = 1`, el proyecto no hubiera compilado en absoluto (`Compile error: User-defined type not defined`).
- **Decisión de porting**: **EXCLUIDO.** No se implementarán `src/server/clsAntiMassClon.hpp` ni `src/server/clsAntiMassClon.cpp`. Se retira la declaración e instanciación de la variable global `aClon` en `Declares.hpp` y `Declares.cpp`, y se registran notas de migración en los módulos consumidores (`Protocol.bas` y timers de `frmMain.frm`) para omitir las llamadas a dicha clase inoperante.
- **Normativa de referencia**: Las decisiones de migración y preservación de convenciones se rigen por [`docs/CONVENTIONS.md`](../CONVENTIONS.md) y [`docs/implementation/00-port-plan.md`](../implementation/00-port-plan.md).

---

## 2. Mapeo de Estructura de `clsAntiMassClon.cls`

A continuación se detalla la estructura completa del archivo original [`legacy/server/Codigo/clsAntiMassClon.cls`](../../legacy/server/Codigo/clsAntiMassClon.cls):

```vb
' [Líneas 1 a 40: Encabezados de licencia AGPL y créditos históricos]
Option Explicit

Private Const MaximoPersonajesPorIP = 15
Private m_coleccion As New Collection

Public Function MaxPersonajes(sIp As String) As Boolean
    Dim i As Long

    For i = 1 To m_coleccion.Count
        If m_coleccion.Item(i).ip = sIp Then
            m_coleccion.Item(i).PersonajesCreados = m_coleccion.Item(i).PersonajesCreados + 1
            MaxPersonajes = (m_coleccion.Item(i).PersonajesCreados > MaximoPersonajesPorIP)
            If MaxPersonajes Then m_coleccion.Item(i).PersonajesCreados = 16
            Exit Function
        End If
    Next i

#If SeguridadAlkon Then
    Dim oIp As New UserIpAdress
    oIp.ip = sIp
    oIp.PersonajesCreados = 1
    m_coleccion.Add oIp
#End If

    MaxPersonajes = False
    Exit Function
End Function

Public Function VaciarColeccion()

On Error GoTo Errhandler

Dim i As Integer

For i = 1 To m_coleccion.Count
   Call m_coleccion.Remove(1)
Next


Exit Function
Errhandler:
    Call LogError("Error en RestarConexion " & Err.description)
End Function
```

### 2.1. Constantes y Variables Miembro
* **`MaximoPersonajesPorIP`** ([Línea 43](../../legacy/server/Codigo/clsAntiMassClon.cls#L43)): Constante privada fijada en `15`. Representa el tope de personajes permitidos por IP.
* **`m_coleccion`** ([Línea 44](../../legacy/server/Codigo/clsAntiMassClon.cls#L44)): Instancia de la clase intrínseca `VBA.Collection`. Lista enlazada que actúa como repositorio de seguimiento en memoria.

### 2.2. Propiedades
* **Ninguna**: La clase no expone propiedades (`Property Get`, `Property Let`, `Property Set`).

### 2.3. Métodos Públicos
* **`MaxPersonajes(sIp As String) As Boolean`** ([Líneas 46 a 67](../../legacy/server/Codigo/clsAntiMassClon.cls#L46-L67)):
  1. Recorre linealmente `m_coleccion` ([Líneas 49-56](../../legacy/server/Codigo/clsAntiMassClon.cls#L49-L56)). Si encuentra un registro cuyo campo `.ip` coincida con el argumento `sIp`:
     - Incrementa `.PersonajesCreados` en 1.
     - Evalúa si `PersonajesCreados > 15`.
     - Si se superó el tope, satura el valor en 16 y retorna `True` (bloqueando la creación).
     - Sale inmediatamente mediante `Exit Function`.
  2. Si la IP no fue encontrada en la colección:
     - Ingresa al bloque condicional `#If SeguridadAlkon Then` ([Líneas 58-63](../../legacy/server/Codigo/clsAntiMassClon.cls#L58-L63)). Si estuviera activo, instanciaría `UserIpAdress`, asignaría `oIp.ip = sIp`, `oIp.PersonajesCreados = 1` y lo agregaría a `m_coleccion`.
  3. Retorna `False` ([Línea 65](../../legacy/server/Codigo/clsAntiMassClon.cls#L65)) y sale de la función.
* **`VaciarColeccion()`** ([Líneas 69 a 83](../../legacy/server/Codigo/clsAntiMassClon.cls#L69-L83)):
  - Itera `m_coleccion.Count` veces removiendo siempre el primer elemento (`Call m_coleccion.Remove(1)`), vaciando la colección en memoria.
  - Posee un controlador de excepciones `On Error GoTo Errhandler` ([Líneas 71 y 81-82](../../legacy/server/Codigo/clsAntiMassClon.cls#L71)) que ante cualquier fallo ejecuta `LogError("Error en RestarConexion " & Err.description)`.

### 2.4. Métodos Privados
* **Ninguno**: No contiene rutinas auxiliares privadas.

---

## 3. Análisis de Dependencias

1. **`Collection` (VBA Runtime)** ([Líneas 44, 49, 62, 75, 76](../../legacy/server/Codigo/clsAntiMassClon.cls#L44)): Colección estándar de punteros `IDispatch`/`IUnknown` de VB6.
2. **`General.LogError`** ([Línea 82](../../legacy/server/Codigo/clsAntiMassClon.cls#L82)): Rutina global de volcado de excepciones a disco ubicada en [`legacy/server/Codigo/General.bas:420`](../../legacy/server/Codigo/General.bas#L420).
3. **`UserIpAdress` (Tipo Fantasma Inexistente)** ([Línea 59](../../legacy/server/Codigo/clsAntiMassClon.cls#L59)): Declaración de tipo invocada únicamente dentro del bloque inactivo `#If SeguridadAlkon Then`. No existe archivo ni definición en toda la solución.

---

## 4. Análisis de Integración con `SecurityIp.bas`

Se investigó con máxima precisión si existía vinculación operativa entre `clsAntiMassClon.cls` y [`legacy/server/Codigo/SecurityIp.bas`](../../legacy/server/Codigo/SecurityIp.bas):

* **Ortogonalidad absoluta**:
  - `clsAntiMassClon` **no interactúa en absoluto** con `SecurityIp.bas`.
  - No depende del sistema de control de intervalos anti-flood (`IpTables` / `IpSecurityAceptarNuevaConexion`).
  - No invoca `IPSecuritySuperaLimiteConexiones` ni `IpRestarConexion`, ni consulta la tabla `MaxConTables`.
* **Disparidad de tipos de representación de IP**:
  - `SecurityIp.bas` manipula IPs exclusivamente en formato binario entero de 32 bits de Winsock (`sa.sin_addr As Long`).
  - `clsAntiMassClon.cls` manipula IPs en cadenas de texto plano ASCII (`sIp As String`, por ejemplo `"192.168.1.100"`).
* **Origen del residuo literal `"Error en RestarConexion"`**:
  - En [`clsAntiMassClon.cls:82`](../../legacy/server/Codigo/clsAntiMassClon.cls#L82) se lee:
    ```vb
    Errhandler:
        Call LogError("Error en RestarConexion " & Err.description)
    ```
  - En [`legacy/server/Codigo/SecurityIp.bas:257`](../../legacy/server/Codigo/SecurityIp.bas#L257) (procedimiento `IpRestarConexion`) se encuentra exactamente el mismo bloque:
    ```vb
    ErrorHandler:
        Call LogError("Error en RestarConexion " & Err.description)
    ```
  - Esto demuestra inequívocamente que el autor copió y pegó de manera descuidada el manejador de errores de `SecurityIp.IpRestarConexion` en `clsAntiMassClon.VaciarColeccion()`, sin que exista vínculo de ejecución entre ambos módulos.

---

## 5. Mapeo de Call Sites en el Repositorio Legacy

Se examinaron todas las apariciones de `clsAntiMassClon`, `aClon`, `MaxPersonajes` y `VaciarColeccion` en el código fuente de Argentum Online:

1. **Instanciación y Alcance Global**:
   - [`legacy/server/Codigo/Declares.bas:39`](../../legacy/server/Codigo/Declares.bas#L39):
     ```vb
     Public aClon As New clsAntiMassClon
     ```
     Declara la variable de alcance global `aClon` mediante instanciación diferida (*auto-instancing*).
2. **Sitio de Verificación: Creación de Nuevo Personaje (`HandleLoginNewChar`)**:
   - [`legacy/server/Codigo/Protocol.bas:1502`](../../legacy/server/Codigo/Protocol.bas#L1502):
     ```vb
     If aClon.MaxPersonajes(UserList(UserIndex).ip) Then
         Call WriteErrorMsg(UserIndex, "Has creado demasiados personajes.")
         Call FlushBuffer(UserIndex)
         Call CloseSocket(UserIndex)
         Exit Sub
     End If
     ```
     Al procesar el paquete `LoginNewChar`, el servidor consulta a `aClon`. Si superara el límite, responde con un mensaje de error y desconecta el socket.
3. **Sitio de Mantenimiento Periódico: Guardado del Mundo (`AutoSave_Timer`)**:
   - [`legacy/server/Codigo/frmMain.frm:405`](../../legacy/server/Codigo/frmMain.frm#L405):
     ```vb
     If Minutos >= MinutosWs Then
         Call ES.DoBackUp
         Call aClon.VaciarColeccion
         Minutos = 0
     End If
     ```
     Cada vez que se completa un ciclo de guardado global (*WorldSave*, configurable mediante `MinutosWs`), se invoca `VaciarColeccion` para reiniciar los contadores de todas las IPs.

---

## 6. Diagnóstico de Código Muerto e Inoperancia en Producción

### 6.1. La Falla de la Compilación Condicional
El archivo de proyecto del servidor de producción [`legacy/server/SERVER.VBP:76`](../../legacy/server/SERVER.VBP#L76) define:
```ini
CondComp="UsarQueSocket = 1 : ConUpTime = 1"
```
Al no estar listada `SeguridadAlkon`:
* `#If SeguridadAlkon` se evalúa como `0` (`False`) durante la compilación.
* Las líneas 58 a 63 de `clsAntiMassClon.cls` son omitidas del ejecutable final.
* Por lo tanto, **jamás se inserta ninguna IP en `m_coleccion`**.

### 6.2. Comportamiento en Ejecución Real (v0.13.0)
1. Durante toda la vida útil del proceso `server.exe`, `m_coleccion.Count` se mantiene invariable en `0`.
2. Cuando `HandleLoginNewChar` convoca `aClon.MaxPersonajes(sIp)`:
   - El ciclo `For i = 1 To m_coleccion.Count` no itera ninguna vez (`1 To 0`).
   - El flujo salta directamente a la línea 65: `MaxPersonajes = False`.
   - La condición de bloqueo en `Protocol.bas:1502` **nunca se cumplió jamás en la historia del servidor 0.13.0**.
3. Cuando el timer de autoguardado ejecuta `aClon.VaciarColeccion()` en `frmMain.frm:405`:
   - El ciclo de purga ejecuta `For i = 1 To 0`, resultando en una operación completamente nula (*no-op*).

### 6.3. Ausencia del Tipo `UserIpAdress`
Una búsqueda exhaustiva en todo el árbol de código fuente (servidor y cliente) confirma que `UserIpAdress` **no existe**:
* No es una clase (`UserIpAdress.cls`).
* No es una estructura `Type` en ningún `.bas`.
* No está exportada por ninguna librería de tipos (`.tlb`) referenciada en `SERVER.VBP`.

Si un programador hubiera intentado definir `SeguridadAlkon = 1` en `CondComp`, el compilador de VB6 se hubiera detenido de inmediato con el error fatal:
```text
Compile error: User-defined type not defined
```
Esto confirma de manera concluyente que la clase fue abandonada a medio desarrollar antes del release de la versión 0.12 / 0.13 y nunca formó parte de la lógica operativa del juego.

---

## 7. Justificación de la Exclusión del Port C++

Portar `clsAntiMassClon` a C++ e implementarlo funcionalmente violaría los principios rectores del proyecto definidos en [`docs/CONVENTIONS.md`](../CONVENTIONS.md):
1. **Preservación de la lógica de producción**: Reimplementar una clase que nunca funcionó en producción alteraría el comportamiento observable del servidor respecto a la versión 0.13.0 real de VB6.
2. **Prohibición de invención de tipos**: Recrear `UserIpAdress` o sustituirlo por estructuras inventadas contradice la directiva de porting estructural libre de rediseños arbitrarios.
3. **Eliminación de sobrecarga inútil**: Mantener una colección que consume ciclos de CPU para no rechazar conexiones ni guardar estado constituye deuda técnica y desperdicio de recursos.

---

## 8. Impacto y Acciones en el Andamiaje de Migración

1. **`Declares` (`src/server/Declares.hpp` / `src/server/Declares.cpp`)**:
   - Se elimina el forward-declaration stub `class clsAntiMassClon {};`.
   - Se retira la variable global `std::unique_ptr<clsAntiMassClon> aClon;`.
   - Se incluye un comentario histórico referenciando este documento de auditoría.
2. **`Protocol.bas` (Módulo #16)**:
   - Al implementar `HandleLoginNewChar` en C++, se omitirá explícitamente el chequeo `If aClon.MaxPersonajes(...) Then`.
3. **Timers y Bucle Principal (Módulo #42 `modNuevoTimer` / `GameLogic` / `frmMain.frm`)**:
   - En la rutina de autoguardado (`DoBackUp` / `AutoSave_Timer`), se omitirá la invocación a `aClon.VaciarColeccion()`.
4. **Master Bug Ledger (`docs/implementation/KNOWN-LEGACY-BUGS.md`)**:
   - Registrado formalmente como la **Entrada #18** de bugs y quirks históricos.
5. **Plan de Porting (`docs/implementation/00-port-plan.md`)**:
   - Módulo #13 marcado como `EXCLUIDO (Código Muerto)`.
