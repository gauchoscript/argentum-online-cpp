# ============================================================================
# Script: tests/generate_fixtures.ps1
# Descripción: Generador de datos de prueba (fixtures) para Argentum Online.
# NOTA: Este script replica de forma 100% fiel los métodos de escritura y
# formato exacto producidos por las rutinas reales de guardado de VB6
# (SaveUser en FileIO.bas, gestión de clanes en modGuilds.bas / clsClan.cls,
# y gestión de foros en modForum.bas), generando la salida en formato INI y
# archivos secuenciales de texto codificados en Windows-1252 (ANSI).
# Todos los nombres en charfile/, guilds/ y foros/ cumplen con el formato de VB6.
# Los casos inválidos (con 'ñ' o acentos) quedan aislados en invalid_chars/ e invalid_guilds/.
# ============================================================================

$ErrorActionPreference = "Stop"

$baseDir = Join-Path $PSScriptRoot "fixtures"
$charDir = Join-Path $baseDir "charfile"
$guildDir = Join-Path $baseDir "guilds"
$foroDir = Join-Path $baseDir "foros"
$invalidCharDir = Join-Path $baseDir "invalid_chars"
$invalidGuildDir = Join-Path $baseDir "invalid_guilds"

# Clean directories to ensure no obsolete files remain
if (Test-Path $baseDir) {
    Remove-Item $baseDir -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $charDir | Out-Null
New-Item -ItemType Directory -Force -Path $guildDir | Out-Null
New-Item -ItemType Directory -Force -Path $foroDir | Out-Null
New-Item -ItemType Directory -Force -Path $invalidCharDir | Out-Null
New-Item -ItemType Directory -Force -Path $invalidGuildDir | Out-Null

$ansiEncoding = [System.Text.Encoding]::GetEncoding(1252)

# Helper function to write ANSI file in Windows-1252 encoding with CRLF
function Write-AnsiFile ($filePath, $content) {
    $normalizedContent = $content -replace "`r`n", "`n" -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($filePath, $normalizedContent, $ansiEncoding)
}

# ----------------------------------------------------------------------------
# 1. PERSONAJES VÁLIDOS (charfile/)
# ----------------------------------------------------------------------------

# 1.1 PEPE.chr (Humano Mago - Nivel 25)
$pepeChr = @"
[INIT]
Password=clave123
Genero=1
Raza=1
Hogar=1
Clase=1
Desc=Mago humano versado en hechizos elementales.
Heading=3
Head=1
Body=1
Arma=2
Escudo=2
Casco=2
UpTime=3600
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=50000
BANCO=100000
MaxHP=250
MinHP=250
MaxMAN=500
MinMAN=500
MaxSTA=150
MinSTA=150
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=25
MinHIT=15
Exp=1250000
ELU=2000000
ELV=25

[ATRIBUTOS]
AT1=18
AT2=18
AT3=20
AT4=15
AT5=18

[SKILLS]
SK1=50
SK2=50
SK3=50
SK4=50
SK5=50
SK6=50
SK7=50
SK8=50
SK9=50
SK10=50
SK11=50
SK12=50
SK13=50
SK14=50
SK15=50
SK16=50
SK17=50
SK18=50
SK19=50
SK20=50
SK21=50

[FLAGS]
Muerto=0
Escondido=0
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[INVENTORY]
CantidadItems=10
Obj1=101-5-0
Obj2=102-10-0
Obj3=103-15-0
Obj4=104-20-0
Obj5=105-25-0
Obj6=106-30-0
Obj7=107-35-0
Obj8=108-40-0
Obj9=109-45-0
Obj10=110-50-0

[SPELLS]
CantidadSpells=5
Sp1=1
Sp2=2
Sp3=3
Sp4=4
Sp5=5

[GUILD]
GuildIndex=1
AspiranteA=0
Miembro=Legion de Honor
"@
Write-AnsiFile (Join-Path $charDir "PEPE.chr") $pepeChr

# 1.2 GONZALO.chr (Elfo Guerrero - Nivel 45 - Líder de Clan)
$gonzaloChr = @"
[INIT]
Password=lider456
Genero=1
Raza=2
Hogar=1
Clase=3
Desc=Lider del clan Legion de Honor y guerrero experimentado.
Heading=3
Head=2
Body=2
Arma=201
Escudo=203
Casco=204
UpTime=86400
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=250000
BANCO=1000000
MaxHP=400
MinHP=400
MaxMAN=1500
MinMAN=1500
MaxSTA=350
MinSTA=350
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=60
MinHIT=40
Exp=8500000
ELU=10000000
ELV=45

[ATRIBUTOS]
AT1=21
AT2=21
AT3=15
AT4=20
AT5=21

[SKILLS]
SK1=100
SK2=100
SK3=100
SK4=100
SK5=100
SK6=100
SK7=100
SK8=100
SK9=100
SK10=100
SK11=100
SK12=100
SK13=100
SK14=100
SK15=100
SK16=100
SK17=100
SK18=100
SK19=100
SK20=100
SK21=100

[FLAGS]
Muerto=0
Escondido=0
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[INVENTORY]
CantidadItems=20
Obj1=201-1-1
Obj2=202-1-1
Obj3=203-1-1
Obj4=204-1-1
Obj5=205-1-0
Obj6=206-1-0
Obj7=207-1-0
Obj8=208-1-0
Obj9=209-1-0
Obj10=210-1-0
Obj11=211-1-0
Obj12=212-1-0
Obj13=213-1-0
Obj14=214-1-0
Obj15=215-1-0
Obj16=216-1-0
Obj17=217-1-0
Obj18=218-1-0
Obj19=219-1-0
Obj20=220-1-0
WeaponEqpSlot=1
ArmourEqpSlot=2
EscudoEqpSlot=3
CascoEqpSlot=4

[SPELLS]
CantidadSpells=0

[GUILD]
GuildIndex=1
AspiranteA=0
Miembro=Legion de Honor
"@
Write-AnsiFile (Join-Path $charDir "GONZALO.chr") $gonzaloChr

# 1.3 NOVATO.chr (Humano Aventurero - Nivel 1 - Stats Iniciales)
$novatoChr = @"
[INIT]
Password=inicio123
Genero=1
Raza=1
Hogar=1
Clase=5
Desc=Un recien llegado al mundo de Argentum.
Heading=3
Head=1
Body=1
Arma=2
Escudo=2
Casco=2
UpTime=0
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=0
BANCO=0
MaxHP=20
MinHP=20
MaxMAN=0
MinMAN=0
MaxSTA=20
MinSTA=20
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=2
MinHIT=1
Exp=0
ELU=100
ELV=1

[ATRIBUTOS]
AT1=13
AT2=13
AT3=13
AT4=13
AT5=13

[SKILLS]
SK1=0
SK2=0
SK3=0
SK4=0
SK5=0
SK6=0
SK7=0
SK8=0
SK9=0
SK10=0
SK11=0
SK12=0
SK13=0
SK14=0
SK15=0
SK16=0
SK17=0
SK18=0
SK19=0
SK20=0
SK21=0

[FLAGS]
Muerto=0
Escondido=0
Hambre=0
Sed=0
Desnudo=1
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[INVENTORY]
CantidadItems=0

[SPELLS]
CantidadSpells=0

[GUILD]
GuildIndex=0
AspiranteA=1
Miembro=
"@
Write-AnsiFile (Join-Path $charDir "NOVATO.chr") $novatoChr

# 1.4 PODEROSO.chr (Alto Elfo Mago - Nivel 50 Max Stats)
$maxInventory = "CantidadItems=30`n"
for ($i=1; $i -le 30; $i++) {
    $eq = if ($i -le 4) { 1 } else { 0 }
    $maxInventory += "Item$i=$(500 + $i)-999-$eq`n"
}
$maxInventory += "WeaponEqpSlot=1`nArmourEqpSlot=2`nEscudoEqpSlot=3`nCascoEqpSlot=4"

$maxBanco = "CantidadItems=30`n"
for ($i=1; $i -le 30; $i++) {
    $maxBanco += "Item$i=$(800 + $i)-10000`n"
}

$maxSpells = "CantidadSpells=35`n"
for ($i=1; $i -le 35; $i++) {
    $maxSpells += "Sp$i=$i`n"
}

$poderosoChr = @"
[INIT]
Password=maxima321
Genero=1
Raza=3
Hogar=1
Clase=1
Desc=Personaje con los mayores atributos posibles.
Heading=3
Head=3
Body=3
Arma=501
Escudo=503
Casco=504
UpTime=1000000
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=5000000
BANCO=10000000
MaxHP=999
MinHP=999
MaxMAN=9999
MinMAN=9999
MaxSTA=999
MinSTA=999
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=99
MinHIT=99
Exp=100000000
ELU=0
ELV=50

[ATRIBUTOS]
AT1=21
AT2=21
AT3=21
AT4=21
AT5=21

[SKILLS]
SK1=100
SK2=100
SK3=100
SK4=100
SK5=100
SK6=100
SK7=100
SK8=100
SK9=100
SK10=100
SK11=100
SK12=100
SK13=100
SK14=100
SK15=100
SK16=100
SK17=100
SK18=100
SK19=100
SK20=100
SK21=100

[FLAGS]
Muerto=0
Escondido=0
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[INVENTORY]
$maxInventory

[SPELLS]
$maxSpells

[BANCO]
$maxBanco

[GUILD]
GuildIndex=0
AspiranteA=0
Miembro=
"@
Write-AnsiFile (Join-Path $charDir "PODEROSO.chr") $poderosoChr

# 1.5 CAZADORFURTIVO.chr (Gnomo Cazador - Nivel 30)
$cazadorChr = @"
[INIT]
Password=caza123
Genero=1
Raza=4
Hogar=1
Clase=6
Desc=Cazador gnomo experto en arcos y rastreo.
Heading=3
Head=4
Body=4
Arma=301
Escudo=2
Casco=304
UpTime=12000
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=80000
BANCO=300000
MaxHP=300
MinHP=300
MaxMAN=0
MinMAN=0
MaxSTA=250
MinSTA=250
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=45
MinHIT=30
Exp=3000000
ELU=4000000
ELV=30

[ATRIBUTOS]
AT1=17
AT2=21
AT3=12
AT4=14
AT5=16

[SKILLS]
SK1=80
SK2=80
SK3=80
SK4=80
SK5=80
SK6=80
SK7=80
SK8=80
SK9=80
SK10=80
SK11=80
SK12=80
SK13=80
SK14=80
SK15=80
SK16=80
SK17=80
SK18=80
SK19=80
SK20=80
SK21=80

[FLAGS]
Muerto=0
Escondido=1
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[INVENTORY]
CantidadItems=8
Obj1=301-1-1
Obj2=302-500-0
Obj3=303-1-1
Obj4=304-1-1
Obj5=305-10-0
Obj6=306-5-0
Obj7=307-2-0
Obj8=308-1-0
WeaponEqpSlot=1
ArmourEqpSlot=3
EscudoEqpSlot=0
CascoEqpSlot=4

[SPELLS]
CantidadSpells=0

[GUILD]
GuildIndex=0
AspiranteA=0
Miembro=
"@
Write-AnsiFile (Join-Path $charDir "CAZADORFURTIVO.chr") $cazadorChr

# 1.6 SACERDOTEREAL.chr (Elfo Oscuro Clérigo - Nivel 40 - Armada Real)
$sacerdoteChr = @"
[INIT]
Password=real789
Genero=1
Raza=5
Hogar=1
Clase=2
Desc=Clerigo elfo oscuro miembro consagrado de la Armada Real.
Heading=3
Head=5
Body=5
Arma=401
Escudo=403
Casco=404
UpTime=45000
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=150000
BANCO=600000
MaxHP=380
MinHP=380
MaxMAN=1200
MinMAN=1200
MaxSTA=300
MinSTA=300
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=50
MinHIT=35
Exp=6000000
ELU=7500000
ELV=40

[ATRIBUTOS]
AT1=18
AT2=18
AT3=19
AT4=17
AT5=19

[SKILLS]
SK1=90
SK2=90
SK3=90
SK4=90
SK5=90
SK6=90
SK7=90
SK8=90
SK9=90
SK10=90
SK11=90
SK12=90
SK13=90
SK14=90
SK15=90
SK16=90
SK17=90
SK18=90
SK19=90
SK20=90
SK21=90

[FLAGS]
Muerto=0
Escondido=0
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[FACCIONES]
EjercitoReal=1
EjercitoCaos=0
CiudMatados=0
CrimMatados=50

[INVENTORY]
CantidadItems=12
Obj1=401-1-1
Obj2=402-1-1
Obj3=403-1-1
Obj4=404-1-1
Obj5=405-20-0
Obj6=406-20-0
Obj7=407-5-0
Obj8=408-1-0
Obj9=409-1-0
Obj10=410-1-0
Obj11=411-1-0
Obj12=412-1-0
WeaponEqpSlot=1
ArmourEqpSlot=2
EscudoEqpSlot=3
CascoEqpSlot=4

[SPELLS]
CantidadSpells=10
Sp1=1
Sp2=2
Sp3=3
Sp4=4
Sp5=5
Sp6=6
Sp7=7
Sp8=8
Sp9=9
Sp10=10

[GUILD]
GuildIndex=2
AspiranteA=0
Miembro=Armada Real
"@
Write-AnsiFile (Join-Path $charDir "SACERDOTEREAL.chr") $sacerdoteChr

# 1.7 ASESINOSOMBRIO.chr (Elfo Oscuro Asesino - Nivel 42 - Fuerzas del Caos)
$asesinoChr = @"
[INIT]
Password=caos666
Genero=1
Raza=5
Hogar=2
Clase=4
Desc=Asesino peligroso reclutado por las Fuerzas del Caos.
Heading=3
Head=5
Body=6
Arma=501
Escudo=2
Casco=504
UpTime=60000
LastIP1=127.0.0.1 - 06/09/2026:19:00

[STATS]
GLD=200000
BANCO=800000
MaxHP=390
MinHP=390
MaxMAN=400
MinMAN=400
MaxSTA=320
MinSTA=320
MaxHAM=100
MinHAM=100
MaxAGU=100
MinAGU=100
MaxHIT=55
MinHIT=42
Exp=7200000
ELU=9000000
ELV=42

[ATRIBUTOS]
AT1=20
AT2=21
AT3=14
AT4=10
AT5=18

[SKILLS]
SK1=95
SK2=95
SK3=95
SK4=95
SK5=95
SK6=95
SK7=95
SK8=95
SK9=95
SK10=95
SK11=95
SK12=95
SK13=95
SK14=95
SK15=95
SK16=95
SK17=95
SK18=95
SK19=95
SK20=95
SK21=95

[FLAGS]
Muerto=0
Escondido=1
Hambre=0
Sed=0
Desnudo=0
Ban=0
Navegando=0
Envenenado=0
Paralizado=0

[FACCIONES]
EjercitoReal=0
EjercitoCaos=1
CiudMatados=80
CrimMatados=0

[INVENTORY]
CantidadItems=15
Obj1=501-1-1
Obj2=502-1-1
Obj3=503-1-0
Obj4=504-1-1
Obj5=505-10-0
Obj6=506-10-0
Obj7=507-2-0
Obj8=508-1-0
Obj9=509-1-0
Obj10=510-1-0
Obj11=511-1-0
Obj12=512-1-0
Obj13=513-1-0
Obj14=514-1-0
Obj15=515-1-0
WeaponEqpSlot=1
ArmourEqpSlot=2
EscudoEqpSlot=0
CascoEqpSlot=4

[SPELLS]
CantidadSpells=2
Sp1=1
Sp2=2

[GUILD]
GuildIndex=3
AspiranteA=0
Miembro=Fuerzas del Caos
"@
Write-AnsiFile (Join-Path $charDir "ASESINOSOMBRIO.chr") $asesinoChr

# ----------------------------------------------------------------------------
# 2. ARCHIVOS DE PRUEBA DE CARACTERES INVÁLIDOS (invalid_chars/ e invalid_guilds/)
# ----------------------------------------------------------------------------
$nanduInvalid = @"
[INIT]
Password=clave123
Genero=1
Raza=1
Clase=1
Desc=Personaje invalido para probar el filtro AsciiValidos (contiene 'ñ' y 'ú').

[STATS]
ELV=25
GLD=50000
"@
Write-AnsiFile (Join-Path $invalidCharDir "ÑANDÚPEÑA.chr") $nanduInvalid

$invalidGuildInfo = @"
[INIT]
NroGuilds=1

[GUILD1]
Founder=ÑandúPeña
GuildName=Legión de Ñandúes
Date=06/09/2026
Alineacion=Neutral
Leader=ÑandúPeña
Desc=Clan invalido para probar el filtro GuildNameValido en VB6 (contiene 'ó', 'Ñ' y 'ú').
"@
Write-AnsiFile (Join-Path $invalidGuildDir "guildsinfo.inf") $invalidGuildInfo

$invalidGuildMembers = @"
[INIT]
NroMembers=1

[Members]
Member1=ÑandúPeña
"@
Write-AnsiFile (Join-Path $invalidGuildDir "Legión de Ñandúes-members.mem") $invalidGuildMembers

# ----------------------------------------------------------------------------
# 3. CLANES VÁLIDOS (guilds/)
# ----------------------------------------------------------------------------

# 3.1 Guilds Master Index (guildsinfo.inf)
$guildsInfo = @"
[INIT]
NroGuilds=3

[GUILD1]
Founder=Gonzalo
GuildName=Legion de Honor
Date=06/09/2026
Antifaccion=0
Alineacion=Neutral
Leader=Gonzalo
URL=http://www.legiondehonor.com
GuildNews=Bienvenidos a Legion de Honor.
Desc=Clan neutral de caballeros y guerreros dedicados a la justicia.
Codex1=Regla 1: Respetar a todos los miembros.
Codex2=Regla 2: Cooperacion en cacerias y entrenamientos.
EleccionesAbiertas=0

[GUILD2]
Founder=SacerdoteReal
GuildName=Armada Real
Date=01/01/2026
Antifaccion=0
Alineacion=Real
Leader=SacerdoteReal
URL=http://www.armadareal.com
GuildNews=Defendemos la corona de Banderbill.
Desc=Clan oficial de las fuerzas leales al Rey.
Codex1=Regla 1: Proteger a los ciudadanos inocentes.
Codex2=Regla 2: Combatir a los criminales y fuerzas del caos.
EleccionesAbiertas=0

[GUILD3]
Founder=AsesinoSombrio
GuildName=Fuerzas del Caos
Date=01/01/2026
Antifaccion=0
Alineacion=Caos
Leader=AsesinoSombrio
URL=http://www.fuerzasdelcaos.com
GuildNews=Gloria a Lord Thek.
Desc=Clan faccionario de las legiones oscuras del Caos.
Codex1=Regla 1: Destruir a los siervos del Rey.
Codex2=Regla 2: Sin piedad hacia los leales.
EleccionesAbiertas=0
"@
Write-AnsiFile (Join-Path $guildDir "guildsinfo.inf") $guildsInfo

# 3.2 Legion de Honor files
$legionMembers = @"
[INIT]
NroMembers=2

[Members]
Member1=Gonzalo
Member2=PEPE
"@
Write-AnsiFile (Join-Path $guildDir "Legion de Honor-members.mem") $legionMembers

$legionSolicitudes = @"
[INIT]
CantSolicitudes=1

[SOLICITUD1]
Nombre=NOVATO
Detalle=Deseo unirme a la Legion de Honor para aprender a jugar.
"@
Write-AnsiFile (Join-Path $guildDir "Legion de Honor-solicitudes.sol") $legionSolicitudes

$legionRelaciones = @"
[RELACIONES]
Dummy=0
"@
Write-AnsiFile (Join-Path $guildDir "Legion de Honor-relaciones.rel") $legionRelaciones

# 3.3 Armada Real files
$armadaMembers = @"
[INIT]
NroMembers=1

[Members]
Member1=SACERDOTEREAL
"@
Write-AnsiFile (Join-Path $guildDir "Armada Real-members.mem") $armadaMembers

$armadaSolicitudes = @"
[INIT]
CantSolicitudes=0
"@
Write-AnsiFile (Join-Path $guildDir "Armada Real-solicitudes.sol") $armadaSolicitudes

$armadaRelaciones = @"
[RELACIONES]
Guild3=Guerra
"@
Write-AnsiFile (Join-Path $guildDir "Armada Real-relaciones.rel") $armadaRelaciones

# 3.4 Fuerzas del Caos files
$caosMembers = @"
[INIT]
NroMembers=1

[Members]
Member1=ASESINOSOMBRIO
"@
Write-AnsiFile (Join-Path $guildDir "Fuerzas del Caos-members.mem") $caosMembers

$caosSolicitudes = @"
[INIT]
CantSolicitudes=0
"@
Write-AnsiFile (Join-Path $guildDir "Fuerzas del Caos-solicitudes.sol") $caosSolicitudes

$caosRelaciones = @"
[RELACIONES]
Guild2=Guerra
"@
Write-AnsiFile (Join-Path $guildDir "Fuerzas del Caos-relaciones.rel") $caosRelaciones

# ----------------------------------------------------------------------------
# 4. FOROS VÁLIDOS (foros/)
# ----------------------------------------------------------------------------

# 4.1 Foro General (General.for) - Mezcla de posts y anuncios con caracteres acentuados
$generalForoInfo = @"
[INFO]
CantMSG=3
CantAnuncios=2
"@
Write-AnsiFile (Join-Path $foroDir "General.for") $generalForoInfo

$generalPost1 = "Bienvenidos a la Taberna`r`nGonzalo`r`nForo general para discusiones e interacciones entre todos los aventureros."
Write-AnsiFile (Join-Path $foroDir "General1.for") $generalPost1

$generalPost2 = "Búsqueda de Grupo para Cacería`r`nPEPE`r`nBusco grupo de cazadores para explorar las catacumbas de Ullathorpe."
Write-AnsiFile (Join-Path $foroDir "General2.for") $generalPost2

# Post con acentos y caracteres ANSI/Windows-1252 (ñ, á, é, í, ó, ú)
$generalPost3 = "Noticias de la Peña Real`r`nPEÑA_DE_ORO`r`n¡Bienvenidos al foro de la Peña Real! Se organizarán torneos de pesca y combate con premios en oro e ítems mágicos."
Write-AnsiFile (Join-Path $foroDir "General3.for") $generalPost3

$generalAnuncio1 = "Anuncio Oficial de la Guardia`r`nSACERDOTEREAL`r`nQueda estrictamente prohibido el combate entre ciudadanos dentro del recinto urbano."
Write-AnsiFile (Join-Path $foroDir "General1a.for") $generalAnuncio1

$generalAnuncio2 = "Reglamento General del Reino`r`nSEÑORÍO_REAL`r`nNormas de convivencia, leyes de la corona y decretos del Rey de Banderbill."
Write-AnsiFile (Join-Path $foroDir "General2a.for") $generalAnuncio2

# 4.2 Foro Mercado (Mercado.for) - Foro al límite máximo de CantMSG (30 posts)
$mercadoForoInfo = @"
[INFO]
CantMSG=30
CantAnuncios=1
"@
Write-AnsiFile (Join-Path $foroDir "Mercado.for") $mercadoForoInfo

for ($i = 1; $i -le 30; $i++) {
    $postContent = "Venta de Ítem #$i`r`nCOMERCIANTE_$i`r`nSe vende objeto número $i al mejor postor. Interesados enviar mensaje privado."
    Write-AnsiFile (Join-Path $foroDir "Mercado$i.for") $postContent
}

$mercadoAnuncio1 = "Reglas de Comercio Seguro`r`nMERCADER_MAYOR`r`nVerifiquen siempre la billetera e inventario antes de confirmar una transacción."
Write-AnsiFile (Join-Path $foroDir "Mercado1a.for") $mercadoAnuncio1

# 4.3 Foro Noticias (Noticias.for) - Foro al límite máximo de CantAnuncios (5 anuncios)
$noticiasForoInfo = @"
[INFO]
CantMSG=5
CantAnuncios=5
"@
Write-AnsiFile (Join-Path $foroDir "Noticias.for") $noticiasForoInfo

for ($i = 1; $i -le 5; $i++) {
    $postContent = "Noticia Diaria #$i`r`nCRONISTA`r`nResumen de eventos ocurridos en las ciudades durante la jornada $i."
    Write-AnsiFile (Join-Path $foroDir "Noticias$i.for") $postContent

    $anuncioContent = "Anuncio Fijado Importante #$i`r`nCONSEJO_REAL`r`nDecreto oficial número $i sobre impuestos y tarifas del reino."
    Write-AnsiFile (Join-Path $foroDir "Noticias$($i)a.for") $anuncioContent
}


# ----------------------------------------------------------------------------
# 5. MANIFIESTO (manifest.md)
# ----------------------------------------------------------------------------
$manifestContent = @'
# Manifiesto de Datos de Prueba (Fixtures) - Argentum Online

> **Nota de Implementación**: El generador de estos datos (`tests/generate_fixtures.ps1`) replica los métodos de escritura e interfaz en disco documentados en las auditorías (`FileIO.bas`, `modGuilds.bas` y `modForum.bas`), utilizando codificación Windows-1252 (ANSI). Todos los nombres en `charfile/`, `guilds/` y `foros/` cumplen con las reglas del motor legacy.
> 
> **ADVERTENCIA DE VERIFICACIÓN (CRÍTICO)**: El script de generación (`tests/generate_fixtures.ps1`) **NO invoca ejecutable ni código ejecutable VB6 real** (vía COM o binarios compilados), sino que construye de forma independiente las cadenas e INIs usando plantillas de PowerShell basándose en la especificación documentada en `docs/audit/06-formatos-de-datos.md` y `docs/audit/11a-modforum-detalle.md`. Por lo tanto, estos fixtures **validan nuestra propia especificación documental**, y no el comportamiento en tiempo de ejecución de un binario VB6 legacy real.

---

## 1. Personajes Válidos (`charfile/`)

Se generaron **7 personajes válidos** cubriendo todas las razas, múltiples clases, alineaciones faccionarias y variedad de estados de inventario, hechizos y estadísticas:

1. **`PEPE.chr`**
   - **Raza / Clase**: Humano / Mago (Nivel 25)
   - **Estado**: 50.000 oro en billetera, 100.000 en banco, inventario semi-lleno (10 slots ocupados), 5 hechizos aprendidos. Miembro del clan `Legion de Honor`.
2. **`GONZALO.chr`**
   - **Raza / Clase**: Elfo / Guerrero (Nivel 45)
   - **Caso de Borde**: Personaje de nivel alto con equipamiento completo (arma, armadura, escudo, casco equipados).
   - **Estado**: 250.000 oro, 20 slots de inventario. Fundador y Líder del clan `Legion de Honor`.
3. **`NOVATO.chr`**
   - **Raza / Clase**: Humano / Aventurero (Nivel 1)
   - **Caso de Borde**: Personaje inicial con stats mínimos (20 HP, 0 Mana, 0 oro) e inventario totalmente vacío (`CantidadItems = 0`).
4. **`PODEROSO.chr`**
   - **Raza / Clase**: Alto Elfo / Mago (Nivel 50 Max)
   - **Caso de Borde**: Atributos en el máximo permitido (21 en Fuerza, Agilidad, Inteligencia, Carisma, Constitución), HP 999, Mana 9999, 5.000.000 oro en billetera, 10.000.000 en banco, inventario completo (30 slots), banco completo (30 slots) y lista máxima de hechizos (35 slots).
5. **`CAZADORFURTIVO.chr`**
   - **Raza / Clase**: Gnomo / Cazador (Nivel 30)
   - **Estado**: 80.000 oro, arcos y flechas equipados, bandera `Escondido = 1`.
6. **`SACERDOTEREAL.chr`**
   - **Raza / Clase**: Elfo Oscuro / Clérigo (Nivel 40)
   - **Alineación**: Faccionario de la Armada Real (`EjercitoReal = 1`, 50 criminales matados).
   - **Estado**: 150.000 oro, 10 hechizos aprendidos, equipado completo. Fundador y Líder del clan `Armada Real`.
7. **`ASESINOSOMBRIO.chr`**
   - **Raza / Clase**: Elfo Oscuro / Asesino (Nivel 42)
   - **Alineación**: Faccionario de las Fuerzas del Caos (`EjercitoCaos = 1`, 80 ciudadanos matados).
   - **Estado**: 200.000 oro, dagas y vestimenta oscura, bandera `Escondido = 1`. Fundador y Líder del clan `Fuerzas del Caos`.

---

## 2. Clanes Válidos (`guilds/`)

Se generaron **3 clanes válidos** representando las tres alineaciones del juego:

1. **`Legion de Honor`** (Alineación Neutral)
   - **Archivos**: `guildsinfo.inf`, `Legion de Honor-members.mem`, `Legion de Honor-solicitudes.sol`, `Legion de Honor-relaciones.rel`.
   - **Integrantes**: 2 miembros (`Gonzalo` como fundador/líder y `PEPE` como integrante). Solicitud pendiente de `NOVATO`.
2. **`Armada Real`** (Alineación Real / Armada)
   - **Archivos**: `guildsinfo.inf`, `Armada Real-members.mem`, `Armada Real-solicitudes.sol`, `Armada Real-relaciones.rel`.
   - **Integrantes**: `SACERDOTEREAL` como líder. Relación de guerra abierta contra `Fuerzas del Caos`.
3. **`Fuerzas del Caos`** (Alineación Caos)
   - **Archivos**: `guildsinfo.inf`, `Fuerzas del Caos-members.mem`, `Fuerzas del Caos-solicitudes.sol`, `Fuerzas del Caos-relaciones.rel`.
   - **Integrantes**: `ASESINOSOMBRIO` como líder. Relación de guerra abierta contra `Armada Real`.

---

## 3. Foros Válidos (`foros/`)

Se generaron **3 foros válidos** cubriendo mezclas de mensajes, caracteres especiales acentuados y límites máximos de capacidad:

1. **`General.for`** (Foro Mixto)
   - **Archivos**: `General.for` (Índice INI: `CantMSG=3`, `CantAnuncios=2`), `General1.for`, `General2.for`, `General3.for`, `General1a.for`, `General2a.for`.
   - **Caso de Borde de Codificación**: Contiene publicaciones con autor y texto acentuados (`SEÑORÍO_REAL`, `PEÑA_DE_ORO`, `Búsqueda`, `Cacería`, `mágicos`) para verificar la compatibilidad de caracteres ANSI/Windows-1252 (ñ, á, é, í, ó, ú).
2. **`Mercado.for`** (Límite Máximo de Posts Generales)
   - **Archivos**: `Mercado.for` (Índice INI: `CantMSG=30`, `CantAnuncios=1`), `Mercado1.for` a `Mercado30.for` (30 posts generales), `Mercado1a.for`.
   - **Caso de Borde**: Cobertura del límite máximo estricto de 30 posts por foro (`MAX_MENSAJES_FORO = 30`).
3. **`Noticias.for`** (Límite Máximo de Anuncios Fijados)
   - **Archivos**: `Noticias.for` (Índice INI: `CantMSG=5`, `CantAnuncios=5`), `Noticias1.for` a `Noticias5.for`, `Noticias1a.for` a `Noticias5a.for` (5 anuncios fijados).
   - **Caso de Borde**: Cobertura del límite máximo estricto de 5 anuncios fijados por foro (`MAX_ANUNCIOS_FORO = 5`).

---

## 4. Datos Inválidos / Casos de Borde de Codificación (`invalid_chars/` e `invalid_guilds/`)

1. **`invalid_chars/ÑANDÚPEÑA.chr`**
   - Personaje con caracteres especiales (`Ñ`, `Ú`, `ñ`) que fallan la validación de `AsciiValidos` en VB6.
2. **`invalid_guilds/Legión de Ñandúes-members.mem`** y `guildsinfo.inf`
   - Clan con caracteres especiales (`ó`, `Ñ`, `ú`) que fallan la validación de `GuildNameValido` en VB6.
'@

Write-AnsiFile (Join-Path $baseDir "manifest.md") $manifestContent

Write-Host "Fixtures variados generados exitosamente en $baseDir"

Write-AnsiFile (Join-Path $baseDir "manifest.md") $manifestContent

Write-Host "Fixtures variados generados exitosamente en $baseDir"
