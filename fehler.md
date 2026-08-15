=== ODBC Test Program v3 (no SQLTables) ===
Date: Aug 14 2026 21:27:33

--- Step 1: Environment ---
SQLAllocHandle(ENV) = SQL_SUCCESS
SQLSetEnvAttr = SQL_SUCCESS

--- Step 2: Connection ---
SQLAllocHandle(DBC) = SQL_SUCCESS
Connecting with: DSN=SAP_DAA
SQLDriverConnect = SQL_SUCCESS
Output: [DSN=SAP_DAA;]

--- Step 3: SQLGetInfo ---
  DBMS_NAME = SAP via Z_EXECUTE_SQL (rc=SQL_SUCCESS)
  SERVER_NAME = jbklsapas1daa.jbdmn.de (rc=SQL_SUCCESS)

--- Step 4: Data Queries (no metadata) ---

--- 4a Simple: SQLExecDirect('SELECT MATNR FROM MARA') ---
SQLAllocHandle(STMT) = SQL_SUCCESS, hstmt=00000273fcd07eb0
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS
SQLNumResultCols = SQL_SUCCESS, numCols=1
  Col 1: name=[MATNR] type=12 size=80 rc=SQL_SUCCESS
  SQLFetch[0] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[1] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[2] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[3] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[4] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[5] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[6] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[7] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[8] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[9] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
Fetched 10 rows total
SQLRowCount = SQL_SUCCESS, rows=13359
(end of 4a Simple)

--- 4b Two-Col: SQLExecDirect('SELECT MATNR, ERNAM FROM MARA') ---
SQLAllocHandle(STMT) = SQL_SUCCESS, hstmt=00000273fcd07eb0
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS
SQLNumResultCols = SQL_SUCCESS, numCols=1
  Col 1: name=[MATNR] type=12 size=80 rc=SQL_SUCCESS
  SQLFetch[0] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[1] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[2] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[3] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[4] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[5] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[6] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[7] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[8] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
  SQLFetch[9] = SQL_SUCCESS
    Col 1: [RFCSUPERUSER] (ind=12, rc=SQL_SUCCESS)
Fetched 10 rows total
SQLRowCount = SQL_SUCCESS, rows=13359
(end of 4b Two-Col)

--- 4c Where: SQLExecDirect('SELECT MATNR FROM MARA WHERE MATNR LIKE 'A%'') ---
SQLAllocHandle(STMT) = SQL_SUCCESS, hstmt=00000273fcd07eb0
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS
SQLNumResultCols = SQL_SUCCESS, numCols=1
  Col 1: name=[MATNR] type=12 size=80 rc=SQL_SUCCESS
  SQLFetch[0] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[1] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[2] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[3] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[4] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[5] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[6] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[7] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[8] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[9] = SQL_SUCCESS
    Col 1: [] (ind=0, rc=SQL_SUCCESS)
Fetched 10 rows total
SQLRowCount = SQL_SUCCESS, rows=589
(end of 4c Where)

--- 4d Star: SQLExecDirect('SELECT * FROM MARA') ---
SQLAllocHandle(STMT) = SQL_SUCCESS, hstmt=00000273fcd07eb0
Calling SQLExecDirect...
SQLExecDirect = SQL_SUCCESS
SQLNumResultCols = SQL_SUCCESS, numCols=4
  Col 1: name=[MANDT] type=12 size=6 rc=SQL_SUCCESS
  Col 2: name=[MATNR] type=12 size=80 rc=SQL_SUCCESS
  Col 3: name=[] type=12 size=4280 rc=SQL_SUCCESS
  Col 4: name=[] type=12 size=40 rc=SQL_SUCCESS
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
  SQLFetch[5] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000044_TO] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[6] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000052_TO] (ind=14, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[7] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000070_00001] (ind=17, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[8] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000070_00002] (ind=17, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)
  SQLFetch[9] = SQL_SUCCESS
    Col 1: [100] (ind=3, rc=SQL_SUCCESS)
    Col 2: [DABF_000070_00003] (ind=17, rc=SQL_SUCCESS)
    Col 3: [] (ind=0, rc=SQL_SUCCESS)
    Col 4: [] (ind=0, rc=SQL_SUCCESS)
Fetched 10 rows total
SQLRowCount = SQL_SUCCESS, rows=13359
(end of 4d Star)

--- Step 5: SQLGetTypeInfo ---
SQLGetTypeInfo = SQL_SUCCESS

--- Cleanup ---
SQLDisconnect done
All handles freed


[2026-08-15 09:04:54] DllMain: DLL_PROCESS_ATTACH - DLL loaded successfully (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLAllocHandle: entry (-)
[2026-08-15 09:04:54] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLSetEnvAttr: entry (-)
[2026-08-15 09:04:54] SQLSetEnvAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLAllocHandle: entry (-)
[2026-08-15 09:04:54] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLGetInfo: entry (-)
[2026-08-15 09:04:54] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLGetInfo: entry (-)
[2026-08-15 09:04:54] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLGetInfo: entry (-)
[2026-08-15 09:04:54] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLSetConnectAttr: entry (-)
[2026-08-15 09:04:54] SQLSetConnectAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:54] SQLDriverConnect: entry (-)
[2026-08-15 09:04:55] SQLDriverConnect: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLError: entry (-)
[2026-08-15 09:04:55] SQLError: exit (SQL_NO_DATA)
[2026-08-15 09:04:55] SQLGetFunctions: entry (-)
[2026-08-15 09:04:55] SQLGetFunctions: ODBC3_ALL: arr[0]=16368 arr[1]=28 arr[2]=47872 arr[3]=24640 size=250 (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetFunctions: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetInfo: entry (-)
[2026-08-15 09:04:55] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetInfo: entry (-)
[2026-08-15 09:04:55] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetInfo: entry (-)
[2026-08-15 09:04:55] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetInfo: entry (-)
[2026-08-15 09:04:55] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetInfo: entry (-)
[2026-08-15 09:04:55] SQLGetInfo: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLAllocHandle: entry (-)
[2026-08-15 09:04:55] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:55] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:55] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:55] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:55] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLExecDirect: entry (-)
[2026-08-15 09:04:55] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLExecDirect: SQL: SELECT MATNR FROM MARA (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLExecDirect: Simple table read: table=MARA fields=MATNR where= (SQL_SUCCESS)
[2026-08-15 09:04:55] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=1 (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLNumResultCols: entry (-)
[2026-08-15 09:04:58] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLDescribeCol: entry (-)
[2026-08-15 09:04:58] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFetch: entry (-)
[2026-08-15 09:04:58] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetData: entry (-)
[2026-08-15 09:04:58] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLRowCount: entry (-)
[2026-08-15 09:04:58] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLFreeHandle: entry (-)
[2026-08-15 09:04:58] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLAllocHandle: entry (-)
[2026-08-15 09:04:58] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:58] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:58] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:58] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLGetStmtAttr: entry (-)
[2026-08-15 09:04:58] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: entry (-)
[2026-08-15 09:04:58] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: SQL: SELECT MATNR, ERNAM FROM MARA (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: Simple table read: table=MARA fields=MATNR, ERNAM where= (SQL_SUCCESS)
[2026-08-15 09:04:58] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=1 (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLNumResultCols: entry (-)
[2026-08-15 09:05:01] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLDescribeCol: entry (-)
[2026-08-15 09:05:01] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLRowCount: entry (-)
[2026-08-15 09:05:01] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFreeHandle: entry (-)
[2026-08-15 09:05:01] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLAllocHandle: entry (-)
[2026-08-15 09:05:01] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: entry (-)
[2026-08-15 09:05:01] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: SQL: SELECT MATNR FROM MARA WHERE MATNR LIKE 'A%' (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Simple table read: table=MARA fields=MATNR where=MATNR LIKE 'A%' (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: rfcReadTableChunked OK, rows=589 cols=1 (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLNumResultCols: entry (-)
[2026-08-15 09:05:01] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLDescribeCol: entry (-)
[2026-08-15 09:05:01] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFetch: entry (-)
[2026-08-15 09:05:01] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetData: entry (-)
[2026-08-15 09:05:01] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLRowCount: entry (-)
[2026-08-15 09:05:01] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLFreeHandle: entry (-)
[2026-08-15 09:05:01] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLAllocHandle: entry (-)
[2026-08-15 09:05:01] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:01] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: entry (-)
[2026-08-15 09:05:01] SQLExecDirect: === ENTER === (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Connection OK, getting SQL text (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: SQL: SELECT * FROM MARA (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Simple table read: table=MARA fields=* where= (SQL_SUCCESS)
[2026-08-15 09:05:01] SQLExecDirect: Calling rfcReadTableChunked... (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLExecDirect: rfcReadTableChunked OK, rows=13359 cols=4 (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLExecDirect: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLNumResultCols: entry (-)
[2026-08-15 09:05:05] SQLNumResultCols: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLDescribeCol: entry (-)
[2026-08-15 09:05:05] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLDescribeCol: entry (-)
[2026-08-15 09:05:05] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLDescribeCol: entry (-)
[2026-08-15 09:05:05] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLDescribeCol: entry (-)
[2026-08-15 09:05:05] SQLDescribeCol: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFetch: entry (-)
[2026-08-15 09:05:05] SQLFetch: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetData: entry (-)
[2026-08-15 09:05:05] SQLGetData: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLRowCount: entry (-)
[2026-08-15 09:05:05] SQLRowCount: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFreeHandle: entry (-)
[2026-08-15 09:05:05] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLAllocHandle: entry (-)
[2026-08-15 09:05:05] SQLAllocHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:05] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:05] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:05] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetStmtAttr: entry (-)
[2026-08-15 09:05:05] SQLGetStmtAttr: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLGetTypeInfo: entry (-)
[2026-08-15 09:05:05] SQLGetTypeInfo: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFreeHandle: entry (-)
[2026-08-15 09:05:05] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLDisconnect: entry (-)
[2026-08-15 09:05:05] SQLDisconnect: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFreeHandle: entry (-)
[2026-08-15 09:05:05] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] SQLFreeHandle: entry (-)
[2026-08-15 09:05:05] SQLFreeHandle: exit (SQL_SUCCESS)
[2026-08-15 09:05:05] DllMain: DLL_PROCESS_DETACH - DLL unloaded (SQL_SUCCESS)



**** Log file opened at 2026-08-15 09:04:54.954260 UTC+02:00 (Mitteleuropäische Zeit), Encoding UTF-8
NW RFC Library: SDK variant, Release 750 Patch Level 18
Compilation date          : Dec  1 2025 21:35:24
CPIC library              : 754.2025.08.18 version 3
NI library                : 40
Kernel Release            : 754 Patch Level 627
Current working directory : C:\Scripts\SAP_ODBC
Program                   : test_odbc3
Process ID                : 43668
User                      : gros
Hardware                  : PC with Windows NT 32x AMD64 Level 6 (Mod 183 Step 1)
Binary Type               : 64bit
Operating_system          : Windows NT 10.0
Hostname                  : PCKL669
IP address                : 192.168.1.64
IPv6 address              : fe80::31dd:f2cb:8abf:83e6
NI IPv6 status            : inactive
Global trace level        : 0 : None

2026-08-15 09:04:54.954320 [39248] >> Info entry
	Did not find config file C:\Scripts\SAP_ODBC\sapnwrfc.ini.

=== Test Complete ===
