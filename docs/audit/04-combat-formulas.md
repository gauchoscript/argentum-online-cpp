---
area: combat-formulas
source_files:
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/InvUsuario.bas
  - legacy/server/Codigo/MODULO_NPCs.bas
  - legacy/server/Codigo/SistemaCombate.bas
  - legacy/server/Codigo/Trabajo.bas
tags: [combat, damage, evasion, experience, regeneration, formulas]
last_updated: 2026-09-05
---

## Summary
Combat damage, hit/miss evasion odds, experience gain allocation, and health/mana regeneration formulas are fully server-authoritative.

## Findings

- **Damage Calculation**:
  - Computed by `CalcularDaño`. Unarmed combat uses base range 4-9 plus glove bonuses from ring slot (`AnilloEqpObjIndex`).
  > Source: `legacy/server/Codigo/SistemaCombate.bas`, function `CalcularDaño`
  - Net damage subtracts target NPC defense in `UserDañoNpc`: `daño = DañoBase - Npclist(NpcIndex).Stats.def`.
  > Source: `legacy/server/Codigo/SistemaCombate.bas`, function `UserDañoNpc`

- **Hit / Miss Chance**:
  - Calculated by `UserImpactoNpc` and `NpcImpacto`. Success probability is bounded between 10% and 90%.
  > Source: `legacy/server/Codigo/SistemaCombate.bas`, function `UserImpactoNpc`<br>Source: `legacy/server/Codigo/SistemaCombate.bas`, function `NpcImpacto`

- **Experience Gained on Kill**:
  - Pro-rata EXP per hit calculated by `CalcularDarExp`: `ElDaño * (GiveEXP / MaxHp)`. Leftover unawarded EXP is granted on NPC death in `MuereNpc`.
  > Source: `legacy/server/Codigo/SistemaCombate.bas`, function `CalcularDarExp`<br>Source: `legacy/server/Codigo/MODULO_NPCs.bas`, function `MuereNpc`

- **Health & Mana Regeneration**:
  - Passive HP regen computed by `Sanar`. Blue potion mana regen computed in `InvUsuario.bas` (`Case 4`). Active meditation mana regen computed by `DoMeditar`.
  > Source: `legacy/server/Codigo/General.bas`, function `Sanar`<br>Source: `legacy/server/Codigo/InvUsuario.bas`, function `UsaItem`<br>Source: `legacy/server/Codigo/Trabajo.bas`, function `DoMeditar`

## Extracted Logic / Data

### 1. Damage Calculation (`CalcularDaño`)

```vb
FUNCTION CalcularDaño(UserIndex, NpcIndex = 0):
    WITH UserList(UserIndex):
        IF .Invent.WeaponEqpObjIndex > 0 THEN
            Arma = ObjData(.Invent.WeaponEqpObjIndex)

            IF NpcIndex > 0 THEN
                IF Arma.proyectil = 1 THEN
                    ModifClase = ModClase(.clase).DañoProyectiles
                    DañoArma = RandomNumber(Arma.MinHIT, Arma.MaxHIT)
                    DañoMaxArma = Arma.MaxHIT

                    IF Arma.Municion = 1 THEN
                        proyectil = ObjData(.Invent.MunicionEqpObjIndex)
                        DañoArma = DañoArma + RandomNumber(proyectil.MinHIT, proyectil.MaxHIT)
                        ' Note: Original comment states: ' For some reason this isn't done...
                        ' DañoMaxArma = DañoMaxArma + proyectil.MaxHIT
                    END IF
                ELSE
                    ModifClase = ModClase(.clase).DañoArmas

                    IF .Invent.WeaponEqpObjIndex = EspadaMataDragonesIndex THEN
                        IF Npclist(NpcIndex).NPCtype = DRAGON THEN
                            DañoArma = RandomNumber(Arma.MinHIT, Arma.MaxHIT)
                            DañoMaxArma = Arma.MaxHIT
                            matoDragon = True
                        ELSE
                            DañoArma = 1
                            DañoMaxArma = 1
                        END IF
                    ELSE
                        DañoArma = RandomNumber(Arma.MinHIT, Arma.MaxHIT)
                        DañoMaxArma = Arma.MaxHIT
                    END IF
                END IF
            ELSE ' Attacking Player
                IF Arma.proyectil = 1 THEN
                    ModifClase = ModClase(.clase).DañoProyectiles
                    DañoArma = RandomNumber(Arma.MinHIT, Arma.MaxHIT)
                    DañoMaxArma = Arma.MaxHIT

                    IF Arma.Municion = 1 THEN
                        proyectil = ObjData(.Invent.MunicionEqpObjIndex)
                        DañoArma = DañoArma + RandomNumber(proyectil.MinHIT, proyectil.MaxHIT)
                    END IF
                ELSE
                    ModifClase = ModClase(.clase).DañoArmas

                    IF .Invent.WeaponEqpObjIndex = EspadaMataDragonesIndex THEN
                        ModifClase = ModClase(.clase).DañoArmas
                        DañoArma = 1
                        DañoMaxArma = 1
                    ELSE
                        DañoArma = RandomNumber(Arma.MinHIT, Arma.MaxHIT)
                        DañoMaxArma = Arma.MaxHIT
                    END IF
                END IF
            END IF
        ELSE ' Unarmed / Wrestling Combat
            ModifClase = ModClase(.clase).DañoWrestling
            DañoMinArma = 4
            DañoMaxArma = 9

            ObjIndex = .Invent.AnilloEqpObjIndex
            IF ObjIndex > 0 THEN
                IF ObjData(ObjIndex).Guante = 1 THEN
                    DañoMinArma = DañoMinArma + ObjData(ObjIndex).MinHIT
                    DañoMaxArma = DañoMaxArma + ObjData(ObjIndex).MaxHIT
                END IF
            END IF

            DañoArma = RandomNumber(DañoMinArma, DañoMaxArma)
        END IF

        DañoUsuario = RandomNumber(.Stats.MinHIT, .Stats.MaxHIT)

        IF matoDragon THEN
            CalcularDaño = Npclist(NpcIndex).Stats.MinHp + Npclist(NpcIndex).Stats.def
        ELSE
            CalcularDaño = (3 * DañoArma + ((DañoMaxArma / 5) * MaximoInt(0, .Stats.UserAtributos(eAtributos.Fuerza) - 15)) + DañoUsuario) * ModifClase
        END IF
    END WITH
END FUNCTION
```

### 2. Hit / Miss Chance (`UserImpactoNpc`)

```vb
FUNCTION UserImpactoNpc(UserIndex, NpcIndex):
    Arma = UserList(UserIndex).Invent.WeaponEqpObjIndex

    IF Arma > 0 THEN
        IF ObjData(Arma).proyectil = 1 THEN
            PoderAtaque = PoderAtaqueProyectil(UserIndex)
        ELSE
            PoderAtaque = PoderAtaqueArma(UserIndex)
        END IF
    ELSE
        PoderAtaque = PoderAtaqueWrestling(UserIndex)
    END IF

    ProbExito = MaximoInt(10, MinimoInt(90, 50 + ((PoderAtaque - Npclist(NpcIndex).PoderEvasion) * 0.4)))
    UserImpactoNpc = (RandomNumber(1, 100) <= ProbExito)
END FUNCTION
```

### 3. Experience Gained on Kill (`CalcularDarExp`)

```vb
SUB CalcularDarExp(UserIndex, NpcIndex, ElDaño):
    IF ElDaño <= 0 THEN ElDaño = 0
    IF Npclist(NpcIndex).Stats.MaxHp <= 0 THEN EXIT SUB
    IF ElDaño > Npclist(NpcIndex).Stats.MinHp THEN ElDaño = Npclist(NpcIndex).Stats.MinHp

    ExpaDar = CLng(ElDaño * (Npclist(NpcIndex).GiveEXP / Npclist(NpcIndex).Stats.MaxHp))
    IF ExpaDar <= 0 THEN EXIT SUB

    IF ExpaDar > Npclist(NpcIndex).flags.ExpCount THEN
        ExpaDar = Npclist(NpcIndex).flags.ExpCount
        Npclist(NpcIndex).flags.ExpCount = 0
    ELSE
        Npclist(NpcIndex).flags.ExpCount = Npclist(NpcIndex).flags.ExpCount - ExpaDar
    END IF

    IF ExpaDar > 0 THEN
        IF UserList(UserIndex).PartyIndex > 0 THEN
            Call mdParty.ObtenerExito(UserIndex, ExpaDar, Npclist(NpcIndex).Pos.Map, Npclist(NpcIndex).Pos.X, Npclist(NpcIndex).Pos.Y)
        ELSE
            UserList(UserIndex).Stats.Exp = UserList(UserIndex).Stats.Exp + ExpaDar
            IF UserList(UserIndex).Stats.Exp > MAXEXP THEN UserList(UserIndex).Stats.Exp = MAXEXP
        END IF
        Call CheckUserLevel(UserIndex)
    END IF
END SUB
```

### 4. Health & Mana Regeneration

```vb
' Passive Health Regeneration (Sanar)
SUB Sanar(UserIndex, EnviarStats, Intervalo):
    WITH UserList(UserIndex):
        ' Odd Edge Case: Impossible AND trigger check in legacy code
        IF MapData(.Pos.Map, .Pos.X, .Pos.Y).trigger = 1 AND _
           MapData(.Pos.Map, .Pos.X, .Pos.Y).trigger = 2 AND _
           MapData(.Pos.Map, .Pos.X, .Pos.Y).trigger = 4 THEN EXIT SUB

        IF .Stats.MinHp < .Stats.MaxHp THEN
            IF .Counters.HPCounter < Intervalo THEN
                .Counters.HPCounter = .Counters.HPCounter + 1
            ELSE
                mashit = RandomNumber(2, Porcentaje(.Stats.MaxSta, 5))
                .Counters.HPCounter = 0
                .Stats.MinHp = .Stats.MinHp + mashit
                IF .Stats.MinHp > .Stats.MaxHp THEN .Stats.MinHp = .Stats.MaxHp
                EnviarStats = True
            END IF
        END IF
    END WITH
END SUB

' Blue Potion Mana Regeneration (InvUsuario.bas)
.Stats.MinMAN = .Stats.MinMAN + Porcentaje(.Stats.MaxMAN, 4) + .Stats.ELV \ 2 + 40 / .Stats.ELV
IF .Stats.MinMAN > .Stats.MaxMAN THEN .Stats.MinMAN = .Stats.MaxMAN
```

## Open Questions
- In `Sanar`, the trigger condition `trigger = 1 And trigger = 2 And trigger = 4` can never evaluate to True; in C++ this dead check should be corrected or removed.
- Ammo max hit (`proyectil.MaxHIT`) is omitted from `DañoMaxArma` calculation as noted in legacy source comments (`' For some reason this isn't done...`).
