/*
 * test_odbc4.c — Focused test for the 3 failing queries
 * Only tests what's broken, nothing else.
 * Build: x86_64-w64-mingw32-gcc -o test_odbc4.exe test_odbc4.c -lodbc32 -luser32
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
    logmsg("\n========================================\n");
    logmsg("--- %s ---\n", label);
    logmsg("SQL: %s\n", sql);
    logmsg("========================================\n\n");

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    logmsg("SQLAllocHandle(STMT) = %s\n", retStr(rc));
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

    /* Columns */
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hstmt, &numCols);
    logmsg("\nSQLNumResultCols = %s, numCols=%d\n\n", retStr(rc), (int)numCols);

    /* Describe each column */
    for (SQLSMALLINT i = 1; i <= numCols; i++) {
        SQLCHAR colName[256] = {0};
        SQLSMALLINT colNameLen = 0, dataType = 0, nullable = 0;
        SQLULEN colSize = 0;
        SQLSMALLINT decimalDigits = 0;
        rc = SQLDescribeCol(hstmt, i, colName, sizeof(colName), &colNameLen,
                           &dataType, &colSize, &decimalDigits, &nullable);
        logmsg("  Col %d: name=[%s] type=%d size=%lu nullable=%d rc=%s\n",
               (int)i, colName, (int)dataType, (unsigned long)colSize, (int)nullable, retStr(rc));
    }

    /* Fetch 5 rows with all columns */
    logmsg("\nFetching rows:\n");
    for (int row = 0; row < 5; row++) {
        rc = SQLFetch(hstmt);
        logmsg("\n  SQLFetch[%d] = %s\n", row, retStr(rc));
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            if (rc == SQL_NO_DATA) logmsg("  -> No more data\n");
            else logDiag(SQL_HANDLE_STMT, hstmt, "SQLFetch");
            break;
        }

        for (SQLUSMALLINT i = 1; i <= (SQLUSMALLINT)numCols; i++) {
            SQLCHAR buf[4096] = {0};
            SQLLEN ind = 0;
            rc = SQLGetData(hstmt, i, SQL_C_CHAR, buf, sizeof(buf), &ind);
            logmsg("    Col %d: [%s] (ind=%ld, rc=%s)\n",
                   (int)i, buf, (long)ind, retStr(rc));
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                logDiag(SQL_HANDLE_STMT, hstmt, "SQLGetData");
            }
        }
    }

    /* Row count */
    SQLLEN affectedRows = 0;
    rc = SQLRowCount(hstmt, &affectedRows);
    logmsg("\nSQLRowCount = %s, rows=%ld\n", retStr(rc), (long)affectedRows);

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    logmsg("\n(%s done)\n\n", label);
}

int main() {
    g_log = fopen("C:\\Scripts\\SAP_ODBC\\test_odbc4.log", "w");
    if (!g_log) g_log = fopen("test_odbc4.log", "w");
    if (!g_log) { MessageBoxA(NULL, "Cannot open log file", "Test ODBC", MB_OK); return 1; }

    logmsg("=== ODBC Test v4 — Focused on failing queries ===\n");
    logmsg("Date: %s %s\n", __DATE__, __TIME__);
    logmsg("Only tests the 3 queries that return wrong data.\n");
    logmsg("Skips all working steps (connect, SQLGetInfo, SQLGetTypeInfo).\n\n");

    /* Quick connect — no logging of steps that work */
    SQLHENV henv = SQL_NULL_HENV;
    SQLHDBC hdbc = SQL_NULL_HDBC;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

    SQLCHAR outConn[1024] = {0};
    SQLSMALLINT outConnLen = 0;
    SQLRETURN rc = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)"DSN=SAP_DAA", SQL_NTS,
                          outConn, sizeof(outConn), &outConnLen, SQL_DRIVER_NOPROMPT);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        logmsg("FATAL: Cannot connect: %s\n", retStr(rc));
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto done;
    }
    logmsg("Connected to SAP_DAA\n");

    /* Test 1: SELECT MATNR FROM MARA → data is empty (ind=0) */
    testQuery(hdbc, "TEST 1: Single column — data empty",
              "SELECT MATNR FROM MARA");

    /* Test 2: SELECT MATNR, ERNAM FROM MARA → only 1 col, wrong data */
    testQuery(hdbc, "TEST 2: Two columns — only 1 returned",
              "SELECT MATNR, ERNAM FROM MARA");

    /* Test 3: SELECT * FROM MARA → cols 3+4 empty/no name */
    testQuery(hdbc, "TEST 3: Star query — cols 3+4 empty",
              "SELECT * FROM MARA");

    /* Cleanup */
    logmsg("\n--- Cleanup ---\n");
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    logmsg("Done.\n");

done:
    logmsg("\n=== Test v4 Complete ===\n");
    if (g_log) fclose(g_log);
    MessageBoxA(NULL,
        "Test v4 complete. Check C:\\Scripts\\SAP_ODBC\\test_odbc4.log",
        "ODBC Test v4", MB_OK | MB_SETFOREGROUND);
    return 0;
}