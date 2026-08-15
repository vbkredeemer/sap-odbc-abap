# SAP ODBC Driver for ABAP — Developer Install Guide

## Overview

ODBC driver that connects Windows applications to SAP ERP systems via the SAP NWRFC SDK.
The driver executes SQL queries against SAP by calling ABAP function modules over RFC.
It supports both simple table reads (chunked, unlimited rows) and complex SQL with JOINs and aggregations.

**Tested with:** QlikView, QlikSense

## Prerequisites

1. **SAP NWRFC SDK 7.50** (Windows x64) — `sapnwrfc.dll` + ICU DLLs (`icudt57.dll`, `icuin57.dll`, `icuuc57.dll`)
2. **SAP-System** with ABAP function modules installed:
   - `Z_EXECUTE_SQL` — for complex SQL queries (JOINs, aggregations, subqueries via ADBC)
   - `Z_READ_TABLE` — for simple table reads with chunking (unlimited rows)
   - ABAP source is in `abap-source.zip` (release) or the `abap/` directory
3. **Windows x64** target
4. **MinGW-w64 cross-compiler** on Linux (x86_64-w64-mingw32-g++)

## Build (MinGW cross-compile on Linux)

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake -DNWRFCSDK_DIR=/path/to/nwrfcsdk
cmake --build . --config Release
```

The MinGW C/C++ runtime is **statically linked** — no `libgcc_s_seh-1.dll` or `libstdc++-6.dll` required.

The build produces `sapodbcabap.dll` with A/W (ANSI/Unicode) wrapper exports for
64-bit ODBC Driver Manager compatibility.

## Installation on Windows

### 1. DLLs kopieren

Kopiere folgende **5 DLLs** in ein Verzeichnis, z.B. `C:\Scripts\SAP_ODBC\`:

| DLL | Quelle |
|-----|--------|
| `sapodbcabap.dll` | Aus dem Release / Build-Artifact |
| `sapnwrfc.dll` | SAP NWRFC SDK 7.50 (`nwrfcsdk\bin\`) |
| `icudt57.dll` | SAP NWRFC SDK 7.50 (`nwrfcsdk\bin\`) |
| `icuin57.dll` | SAP NWRFC SDK 7.50 (`nwrfcsdk\bin\`) |
| `icuuc57.dll` | SAP NWRFC SDK 7.50 (`nwrfcsdk\bin\`) |

Alle 5 DLLs müssen im selben Verzeichnis liegen (oder in `C:\Windows\System32`).

### 2. ODBC-Treiber registrieren

Als Administrator in PowerShell:

```powershell
.\install_driver.ps1
```

Das Skript registriert den ODBC-Treiber in der Windows-Registry und legt eine System-DSN an.
Interaktive Abfrage der Verbindungsparameter, oder direkt:

```powershell
.\install_driver.ps1 -DSNName "SAP_DAA" -SapHost "jbklsapas1daa.jbdmn.de"
```

> Client (100), SysNr (10) und Lang (DE) sind vorbelegt. User und Passwort werden immer abfragt.

### 3. Weitere SAP-Systeme anlegen

```powershell
.\install_driver.ps1 -DSNName "SAP_KAA" -SapHost "jbklsapas1kaa.jbdmn.de"
.\install_driver.ps1 -DSNName "SAP_PAA" -SapHost "jbklsapas1paa.jbdmn.de"
```

### 4. Registry (manual, falls ohne Skript installiert)

**Treiber:**
```
HKLM\SOFTWARE\ODBC\ODBCINST.INI\SAP via Z_EXECUTE_SQL
    Driver = C:\Scripts\SAP_ODBC\sapodbcabap.dll
    Setup = C:\Scripts\SAP_ODBC\sapodbcabap.dll
```

**Datenquelle (System-DSN):**
```
HKLM\SOFTWARE\ODBC\ODBC.INI\SAP_DAA
    Driver = C:\Scripts\SAP_ODBC\sapodbcabap.dll
    Host = jbklsapas1daa.jbdmn.de
    SysNr = 10
    Client = 100
    User = DEIN_USER
    Password = ********
    Lang = DE
```

## Connection Parameters

| Parameter | Beschreibung | Default | Pflicht |
|-----------|-------------|---------|---------|
| Host | SAP Applikationsserver (Hostname oder IP) | — | Ja |
| SysNr | Systemnummer | 10 | Ja |
| Client | SAP-Mandant | 100 | Ja |
| User | SAP-Benutzer | — | Ja |
| Password | Passwort | — | Ja |
| Lang | Anmeldesprache | DE | Nein |
| MaxRows | Maximale Zeilen pro Query (Z_EXECUTE_SQL) | 50000 | Nein |

ODBC-Verbindungsstring:
```
DSN=SAP_DAA;Host=jbklsapas1daa.jbdmn.de;SysNr=10;Client=100;User=DEIN_USER;Password=********;Lang=DE
```

## SAP Systems

| DSN | Host | Beschreibung |
|-----|------|-------------|
| SAP_DAA | jbklsapas1daa.jbdmn.de | Entwicklung |
| SAP_KAA | jbklsapas1kaa.jbdmn.de | Konsolidierung |
| SAP_PAA | jbklsapas1paa.jbdmn.de | Produktion |

Alle Systeme: Client=100, SysNr=10, Lang=DE

## Supported ODBC Functions

- `SQLTables` — Tabellen-Discovery
- `SQLColumns` — Spalten-Metadaten
- `SQLGetTypeInfo` — SQL-Datentyp-Information
- `SQLExecDirect` — SQL ausführen
- `SQLFetch` — Zeilen abrufen
- `SQLGetData` — Spaltenwerte lesen
- `SQLBindCol` — Spalten binden

Der Treiber deklariert korrekte SQL-Typen: `SQL_TYPE_DATE`, `SQL_TYPE_TIME`,
`SQL_VARBINARY`, `SQL_DECIMAL`, `SQL_VARCHAR`, `SQL_INTEGER`, etc.

## Query Routing (Dual-Mode)

| SQL-Typ | ABAP-Funktionsbaustein | Eigenschaft |
|---------|----------------------|-------------|
| Einfaches `SELECT * FROM table` | `Z_READ_TABLE` | Chunked, unbegrenzte Zeilenzahl |
| Komplexes SQL (JOIN, GROUP BY, Subquery) | `Z_EXECUTE_SQL` | ADBC-basiert, MaxRows-Limit |

Der Treiber erkennt automatisch, ob eine Query einfach oder komplex ist und wählt
den entsprechenden Funktionsbaustein.

## DATE / TIME Format

- DATE values: `YYYY-MM-DD` (ODBC-Standardformat)
- TIME values: `HH:MM:SS` (ODBC-Standardformat)

In QlikView/QlikSense: Datumswerte kommen als String im Format `YYYY-MM-DD`.
Mit der QlikView `Date()`-Funktion kann der Wert interpretiert werden:

```qlikview
Date#(ERSDA, 'YYYY-MM-DD') as ERSDA
```

## Debug Logging

Der Treiber unterstützt optionales Debug-Logging via Registry-Wert:

```
HKLM\SOFTWARE\ODBC\ODBC.INI\<DSN-Name>\LogEnable
```

- `0` (Default) — Logging aus
- `1` — Logging an (schreibt nach `C:\Temp\sap_odbc_debug.log`)

Das Logging wird beim Verbindungsaufbau gelesen. Es muss manuell in der Registry
gesetzt werden (nicht Teil des Installations-Skripts).

## Nutzung in QlikView

1. Script-Editor öffnen (Ctrl+E)
2. ODBC-Verbindung anlegen: `ODBC CONNECT TO SAP_DAA;`
3. SQL laden:
```qlikview
SQL SELECT MATNR, MTART, ERSDA FROM MARA WHERE MTART = 'FERT';
```
4. Tabellen-Discovery: QlikView zeigt verfügbare Tabellen und Spalten über `SQLTables`/`SQLColumns`

## Nutzung in QlikSense

1. Daten-Verbindung → ODBC → DSN `SAP_DAA` auswählen
2. SQL eingeben oder Tabellen aus dem Katalog wählen

## ConfigDSN / Test Connection

Der Treiber implementiert `ConfigDSN` mit einem Win32-Dialog, der einen
**Test Connection** Button enthält. Beim Anlegen einer DSN über den Windows
ODBC-Administrator (odbcad32.exe) kann die Verbindung direkt getestet werden.

## Fehlerbehebung

| Problem | Lösung |
|---------|--------|
| `Systemfehlercode 126` / Modul nicht gefunden | Eine der 5 DLLs fehlt — alle DLLs müssen im selben Verzeichnis liegen |
| `sapnwrfc.dll not found` | DLL aus NWRFC SDK (`nwrfcsdk\bin\`) kopieren — zusammen mit den 3 ICU-DLLs |
| `Function Z_EXECUTE_SQL not found` | ABAP-Funktionsbaustein nicht installiert — `abap-source.zip` ins SAP-System importieren |
| `Function Z_READ_TABLE not found` | ABAP-Funktionsbaustein nicht installiert — `abap-source.zip` ins SAP-System importieren |
| `[IM002] Data source not found` | DSN nicht korrekt in der Registry registriert — `install_driver.ps1` erneut ausführen |
| `RFC connection failed` | Host, Systemnummer oder Client falsch — Verbindungsoptionen prüfen |
| `Table XXX does not exist in DDIC` | Tabelle existiert nicht im SAP-System — Tabellennamen prüfen |
| `SAP error: ...` | Fehler aus SAP — SQL-Syntax oder Berechtigungen prüfen |

## Einschränkungen

- **Read-only** — keine INSERT/UPDATE/DELETE-Statements
- **Keine Transaktionen** — jede Query ist eigenständig
- **Keine Parameter-Bindings** — SQL als direkter String
- **Excel** — aktuell nicht kompatibel (DM-Konflikt mit A/W-Exports), QlikView/QlikSense funktionieren