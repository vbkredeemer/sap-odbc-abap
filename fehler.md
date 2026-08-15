=== ODBC Test v4 — Focused on failing queries ===
Date: Aug 15 2026 09:41:45
Only tests the 3 queries that return wrong data.
Skips all working steps (connect, SQLGetInfo, SQLGetTypeInfo).

Connected to SAP_DAA

========================================
--- TEST 1: Single column — data empty ---
SQL: SELECT MATNR FROM MARA
========================================

SQLAllocHandle(STMT) = SQL_SUCCESS
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS

SQLNumResultCols = SQL_SUCCESS, numCols=1

  Col 1: name=[MATNR] type=12 size=80 nullable=2 rc=SQL_SUCCESS

Fetching rows:

  SQLFetch[0] = SQL_SUCCESS
    Col 1: [DABF_000118_TO] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[1] = SQL_SUCCESS
    Col 1: [DABF_000123_M3] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[2] = SQL_SUCCESS
    Col 1: [DABF_000043_M3] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[3] = SQL_SUCCESS
    Col 1: [DABF_000043_TO] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[4] = SQL_SUCCESS
    Col 1: [DABF_000044_M3] (ind=14, rc=SQL_SUCCESS)

SQLRowCount = SQL_SUCCESS, rows=13359

(TEST 1: Single column — data empty done)


========================================
--- TEST 2: Two columns — only 1 returned ---
SQL: SELECT MATNR, ERNAM FROM MARA
========================================

SQLAllocHandle(STMT) = SQL_SUCCESS
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS

SQLNumResultCols = SQL_SUCCESS, numCols=1

  Col 1: name=[MATNR] type=12 size=80 nullable=2 rc=SQL_SUCCESS

Fetching rows:

  SQLFetch[0] = SQL_SUCCESS
    Col 1: [DABF_000118_TO] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[1] = SQL_SUCCESS
    Col 1: [DABF_000123_M3] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[2] = SQL_SUCCESS
    Col 1: [DABF_000043_M3] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[3] = SQL_SUCCESS
    Col 1: [DABF_000043_TO] (ind=14, rc=SQL_SUCCESS)

  SQLFetch[4] = SQL_SUCCESS
    Col 1: [DABF_000044_M3] (ind=14, rc=SQL_SUCCESS)

SQLRowCount = SQL_SUCCESS, rows=13359

(TEST 2: Two columns — only 1 returned done)


========================================
--- TEST 3: Star query — cols 3+4 empty ---
SQL: SELECT * FROM MARA
========================================

SQLAllocHandle(STMT) = SQL_SUCCESS
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS

SQLNumResultCols = SQL_SUCCESS, numCols=4

  Col 1: name=[MANDT] type=12 size=6 nullable=2 rc=SQL_SUCCESS
  Col 2: name=[MATNR] type=12 size=80 nullable=2 rc=SQL_SUCCESS
  Col 3: name=[] type=12 size=4280 nullable=2 rc=SQL_SUCCESS
  Col 4: name=[] type=12 size=40 nullable=2 rc=SQL_SUCCESS

Fetching rows:

  SQLFetch[0] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000118_TO] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)

  SQLFetch[1] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000123_M3] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)

  SQLFetch[2] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000043_M3] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)

  SQLFetch[3] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000043_TO] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)

  SQLFetch[4] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000044_M3] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)

SQLRowCount = SQL_SUCCESS, rows=13359

(TEST 3: Star query — cols 3+4 empty done)


--- Cleanup ---
Done.

=== Test v4 Complete ===



[2026-08-15 20:28:21] DllMain: DLL_PROCESS_ATTACH - DLL loaded successfully (SQL_SUCCESS)
[2026-08-15 20:28:21] SQLAllocHandle: entry (-)
[2026-08-15 20:28:21] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLSetEnvAttr: entry (-)
[2026-08-15 20:28:22] SQLSetEnvAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLAllocHandle: entry (-)
[2026-08-15 20:28:22] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLSetConnectAttr: entry (-)
[2026-08-15 20:28:22] SQLSetConnectAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLDriverConnect: entry (-)
[2026-08-15 20:28:22] SQLDriverConnect: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLError: entry (-)
[2026-08-15 20:28:22] SQLError: exit (SQL_NO_DATA)
[2026-08-15 20:28:22] SQLGetFunctions: entry (-)
[2026-08-15 20:28:22] SQLGetFunctions: ODBC3_ALL: arr[0]=16368 arr[1]=28 arr[2]=47872 arr[3]=24640 size=250 (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetFunctions: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetInfo: entry (-)
[2026-08-15 20:28:22] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLAllocHandle: entry (-)
[2026-08-15 20:28:22] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:22] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:22] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:22] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:22] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLExecDirect: entry (-)
[2026-08-15 20:28:22] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLExecDirect: SQL: SELECT MATNR FROM MARA (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLExecDirect: Simple table read: table=MARA fields=MATNR where= (SQL_SUCCESS)
[2026-08-15 20:28:22] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=1 (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLNumResultCols: entry (-)
[2026-08-15 20:28:25] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLDescribeCol: entry (-)
[2026-08-15 20:28:25] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFetch: entry (-)
[2026-08-15 20:28:25] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetData: entry (-)
[2026-08-15 20:28:25] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFetch: entry (-)
[2026-08-15 20:28:25] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetData: entry (-)
[2026-08-15 20:28:25] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFetch: entry (-)
[2026-08-15 20:28:25] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetData: entry (-)
[2026-08-15 20:28:25] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFetch: entry (-)
[2026-08-15 20:28:25] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetData: entry (-)
[2026-08-15 20:28:25] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFetch: entry (-)
[2026-08-15 20:28:25] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetData: entry (-)
[2026-08-15 20:28:25] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLRowCount: entry (-)
[2026-08-15 20:28:25] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLFreeHandle: entry (-)
[2026-08-15 20:28:25] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLAllocHandle: entry (-)
[2026-08-15 20:28:25] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:25] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:25] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:25] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:25] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: entry (-)
[2026-08-15 20:28:25] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: SQL: SELECT MATNR, ERNAM FROM MARA (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: Simple table read: table=MARA fields=MATNR,ERNAM where= (SQL_SUCCESS)
[2026-08-15 20:28:25] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=1 (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLNumResultCols: entry (-)
[2026-08-15 20:28:28] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLDescribeCol: entry (-)
[2026-08-15 20:28:28] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFetch: entry (-)
[2026-08-15 20:28:28] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetData: entry (-)
[2026-08-15 20:28:28] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFetch: entry (-)
[2026-08-15 20:28:28] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetData: entry (-)
[2026-08-15 20:28:28] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFetch: entry (-)
[2026-08-15 20:28:28] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetData: entry (-)
[2026-08-15 20:28:28] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFetch: entry (-)
[2026-08-15 20:28:28] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetData: entry (-)
[2026-08-15 20:28:28] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFetch: entry (-)
[2026-08-15 20:28:28] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetData: entry (-)
[2026-08-15 20:28:28] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLRowCount: entry (-)
[2026-08-15 20:28:28] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLFreeHandle: entry (-)
[2026-08-15 20:28:28] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLAllocHandle: entry (-)
[2026-08-15 20:28:28] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:28] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:28] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:28] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLGetStmtAttr: entry (-)
[2026-08-15 20:28:28] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: entry (-)
[2026-08-15 20:28:28] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: SQL: SELECT * FROM MARA (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: Simple table read: table=MARA fields=* where= (SQL_SUCCESS)
[2026-08-15 20:28:28] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=4 (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLNumResultCols: entry (-)
[2026-08-15 20:28:32] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLDescribeCol: entry (-)
[2026-08-15 20:28:32] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLDescribeCol: entry (-)
[2026-08-15 20:28:32] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLDescribeCol: entry (-)
[2026-08-15 20:28:32] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLDescribeCol: entry (-)
[2026-08-15 20:28:32] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFetch: entry (-)
[2026-08-15 20:28:32] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFetch: entry (-)
[2026-08-15 20:28:32] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFetch: entry (-)
[2026-08-15 20:28:32] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFetch: entry (-)
[2026-08-15 20:28:32] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFetch: entry (-)
[2026-08-15 20:28:32] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLGetData: entry (-)
[2026-08-15 20:28:32] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLRowCount: entry (-)
[2026-08-15 20:28:32] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFreeHandle: entry (-)
[2026-08-15 20:28:32] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLDisconnect: entry (-)
[2026-08-15 20:28:32] SQLDisconnect: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFreeHandle: entry (-)
[2026-08-15 20:28:32] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] SQLFreeHandle: entry (-)
[2026-08-15 20:28:32] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 20:28:32] DllMain: DLL_PROCESS_DETACH - DLL unloaded (SQL_SUCCESS)



**** Log file opened at 2026-08-15 20:28:22.013567 UTC+02:00 (Mitteleuropäische Zeit), Encoding UTF-8
NW RFC Library: SDK variant, Release 750 Patch Level 18
Compilation date          : Dec  1 2025 21:35:24
CPIC library              : 754.2025.08.18 version 3
NI library                : 40
Kernel Release            : 754 Patch Level 627
Current working directory : C:\Scripts\SAP_ODBC
Program                   : test_odbc4
Process ID                : 26700
User                      : gros
Hardware                  : PC with Windows NT 32x AMD64 Level 6 (Mod 183 Step 1)
Binary Type               : 64bit
Operating_system          : Windows NT 10.0
Hostname                  : PCKL669
IP address                : 192.168.1.64
IPv6 address              : fe80::31dd:f2cb:8abf:83e6
NI IPv6 status            : inactive
Global trace level        : 0 : None

2026-08-15 20:28:22.013625 [25204] >> Info entry
	Did not find config file C:\Scripts\SAP_ODBC\sapnwrfc.ini.
