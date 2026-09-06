---
area: seguridad-y-autenticacion
source_files:
  - legacy/client/CODIGO/Protocol.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
tags: [seguridad, autenticacion, login, contraseñas, texto-plano, md5, vulnerabilidades]
last_updated: 2026-09-06
---

## Resumen
El proceso de autenticación de Argentum Online v0.13.0 transmite credenciales y verifica contraseñas almacenadas en texto plano en los archivos de personaje (`.chr`). Existen vulnerabilidades críticas de seguridad en la persistencia y la transmisión sin cifrar.

## Hallazgos

- **Flujo de Autenticación**:
  1. El cliente captura el usuario y contraseña en `frmConnect.frm` y llama a `WriteLoginExistingChar`.
  > Fuente: `legacy/client/CODIGO/Protocol.bas`, función `WriteLoginExistingChar`
  2. El paquete `ClientPacketID.LoginExistingChar` envía en texto plano el nombre de usuario, la contraseña, la versión del cliente y la firma MD5 del ejecutable.
  > Fuente: `legacy/client/CODIGO/Protocol.bas`, función `WriteLoginExistingChar`
  3. El servidor recibe la petición en `HandleLoginExistingChar`, busca el archivo `legacy/server/Charfile/<NOMBRE>.chr` y valida las credenciales.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleLoginExistingChar`

- **Almacenamiento de Contraseñas (VULNERABILIDAD CRÍTICA)**:
  - Las contraseñas **NO** utilizan hashing (ni SHA-256, ni bcrypt, ni MD5). Se guardan directamente en texto claro dentro del archivo `.chr`.
  ```ini
  [INIT]
  Password=mi_clave_secreta
  ```
  > Fuente: `legacy/server/Codigo/FileIO.bas`, función `CheckPassword`<br>Fuente: `legacy/server/Codigo/FileIO.bas`, procedimiento `SaveUser`

- **Verificación de Ejecutable (MD5 Check)**:
  - El cliente calcula el hash MD5 del ejecutable `Argentum.exe` y lo envía en el paquete de login para evitar modificaciones del cliente. El servidor compara este hash contra el valor en `Server.ini`.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleLoginExistingChar`

## Lógica y Datos Extraídos

### Matriz de Evaluación de Seguridad

| Aspecto de Seguridad | Estado en VB6 Legacy | Descripción del Riesgo / Comportamiento |
| :--- | :--- | :--- |
| **Almacenamiento de Claves** | ❌ **Texto Plano** | Contraseñas guardadas sin hash en `Charfile/<Nombre>.chr` bajo `Password=`. |
| **Cifrado de Red** | ❌ **Sin TLS / SSL** | Los paquetes TCP viajan en texto binario plano; susceptible a sniffing de red. |
| **Integridad del Cliente** | ⚠️ **MD5 Básico** | Envío de MD5 del ejecutable en el login; fácilmente falsificable manipulando la cola de paquetes. |
| **Validación de Datos** | ✅ **Servidor Autoritativo** | Las estadísticas, oro, inventario y posiciones ilegales son validadas por el servidor. |

## Preguntas Abiertas
- **Recomendación para C++**: En el nuevo servidor en C++, se debe introducir el hashing seguro de contraseñas mediante bcrypt o Argon2id, permitiendo una rutina de migración transparente que encripte la clave en texto plano la primera vez que el usuario inicie sesión.
