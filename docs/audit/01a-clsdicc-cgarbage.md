---
area: estructura-del-proyecto
source_files:
  - legacy/server/Codigo/clsdicc.cls
  - legacy/server/Codigo/cGarbage.cls
  - legacy/server/Codigo/clsClan.cls
  - legacy/server/Codigo/Acciones.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/Declares.bas
tags: [auditoria, capa-0, clsdicc, cgarbage, vb6, cpp, diccionario, garbage-collector]
last_updated: 2026-09-06
---

# Auditoría de Módulos Capa 0: clsdicc.cls y cGarbage.cls

Este documento presenta la auditoría técnica detallada de los módulos de **Capa 0** `legacy/server/Codigo/clsdicc.cls` y `legacy/server/Codigo/cGarbage.cls`, cerrando la brecha de cobertura identificada en la estructura general del proyecto.

---

## Resumen

- **`clsdicc.cls` (`diccionario`)**: Es una clase contenedora que implementa una tabla asociativa clave-valor de capacidad fija (100 elementos) basada en un arreglo estático con búsqueda lineal $O(N)$ e insensibilidad a mayúsculas/minúsculas. Posee un método especializado (`MayorValor`) para determinar la clave con el valor numérico más alto y contabilizar empates. En todo el servidor legacy, se utiliza únicamente en `clsClan.cls` para la contabilización de votos en las elecciones de líderes de clan.
- **`cGarbage.cls` (`cGarbage`)**: Es un Data Transfer Object (DTO) o estructura plana compuesta por tres campos públicos (`map`, `X`, `Y`). **No es un recolector de basura de memoria ni un workaround para ciclos de referencias en VB6**, sino una estructura para registrar la ubicación en el mapa de objetos temporales del mundo (específicamente fogatas creadas con la habilidad de supervivencia) que deben eliminarse durante la rutina `LimpiarMundo`.

---

## Hallazgos

### 1. Auditoría de `clsdicc.cls` (`diccionario`)

- **Estructura Interna de Datos**: Utiliza un arreglo estático de tamaño fijo `Private p_elementos(1 To 100) As diccElem`, donde `diccElem` es un `Type` privado con `clave As String` y `def As Variant`.
- **Mecanismo de Búsqueda y Complejidad**: Realiza una búsqueda secuencial lineal $O(N)$ recorriendo desde el índice 1 hasta `p_cant`. Las claves se normalizan siempre a mayúsculas usando `UCase$`.
- **Capacidad y Crecimiento**: Límite estático estricto de 100 elementos (`MAX_ELEM = 100`). No posee reasignación ni crecimiento dinámico. Intentar insertar una clave nueva cuando `p_cant == 100` provoca que `AtPut` devuelva `False` sin modificar la estructura.
- **Comportamiento en Claves Duplicadas**: Si se llama a `AtPut` con una clave ya existente, el valor `.def` se actualiza in-place y retorna `True` sin incrementar `p_cant`.
- **Comportamiento en Claves Inexistentes**: La consulta `At` sobre una clave no registrada retorna la constante `Null` de VB6 (`Variant`).
- **Garantías de Orden**: `AtIndex(i)` retorna la clave almacenada en la posición de inserción `i` (base 1), preservando el orden en que se agregaron las claves distintas.
- **Método `MayorValor`**: Recorre los elementos asumiendo que los valores son numéricos (`CInt(def)`). Mantiene una variable `max` inicializada en `-1`, construye una lista separada por comas de las claves que empatan en la puntuación máxima y devuelve en el parámetro `cant` (pasado por referencia) la cantidad de claves empatadas.
- **Único Uso en el Codebase**: Se utiliza exclusivamente en `legacy/server/Codigo/clsClan.cls`, dentro de la función `ContarVotos`, instanciándolo con `Set d = New diccionario` para acumular los votos recibidos por cada candidato durante las elecciones internas del clan.

### 2. Auditoría de `cGarbage.cls` (`cGarbage`)

- **Estructura e Interfaz Pública**: Módulo de clase minimalista sin métodos, constructores ni destructores. Expone exactamente tres variables públicas de tipo `Integer`: `map`, `X` y `Y`.
- **Trazabilidad de Uso**:
  1. **Declaración global**: `legacy/server/Codigo/Declares.bas` define la colección global `Public TrashCollector As New Collection`.
  2. **Creación/Encolado**: En `legacy/server/Codigo/Acciones.bas` (función `CrearFuego` / habilidad Supervivencia), al encender con éxito una fogata, se asigna `Obj.ObjIndex = FOGATA` en el tile del mapa con `MakeObj` y se crea un registro de basura:
     ```vb
     Dim Fogatita As New cGarbage
     Fogatita.Map = Map
     Fogatita.X = X
     Fogatita.Y = Y
     Call TrashCollector.Add(Fogatita)
     ```
  3. **Limpieza/Descolado**: En `legacy/server/Codigo/General.bas` (procedimiento `LimpiarMundo`), durante el ciclo periódico de mantenimiento del servidor, se recorre la colección en orden inverso:
     ```vb
     For i = TrashCollector.Count To 1 Step -1
         Set d = TrashCollector(i)
         Call EraseObj(1, d.Map, d.X, d.Y)
         Call TrashCollector.Remove(i)
         Set d = Nothing
     Next i
     ```
- **Investigación de Gestión de Memoria y Referencias Circulares**:
  - **Diagnóstico**: La clase `cGarbage` **no tiene relación alguna con la recolección de basura de objetos de memoria ni con romper ciclos de referencias circulares en el modelo de conteo de referencias de VB6**. El término `cGarbage` y el nombre de la colección `TrashCollector` son una **anomalía de nomenclatura (misnomer)** del código original de AO; se refieren a "basura" en el sentido de objetos fijos/temporales depositados en el suelo del mundo virtual (fogatas) que deben limpiarse periódicamente.
  - **Evaluación para C++**: En C++, este problema de ciclos de referencias circulares (que en otros escenarios requeriría `std::weak_ptr` para evitar fugas de memoria con `std::shared_ptr`) **no existe en este módulo**. Para el port a C++, `cGarbage` no necesita ser una clase ni un gestor de memoria, sino una simple estructura plana de coordenadas (`struct ElementoBasura { int map; int x; int y; };`) o una lista de posiciones `WorldPos` almacenada en un `std::vector` dentro del sistema del mundo o del bucle de timers.

---

## Lógica y Datos Extraídos

### Interfaz Pública de `clsdicc.cls` (`diccionario`)

| Firma de Método / Propiedad | Parámetros | Tipo de Retorno | Descripción y Comportamiento |
| :--- | :--- | :--- | :--- |
| `CantElem` *(Property Get)* | Ninguno | `Integer` | Devuelve la cantidad de elementos activos almacenados (`p_cant`). |
| `AtPut` *(Function)* | `ByVal clave As String`, `ByRef elem As Variant` | `Boolean` | Normaliza `clave` a mayúsculas (`UCase$`). Si `LenB(clave) == 0`, retorna `False`. Si la clave ya existe, actualiza su valor `.def = elem` y retorna `True`. Si es una clave nueva y `p_cant < 100`, la agrega al final y retorna `True`. Si `p_cant == 100`, retorna `False`. |
| `At` *(Function)* | `ByVal clave As String` | `Variant` | Normaliza `clave` a mayúsculas (`UCase$`). Busca secuencialmente en el arreglo. Devuelve la definición `.def` si la encuentra, o `Null` si la clave no está presente. |
| `AtIndex` *(Function)* | `ByVal i As Integer` | `String` | Retorna la clave almacenada en la posición de índice 1-based `i` de `p_elementos`. |
| `MayorValor` *(Function)* | `ByRef cant As Integer` *(salida)* | `String` | Parsea los valores a `Integer` (`CInt`), encuentra el valor máximo ($\ge 0$), cuenta en `cant` la cantidad de claves que empatan en el máximo y devuelve sus nombres concatenados por coma (ej. `"JUGADOR1,JUGADOR2"`). |
| `DumpAll` *(Sub)* | Ninguno | `Void` | Resetea todas las posiciones (1 a 100) asignando `.clave = vbNullString` y `.def = Null`. Establece `p_cant = 0`. |

### Interfaz Pública de `cGarbage.cls` (`cGarbage`)

| Campo | Tipo | Acceso | Descripción |
| :--- | :--- | :--- | :--- |
| `map` | `Integer` | Public | Número de mapa del objeto temporal a limpiar. |
| `X` | `Integer` | Public | Coordenada X del mapa. |
| `Y` | `Integer` | Public | Coordenada Y del mapa. |

---

- **Especificación de Migración a C++**: Para ver las decisiones de diseño C++, la regla de nombres, la estrategia de comparación case-insensitive y las notas de implementación de `cGarbage` y `clsdicc`, consultá [`03-clsdicc.md`](../implementation/03-clsdicc.md) y [`05-cgarbage.md`](../implementation/05-cgarbage.md).

---

## Preguntas Abiertas

- **Formato del Valor en `MayorValor`**: El método `MayorValor` utiliza `CInt(p_elementos(i).def)` asumiendo que el valor del diccionario es siempre convertible a entero. En C++, la implementación de este método en el adaptador del diccionario debe contemplar tipos enteros o plantillas de tipos numéricos.
