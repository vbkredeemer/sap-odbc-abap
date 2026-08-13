# Produktvergleich: Unser ODBC-Treiber vs. Qlik SAP Connector vs. Theobald Software

> Dieses Dokument ist Teil des [sap-data-replication](https://github.com/vbkredeemer/sap-data-replication) Projekts und wird hier im ODBC-Projekt referenziert.

## Kurzvergleich

| | Unser Treiber | Qlik SAP Connector | Theobald Software |
|---|---|---|---|
| **Lizenz** | GPL-3.0 (kostenlos) | Kommerziell (Qlik-Plattform) | Kommerziell (separate Lizenz) |
| **SAP-Zertifiziert** | ❌ | ✅ | ✅ |
| **ODBC-Interface** | ✅ | ❌ (Qlik-intern) | ❌ (Xtract-intern) |
| **JDBC-Interface** | ✅ (Schwesterprojekt) | ❌ | ❌ |
| **Komplexe SQL-Queries** | ✅ Serverseitig (HANA) | ❌ | ❌ |
| **Full Table Extract** | ✅ Mit Chunking | ✅ | ✅ |
| **Delta (automatisch)** | ❌ Nur manuell | ✅ ODP-Queue | ✅ Table CDC (Trigger) |
| **Von SAP Note 3255746 betroffen** | ❌ **Nein** | ❌ **Ja (ODP blockiert)** | ⚠️ Teilweise (ODP-Komponente) |
| **Zukunftssicher** | ✅ | ❌ ODP-RFC blockiert | ✅ Table CDC nicht betroffen |

## SAP Note 3255746 — Die ODP-RFC-Blockade

SAP hat im **Juni 2026** einen Security-Patch ausgerollt, der ODP-RFC-Aufrufe von Drittanbietern **technisch blockiert**. Betroffen sind:

- **Qlik Replicate** (ODP-RFC Connector)
- **Microsoft Azure Data Factory** (SAP BW CDC Connector)
- **Fivetran** (SAP NetWeaver Connector)
- **Talend, Informatica** (alle ODP-RFC-basierten Connector)
- **Theobald** (ODP Extraction Type — bietet Migration an)

Eine temporäre Opt-Out-Möglichkeit besteht bis Ende 2026. Danach ist ODP-RFC für Drittanbieter endgültig nicht mehr nutzbar.

**Unser Treiber ist nicht betroffen** — er nutzt ausschließlich Custom-Funktionsbausteine (`Z_EXECUTE_SQL`, `Z_READ_TABLE`), nicht SAP's ODP-RFC-API. SAP kann unsere Bausteine nicht blockieren.

> **Theobald-Dokumentation:** *"SAP Note 3255746 verbietet nicht RFC als Kommunikationsprotokoll. RFC bleibt voll nutzbar — für Tabellen/CDS-View-Extraktion, DeltaQ, BAPIs oder Custom-Funktionsbausteine. Nur die Nutzung des ODP Data Replication API via RFC ist eingeschränkt. Wenn Sie nicht ODP-RFC nutzen, sind Sie nicht betroffen."*

## Architektur-Vergleich

### Unser Treiber
```
Client (Excel/Power BI/QlikSense/SQL Server/DBeaver)
  ↓ ODBC API (Standard)
  sapodbcabap.dll
  ↓ SQL-Parser erkennt Query-Typ
  ├── Z_READ_TABLE (einfache Queries, mit Chunking) → Open SQL → HANA
  └── Z_EXECUTE_SQL (komplexe Queries, Joins)       → ADBC Native SQL → HANA
  ↓ RFC (NWRFC SDK)
  SAP Applikationsserver
```

### Qlik SAP Connector
```
QlikSense / Qlik Replicate
  ↓ Qlik-internes Protokoll (kein ODBC)
  ├── ODP-RFC (SAP-Standard)          ← AB JUNI 2026 BLOCKIERT
  └── Table Connector (RFC_READ_TABLE)
  ↓ RFC
  SAP Applikationsserver
```

### Theobald Software
```
Xtract Universal / Xtract IS
  ↓ Theobald-internes Protokoll (kein ODBC)
  ├── /THEO/READ_TABLE (Table)
  ├── /THEO/CDC (Table CDC, Trigger-basiert)
  ├── DeltaQ (Extractors via SAPI)
  └── ODP (wird migriert)              ← BLOCKIERT
  ↓ RFC (NWRFC SDK)
  SAP Applikationsserver
```

## Funktionsumfang im Detail

### Abfrage-Typen

| Abfrage | Unser Treiber | Qlik | Theobald |
|---|---|---|---|
| `SELECT * FROM MARA` | ✅ Chunking (Z_READ_TABLE) | ✅ | ✅ |
| `SELECT * FROM MARA WHERE MTART = 'FERT'` | ✅ Chunking | ✅ | ✅ |
| `SELECT a, b FROM MARA JOIN MARC ON a = b` | ✅ **Serverseitig (HANA)** | ❌ | ❌ |
| `SELECT COUNT(*) FROM MARA` | ✅ ADBC | ❌ | ❌ |
| `SELECT MTART, COUNT(*) FROM MARA GROUP BY MTART` | ✅ ADBC | ❌ | ❌ |
| Massendaten (Millionen Zeilen) | ✅ Chunking, unbegrenzt | ✅ | ✅ |

**Vorteil unseres Treibers:** Als Einziger unterstützt er **komplexe SQL-Abfragen serverseitig**. Qlik und Theobald können nur Flat-Table-Reads — Joins müssen client-seitig gemacht werden.

### Delta-Handling

| | Unser Treiber | Qlik ODP | Theobald CDC |
|---|---|---|---|
| Full-Load | ✅ | ✅ | ✅ |
| Delta per WHERE-Klausel | ✅ `WHERE AEDAT >= '...'` | N/A | N/A |
| Automatische Delta-Queue | ❌ | ✅ (ODP) — **blockiert** | N/A |
| Trigger-basiertes CDC | Geplant | ✅ (Replicate) | ✅ |
| DELETE-Erkennung | ⚠️ Nur über Zeitfenster | ✅ | ✅ |
| Echtzeit | ❌ Batch | ✅ Nahezu Echtzeit — **blockiert** | ✅ |

### Client-Integration

| Client | Unser Treiber | Qlik | Theobald |
|---|---|---|---|
| Microsoft Excel | ✅ Direkt (ODBC) | ✅ (über Qlik) | ⚠️ (über Umwege) |
| Power BI | ✅ Direkt (ODBC) | ✅ (über Qlik) | ⚠️ (über Umwege) |
| QlikSense | ✅ (ODBC) | ✅ Nativ | ⚠️ (über Qlik Web Connector) |
| SQL Server (Linked Server) | ✅ Nativ | ❌ | ❌ |
| SSIS (SQL Server Integration Services) | ✅ (ODBC Source) | ❌ | ✅ (Xtract IS) |
| DBeaver | ✅ (JDBC-Treiber) | ❌ | ❌ |
| Python / C# (pyodbc) | ✅ | ❌ | ❌ |

### Performance

Alle RFC-basierten Lösungen haben ähnliche Performance — der Flaschenhals ist RFC, nicht die Client-Software.

| | Weg | Relative Performance |
|---|---|---|
| Unser Treiber | RFC → Custom Baustein → HANA | Mittel |
| Qlik ODP | RFC → ODP → HANA | Mittel — **blockiert** |
| Qlik Table | RFC → RFC_READ_TABLE → HANA | Mittel (512-Byte-Limit!) |
| Theobald | RFC → Custom Baustein → HANA | Mittel |
| SLT (Referenz) | DB-Trigger → direkt | Sehr schnell (aber SAP-Lizenz) |

## Zeilenbreiten-Limit

| | Max. Zeilenbreite |
|---|---|
| Unser Treiber | **10.000 Zeichen** (ZSQL_ROW = CHAR 10000) |
| Qlik Table (RFC_READ_TABLE) | **512 Zeichen** (TAB512) — Truncation bei breiten Tabellen |
| Theobald (Z_THEO_READ_TABLE) | Variabel (Custom Baustein) |

## Zusammenfassung

### Wann unser Treiber die beste Wahl ist

- ✅ Wenn man **komplexe SQL-Abfragen** mit Joins und Aggregationen braucht
- ✅ Wenn man **ODBC als Standard-Interface** nutzen will (Excel, Power BI, SQL Server)
- ✅ Wenn man **keine Lizenzkosten** haben will
- ✅ Wenn man **unabhängig von SAP's ODP-RFC-Blockade** sein will
- ✅ Wenn man **DBeaver** oder Java-Anwendungen nutzt (JDBC-Treiber)
- ✅ Wenn man **SQL Server Linked Server** nutzen will

### Wann Qlik die beste Wahl ist

- ✅ Wenn man bereits **QlikSense als Plattform** nutzt
- ✅ Wenn man **automatisches Delta-Handling** braucht (solange ODP noch funktioniert)
- ❌ **Aber:** ODP-RFC wird ab Juni 2026 blockiert — Qlik muss auf Alternativen migrieren

### Wann Theobald die beste Wahl ist

- ✅ Wenn man **professionelles CDC mit Trigger** braucht
- ✅ Wenn man **SAP-zertifizierte Lösung** braucht
- ✅ Wenn man **kommerziellen Support** braucht
- ✅ Wenn man **SSIS-Integration** braucht (Xtract IS)

### Wann SLT die beste Wahl ist

- ✅ Wenn man **Echtzeit-Replikation** braucht
- ✅ Wenn man **sehr große Datenmengen** hat (10M+ Zeilen)
- ❌ **Aber:** Extra SAP-Lizenz erforderlich

---

*Vollständige Dokumentation aller Ansätze: https://github.com/vbkredeemer/sap-data-replication*