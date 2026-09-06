---
area: login-security
source_files:
  - legacy/client/CODIGO/Protocol.bas
  - legacy/client/CODIGO/ProtocolCmdParse.bas
  - legacy/client/CODIGO/frmConnect.frm
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/TCP.bas
tags: [security, authentication, login, passwords, plaintext, vulnerability]
last_updated: 2026-09-05
---

## Summary
Authentication reads character profiles directly from flat INI files (`.chr`). Passwords are transmitted over the socket in unencrypted plaintext and stored in disk INI files in unhashed cleartext.

## Findings

- **Authentication Flow**:
  - `cmdConectarse_Click` on `frmConnect.frm` calls `WriteLoginExistingChar` sending username and plaintext password over the TCP socket.
  > Source: `legacy/client/CODIGO/Protocol.bas`, function `WriteLoginExistingChar`
  - Server `HandleLoginExistingChar` validates input strings, character file existence (`PersonajeExiste`), and IP ban status.
  > Source: `legacy/server/Codigo/Protocol.bas`, function `HandleLoginExistingChar`
  - `ConnectUser` loads `Charfile/<Username>.chr` and compares the incoming password directly against the disk INI file:
    ```vb
    If UCase$(Password) <> UCase$(GetVar(CharPath & UCase$(name) & ".chr", "INIT", "Password")) Then
    ```
  > Source: `legacy/server/Codigo/TCP.bas`, function `ConnectUser`

- **Identified Security Vulnerabilities**:
  1. **Plaintext Password Storage on Disk**: In [TCP.bas:L599](file:///c:/Users/Elio/Documents/ArgentumOnline0.13.0/legacy/server/Codigo/TCP.bas#L599) (`ConnectNewUser`), passwords are written directly as cleartext to the `.chr` INI file:
     ```vb
     Call WriteVar(CharPath & UCase$(name) & ".chr", "INIT", "Password", Password)
     ```
  2. **Unencrypted Socket Credentials**: Passwords are transmitted in cleartext over the TCP stream without TLS/SSL or password hashing.
  3. **Case-Insensitive Password Comparison**: `UCase$(Password)` reduces password entropy.
  4. **Admin Command Credential Copying**: In `HandleAlterPassword` ([Protocol.bas:L13370](file:///c:/Users/Elio/Documents/ArgentumOnline0.13.0/legacy/server/Codigo/Protocol.bas#L13370)), GMs can copy plaintext passwords between account files.

## Extracted Logic / Data

### Login Authentication Flow

```
Client (frmConnect) ---> WriteLoginExistingChar(Name, Password)
                           |
                     Unencrypted TCP
                           |
                           v
Server (Protocol.bas) --> HandleLoginExistingChar()
                           |
Server (TCP.bas) -------> ConnectUser()
                           |-- Check FileExist(Charfile/<Name>.chr)
                           |-- GetVar(Charfile/<Name>.chr, "INIT", "Password")
                           |-- Compare: UCase$(Pass) == UCase$(StoredPass)
                           v
                      LoadUserInit() & WriteLogged() [ServerPacketID.logged]
```

## Open Questions
- In the new C++ system, password hashing (e.g. Argon2id / bcrypt / PBKDF2) and network payload encryption (TLS 1.3 / Noise Protocol) must be implemented to replace legacy cleartext authentication.
