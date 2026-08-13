# SAP ODBC Driver via Z_EXECUTE_SQL

Native ODBC-Treiber für SAP, der über den Custom-Funktionsbaustein `Z_EXECUTE_SQL` Native SQL auf HANA ausführt — über den Applikationsserver (RFC), lizenztechnisch sicher.

## Architektur

```
Excel / Power BI / Access / beliebiges ODBC-Tool
  ↓ ODBC
sapodbcabap.dll (unser Treiber) + libsapnwrfc.dll (SAP NWRFC SDK)
  ↓ RFC via NWRFC SDK
SAP Applikationsserver
  ↓ Z_EXECUTE_SQL (Custom Funktionsbaustein)
  ↓ ADBC (Native SQL)
SAP HANA (serverseitig, mit JOIN-Optimierung)
```

## Voraussetzungen

- SAP NWRFC SDK 7.50 (Windows x64) — `libsapnwrfc.dll` + Header-Dateien
- SAP-System mit aktiviertem Funktionsbaustein `Z_EXECUTE_SQL` (siehe sap-jdbc-abap Projekt)
- Windows x64
- CMake 3.20+
- C/C++ Compiler (MSVC 2019+ oder MinGW)

## Build

```bash
mkdir build && cd build
cmake .. -DNWRFCSDK_DIR="C:/path/to/nwrfcsdk"
cmake --build . --config Release
```

## Installation

1. `sapodbcabap.dll` + `libsapnwrfc.dll` in ein Verzeichnis kopieren
2. ODBC-Datenquelle anlegen (System-DSN):
   ```
   regedit → HKLM\SOFTWARE\ODBC\ODBC.INI\SAP_ODBC_ABAP
   Driver = C:\path\to\sapodbcabap.dll
   Host = sap-prod.firma.de
   SysNr = 10
   Client = 100
   User = DEIN_USER
   Password = ********
   ```

3. In Excel/Power BI: ODBC-Datenquelle "SAP_ODBC_ABAP" auswählen

## Lizenz

GPL-3.0 (basierend auf dem Konzept des sap-jdbc-abap Projekts)