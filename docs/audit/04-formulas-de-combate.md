---
area: formulas-de-combate
source_files:
  - legacy/server/Codigo/SistemaCombate.bas
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/NPCs.bas
  - legacy/server/Codigo/ModoParty.bas
tags: [combate, formulas, daño, evasion, experiencia, regeneracion]
last_updated: 2026-09-06
---

## Resumen
Las fórmulas matemáticas de combate, evasión, experiencia y regeneración se ejecutan exclusivamente en el servidor (`legacy/server/`). No existe cálculo de combate en el cliente.

## Hallazgos

- **Cálculo de Daño Físico**:
  - `CalcularDaño` computa el daño final considerando el arma equipada, modificadores por atributo de fuerza y la clase del atacante, reduciendo el valor por la defensa de armadura, casco y escudo de la víctima.
  > Fuente: `legacy/server/Codigo/SistemaCombate.bas`, función `CalcularDaño`
  - Si el atacante es un NPC, el daño se basa en `NpcList(NpcIndex).Stats.MinHit` y `MaxHit`.
  > Fuente: `legacy/server/Codigo/SistemaCombate.bas`, función `NpcAtacaUsuario`

- **Probabilidad de Impacto / Evasión**:
  - `ProbabilidadGolpe` calcula la tasa de acierto entre 10% y 90% utilizando las habilidades de combate del atacante contra la evasión o escudo del objetivo.
  > Fuente: `legacy/server/Codigo/SistemaCombate.bas`, función `ProbabilidadGolpe`

- **Experiencia Ganada al Matar**:
  - Al morir un NPC, la experiencia se multiplica por la constante del servidor `EXP_MUL` y se otorga al jugador o se reparte en la party.
  > Fuente: `legacy/server/Codigo/NPCs.bas`, procedimiento `MuereNpc`<br>Fuente: `legacy/server/Codigo/Modulo_UsUaRiOs.bas`, procedimiento `GiveEXP`

- **Regeneración de Salud y Maná**:
  - Ocurre mediante timers en el bucle principal invocando `RegenerarHP` y `RegenerarMana`, dependiendo del estado de descanso o meditación.
  > Fuente: `legacy/server/Codigo/Modulo_UsUaRiOs.bas`, procedimientos `RegenerarHP`, `RegenerarMana`, `SanarUsuario`, `RestaurarManar`

## Lógica y Datos Extraídos

### Pseudocódigo de Cálculo de Daño (`CalcularDaño`)

```vb
' Fuente: legacy/server/Codigo/SistemaCombate.bas -> CalcularDaño
Function CalcularDaño(ByVal UserIndex As Integer, Optional ByVal NpcIndex As Integer = 0) As Long
    Dim DañoArma As Long
    Dim ModificadorFuerza As Single
    Dim DañoBase As Long
    Dim DefensaVictima As Long

    ' 1. Daño de Arma o Golpe Desarmado
    If UserList(UserIndex).Inventario.WeaponEqpSlot > 0 Then
        DañoArma = RandomNumber(OBJDAT(UserList(UserIndex).Inventario.WeaponEqpSlot).MinHit, _
                                OBJDAT(UserList(UserIndex).Inventario.WeaponEqpSlot).MaxHit)
    Else
        DañoArma = RandomNumber(1, 2)
    End If

    ' 2. Modificador por Fuerza del Atacante
    ModificadorFuerza = (UserList(UserIndex).Stats.UserAtributos(eAtributos.Fuerza) - 10) / 2
    DañoBase = DañoArma + ModificadorFuerza + UserList(UserIndex).Stats.ModificadorDañoClase

    ' 3. Defensa de la Víctima (Armadura + Casco + Escudo)
    DefensaVictima = ObtenerDefensaAbsoluta(VictimaIndex)

    ' 4. Daño Final Garantizado Mínimo 1
    CalcularDaño = Maximo(1, DañoBase - DefensaVictima)
End Function
```

### Pseudocódigo de Probabilidad de Impacto (`ProbabilidadGolpe`)

```vb
' Fuente: legacy/server/Codigo/SistemaCombate.bas -> ProbabilidadGolpe
Function ProbabilidadGolpe(ByVal SkillAtacante As Long, ByVal SkillDefensor As Long) As Long
    Dim Chance As Long
    Chance = 50 + (SkillAtacante - SkillDefensor) / 2
    
    ' Clampeo estricto entre 10% y 90%
    If Chance < 10 Then Chance = 10
    If Chance > 90 Then Chance = 90
    
    ProbabilidadGolpe = Chance
End Function
```

## Preguntas Abiertas
- La constante de multiplicador de experiencia `EXP_MUL` puede modificarse dinámicamente desde el archivo `Server.ini` sin requerir recompilación.
