/*
 * test_odbc3.c — ODBC test program v3
 * Skips SQLTables (which fetches ALL tables from SAP = slow).
 * Goes directly to data queries.
 * Build: x86_64-w64-mingw32-gcc -o test_odbc3.exe test_odbc3.c -lodbc32 -luser32
 */
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* g_log = NULL;

static void logmsg(const char* fmt, ...) {
    if (!g_log) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log, fmt, args);
    va_end(args);
    fflush(g_log);
}

static const char* retStr(SQLRETURN rc) {
    switch (rc) {
        case SQL_SUCCESS: return "SQL_SUCCESS";
        case SQL_SUCCESS_WITH_INFO: return "SQL_SUCCESS_WITH_INFO";
        case SQL_ERROR: return "SQL_ERROR";
        case SQL_INVALID_HANDLE: return "SQL_INVALID_HANDLE";
        case SQL_NO_DATA: return "SQL_NO_DATA";
        case SQL_NEED_DATA: return "SQL_NEED_DATA";
        default: { static char buf[32]; sprintf(buf, "%d", (int)rc); return buf; }
    }
}

static void logDiag(SQLSMALLINT htype, SQLHANDLE h, const char* func) {
    SQLCHAR state[6] = {0};
    SQLINTEGER native = 0;
    SQLCHAR msg[1024] = {0};
    SQLSMALLINT msglen = 0;
    SQLRETURN rc = SQLGetDiagRec(htype, h, 1, state, &native, msg, sizeof(msg), &msglen);
    if (rc == SQL_SUCCESS)
        logmsg("  Diag: state=[%s] native=%d msg=[%s]\n", state, (int)native, msg);
    else
        logmsg("  Diag: SQLGetDiagRec returned %s\n", retStr(rc));
}

static void testQuery(SQLHDBC hdbc, const char* label, const char* sql) {
    logmsg("\n--- %s: SQLExecDirect('%s') ---\n", label, sql);
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    logmsg("SQLAllocHandle(STMT) = %s, hstmt=%p\n", retStr(rc), (void*)hstmt);
    if (rc != SQL_SUCCESS) { logDiag(SQL_HANDLE_DBC, hdbc, "SQLAllocHandle"); return; }

    logmsg("Calling SQLExecDirect...\n");
    fflush(g_log);
    rc = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
    logmsg("SQLExecDirect = %s\n", retStr(rc));
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        logDiag(SQL_HANDLE_STMT, hstmt, "SQLExecDirect");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return;
    }

    /* Get number of result columns */
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hstmt, &numCols);
    logmsg("SQLNumResultCols = %s, numCols=%d\n", retStr(rc), (int)numCols);
    if (rc != SQL_SUCCESS) logDiag(SQL_HANDLE_STMT, hstmt, "SQLNumResultCols");

    /* Describe columns */
    for (SQLSMALLINT i = 1; i <= numCols && i <= 5; i++) {
        SQLCHAR colName[256] = {0};
        SQLSMALLINT colNameLen = 0, dataType = 0, nullable = 0;
        SQLULEN colSize = 0;
        SQLSMALLINT decimalDigits = 0;
        rc = SQLDescribeCol(hstmt, i, colName, sizeof(colName), &colNameLen,
                           &dataType, &colSize, &decimalDigits, &nullable);
        logmsg("  Col %d: name=[%s] type=%d size=%lu rc=%s\n",
               (int)i, colName, (int)dataType, (unsigned long)colSize, retStr(rc));
    }

    /* Fetch rows */
    int rowCount = 0;
    while (rowCount < 10) {
        rc = SQLFetch(hstmt);
        logmsg("  SQLFetch[%d] = %s\n", rowCount, retStr(rc));
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            if (rc == SQL_NO_DATA) logmsg("  -> No more data\n");
            else logDiag(SQL_HANDLE_STMT, hstmt, "SQLFetch");
            break;
        }
        rowCount++;
        for (SQLUSMALLINT i = 1; i <= (SQLUSMALLINT)numCols && i <= 5; i++) {
            SQLCHAR buf[1024] = {0};
            SQLLEN ind = 0;
            rc = SQLGetData(hstmt, i, SQL_C_CHAR, buf, sizeof(buf), &ind);
            logmsg("    Col %d: [%s] (ind=%ld, rc=%s)\n",
                   (int)i, buf, (long)ind, retStr(rc));
        }
    }
    logmsg("Fetched %d rows total\n", rowCount);

    /* Row count */
    SQLLEN affectedRows = 0;
    rc = SQLRowCount(hstmt, &affectedRows);
    logmsg("SQLRowCount = %s, rows=%ld\n", retStr(rc), (long)affectedRows);

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    logmsg("(end of %s)\n", label);
}

int main(int argc, char* argv[]) {
    g_log = fopen("C:\\Scripts\\SAP_ODBC\\test_odbc3.log", "w");
    if (!g_log) g_log = fopen("test_odbc3.log", "w");
    if (!g_log) { MessageBoxA(NULL, "Cannot open log file", "Test ODBC", MB_OK); return 1; }

    logmsg("=== ODBC Test Program v3 (no SQLTables) ===\n");
    logmsg("Date: %s %s\n\n", __DATE__, __TIME__);

    /* Step 1: Environment */
    logmsg("--- Step 1: Environment ---\n");
    SQLHENV henv = SQL_NULL_HENV;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    logmsg("SQLAllocHandle(ENV) = %s\n", retStr(rc));
    if (rc != SQL_SUCCESS) goto done;

    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    logmsg("SQLSetEnvAttr = %s\n", retStr(rc));

    /* Step 2: Connection */
    logmsg("\n--- Step 2: Connection ---\n");
    SQLHDBC hdbc = SQL_NULL_HDBC;
    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    logmsg("SQLAllocHandle(DBC) = %s\n", retStr(rc));
    if (rc != SQL_SUCCESS) goto done;

    const char* connStr = (argc > 1) ? argv[1] : "DSN=SAP_DAA";
    logmsg("Connecting with: %s\n", connStr);
    SQLCHAR outConn[1024] = {0};
    SQLSMALLINT outConnLen = 0;
    rc = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connStr, SQL_NTS,
                          outConn, sizeof(outConn), &outConnLen, SQL_DRIVER_NOPROMPT);
    logmsg("SQLDriverConnect = %s\n", retStr(rc));
    logmsg("Output: [%s]\n", outConn);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto done;
    }

    /* Step 3: Quick SQLGetInfo */
    logmsg("\n--- Step 3: SQLGetInfo ---\n");
    SQLCHAR buf[256] = {0}; SQLSMALLINT bufLen = 0;
    rc = SQLGetInfo(hdbc, SQL_DBMS_NAME, buf, sizeof(buf), &bufLen);
    logmsg("  DBMS_NAME = %s (rc=%s)\n", buf, retStr(rc));
    rc = SQLGetInfo(hdbc, SQL_SERVER_NAME, buf, sizeof(buf), &bufLen);
    logmsg("  SERVER_NAME = %s (rc=%s)\n", buf, retStr(rc));

    /* Step 4: Directly query data — skip SQLTables! */
    logmsg("\n--- Step 4: Data Queries (no metadata) ---\n");

    /* 4a: Simple single-column query */
    testQuery(hdbc, "4a Simple", "SELECT MATNR FROM MARA");

    /* 4b: Two columns */
    testQuery(hdbc, "4b Two-Col", "SELECT MATNR, ERNAM FROM MARA");

    /* 4c: With WHERE clause */
    testQuery(hdbc, "4c Where", "SELECT MATNR FROM MARA WHERE MATNR LIKE 'A%'");

    /* 4d: SELECT * (all columns) */
    testQuery(hdbc, "4d Star", "SELECT * FROM MARA");

    /* Step 5: SQLGetTypeInfo (local, fast) */
    logmsg("\n--- Step 5: SQLGetTypeInfo ---\n");
    {
        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        if (rc == SQL_SUCCESS) {
            rc = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
            logmsg("SQLGetTypeInfo = %s\n", retStr(rc));
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    }

    /* Cleanup */
    logmsg("\n--- Cleanup ---\n");
    SQLDisconnect(hdbc);
    logmsg("SQLDisconnect done\n");
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    logmsg("All handles freed\n");

done:
    logmsg("\n=== Test Complete ===\n");
    if (g_log) fclose(g_log);
    MessageBoxA(NULL,
        "Test v3 complete. Check C:\\Scripts\\SAP_ODBC\\test_odbc3.log",
        "ODBC Test v3", MB_OK | MB_SETFOREGROUND);
    return 0;
}