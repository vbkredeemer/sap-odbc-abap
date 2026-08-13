# SAP ODBC Driver via Z_EXECUTE_SQL — Installation Guide

## Voraussetzungen

1. **SAP NWRFC SDK 7.50** (Windows x64) — `libsapnwrfc.dll` + Header-Dateien
2. **SAP-System** mit aktiviertem Funktionsbaustein `Z_EXECUTE_SQL`
   (siehe sap-jdbc-abap Projekt für die SAP-Seite)
3. **Windows x64**
4. **CMake 3.20+** und **MSVC 2019+** (Visual Studio Build Tools)

## Build

```bash
mkdir build && cd build
cmake .. -DNWRFCSDK_DIR="C:/path/to/nwrfcsdk"
cmake --build . --config Release
```

Das erzeugt `sapodbcabap.dll`.

## Installation

### 1. DLLs kopieren

Kopiere folgende Dateien in ein Verzeichnis, z.B. `C:\SAP\ODBC\`:
- `sapodbcabap.dll` (der Treiber)
- `libsapnwrfc.dll` (aus dem NWRFC SDK)

### 2. ODBC-Treiber registrieren

Als Administrator in der Registry:

```
HKLM\SOFTWARE\ODBC\ODBCINST.INI\SAP via Z_EXECUTE_SQL
    Driver = C:\SAP\ODBC\sapodbcabap.dll
    Setup = C:\SAP\ODBC\sapodbcabap.dll
```

Und unter `HKLM\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers`:
```
SAP via Z_EXECUTE_SQL = Installed
```

### 3. Datenquelle anlegen (System-DSN)

```
HKLM\SOFTWARE\ODBC\ODBC.INI\SAP_ODBC_ABAP
    Driver = C:\SAP\ODBC\sapodbcabap.dll
    Host = sap-prod.firma.de
    SysNr = 10
    Client = 100
    User = DEIN_USER
    Password = ********
    Lang = EN
```

### 4. Verbindung testen

In Excel: Daten → Externe Daten abrufen → Aus anderen Quellen → ODBC-Datenquelle → "SAP_ODBC_ABAP" auswählen.

ODBC-Verbindungsstring (für SQLDriverConnect):
```
Host=sap-prod.firma.de;SysNr=10;Client=100;User=DEIN_USER;Password=********;Lang=EN
```

## Nutzung in Excel

1. Daten → Externe Daten abrufen → Aus anderen Quellen → ODBC
2. DSN "SAP_ODBC_ABAP" auswählen
3. SQL eingeben: `SELECT * FROM MARA LIMIT 100`
4. Daten werden geladen

## Nutzung in Power BI

1. Daten abrufen → Andere → ODBC
2. DSN "SAP_ODBC_ABAP" auswählen
3. Erweiterte Optionen: SQL eingeben
4. `SELECT * FROM MARA LIMIT 100`

## Fehlerbehebung

| Problem | Lösung |
|---------|--------|
| `libsapnwrfc.dll not found` | DLL aus NWRFC SDK ins gleiche Verzeichnis wie sapodbcabap.dll oder nach `C:\Windows\System32` |
| `Function Z_EXECUTE_SQL not found` | Funktionsbaustein in SAP nicht angelegt — siehe sap-jdbc-abap Projekt |
| `[IM002] Data source not found` | DSN nicht korrekt in der Registry registriert |
| `RFC connection failed` | Host, Systemnr. oder Client falsch |
| `SAP error: ...` | Fehler aus Z_EXECUTE_SQL — SQL-Syntax prüfen |

## Einschränkungen

- **Nur User-Queries** — keine Tabellenanzeige, kein Spalten-Discovery, kein Auto-Complete
- **Read-only** — `IV_READONLY = 'X'` ist hardcoded
- **Keine Transaktionen** — jede Query ist eigenständig
- **Keine Parameter-Bindings** — SQL als direkter String
- **ODBC 3.8** — minimale API, kompatibel mit Excel, Power BI, Access