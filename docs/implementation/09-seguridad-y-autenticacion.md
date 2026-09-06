---
area: seguridad-y-autenticacion
status: not-started
audit_reference: docs/audit/09-seguridad-y-autenticacion.md
tags: [seguridad, autenticacion, login, contraseñas, texto-plano, md5, vulnerabilidades]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Hashing Seguro de Contraseñas**: Reemplazar la persistencia en texto plano con bcrypt/Argon2id. Al cargar un personaje legacy con clave en texto claro, el servidor C++ la migrará automáticamente a hash seguro al autenticar con éxito.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Mantener la compatibilidad del paquete binario de login para clientes legacy mientras se asegura el backend.

## Tareas
- [ ] Implementar módulo de autenticación con hashing bcrypt en C++.
- [ ] Crear rutina de migración automática de credenciales de `.chr` legacy a formato de hash seguro.

## Archivos de Código Relacionados
- `legacy/server/Codigo/Protocol.bas`
- `legacy/server/Codigo/FileIO.bas`
