/*
 * test_odbc2.c — Enhanced ODBC test program v2
 * Tries metadata functions before data queries to isolate the crash.
 * Build: x86_64-w64-mingw32-gcc -o test_odbc2.exe test_odbc2.c -lodbc32 -luser32
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
        case SQL_STILL_EXECUTING: return "SQL_STILL_EXECUTING";
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
    } else {
        SQLSMALLINT numCols = 0;
        rc = SQLNumResultCols(hstmt, &numCols);
        logmsg("SQLNumResultCols = %s, numCols=%d\n", retStr(rc), (int)numCols);

        /* Fetch a few rows */
        for (int row = 0; row < 3; row++) {
            rc = SQLFetch(hstmt);
            logmsg("  SQLFetch[%d] = %s\n", row, retStr(rc));
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) break;
            /* Get first 3 columns */
            for (SQLUSMALLINT i = 1; i <= 3 && i <= (SQLUSMALLINT)numCols; i++) {
                SQLCHAR buf[1024] = {0};
                SQLLEN ind = 0;
                rc = SQLGetData(hstmt, i, SQL_C_CHAR, buf, sizeof(buf), &ind);
                logmsg("    Col %d: [%s] (ind=%ld, rc=%s)\n",
                       (int)i, buf, (long)ind, retStr(rc));
            }
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    logmsg("(end of %s)\n", label);
}

int main(int argc, char* argv[]) {
    g_log = fopen("C:\\Scripts\\SAP_ODBC\\test_odbc2.log", "w");
    if (!g_log) g_log = fopen("test_odbc2.log", "w");
    if (!g_log) { MessageBoxA(NULL, "Cannot open log file", "Test ODBC", MB_OK); return 1; }

    logmsg("=== ODBC Test Program v2 ===\n");
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

    /* Step 3: SQLGetFunctions (AFTER connect — fixed!) */
    logmsg("\n--- Step 3: SQLGetFunctions (after connect) ---\n");
    SQLUSMALLINT funcBuf[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
    memset(funcBuf, 0, sizeof(funcBuf));
    rc = SQLGetFunctions(hdbc, SQL_API_ODBC3_ALL_FUNCTIONS, funcBuf);
    logmsg("SQLGetFunctions(ODBC3_ALL) = %s\n", retStr(rc));
    if (rc == SQL_SUCCESS) {
        int funcs[] = {SQL_API_SQLCONNECT, SQL_API_SQLDRIVERCONNECT, SQL_API_SQLEXECDIRECT,
            SQL_API_SQLFETCH, SQL_API_SQLGETDATA, SQL_API_SQLNUMRESULTCOLS,
            SQL_API_SQLDESCRIBECOL, SQL_API_SQLGETINFO, SQL_API_SQLGETFUNCTIONS,
            SQL_API_SQLTABLES, SQL_API_SQLCOLUMNS, SQL_API_SQLGETTYPEINFO,
            SQL_API_SQLBINDCOL, SQL_API_SQLPREPARE, SQL_API_SQLEXECUTE,
            SQL_API_SQLSETCONNECTATTR, SQL_API_SQLSETSTMTATTR,
            SQL_API_SQLGETCONNECTATTR, SQL_API_SQLGETSTMTATTR,
            SQL_API_SQLGETDIAGREC, SQL_API_SQLDISCONNECT};
        const char* fnames[] = {"SQLConnect","SQLDriverConnect","SQLExecDirect",
            "SQLFetch","SQLGetData","SQLNumResultCols","SQLDescribeCol",
            "SQLGetInfo","SQLGetFunctions","SQLTables","SQLColumns",
            "SQLGetTypeInfo","SQLBindCol","SQLPrepare","SQLExecute",
            "SQLSetConnectAttr","SQLSetStmtAttr","SQLGetConnectAttr",
            "SQLGetStmtAttr","SQLGetDiagRec","SQLDisconnect"};
        for (int i = 0; i < (int)(sizeof(funcs)/sizeof(funcs[0])); i++) {
            logmsg("  %s: %s\n", fnames[i],
                   SQL_FUNC_EXISTS(funcBuf, funcs[i]) == SQL_TRUE ? "TRUE" : "FALSE");
        }
    } else {
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLGetFunctions");
    }

    /* Step 4: SQLGetTypeInfo (pure local, no SAP call) */
    logmsg("\n--- Step 4: SQLGetTypeInfo ---\n");
    {
        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        logmsg("SQLAllocHandle(STMT) = %s\n", retStr(rc));
        if (rc == SQL_SUCCESS) {
            rc = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
            logmsg("SQLGetTypeInfo = %s\n", retStr(rc));
            if (rc != SQL_SUCCESS) logDiag(SQL_HANDLE_STMT, hstmt, "SQLGetTypeInfo");
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    }

    /* Step 5: SQLTables (metadata — calls metaGetTables → SAP) */
    logmsg("\n--- Step 5: SQLTables ---\n");
    {
        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        logmsg("SQLAllocHandle(STMT) = %s\n", retStr(rc));
        if (rc == SQL_SUCCESS) {
            logmsg("Calling SQLTables...\n");
            fflush(g_log);
            rc = SQLTables(hstmt, NULL, 0, NULL, 0, NULL, 0, (SQLCHAR*)"TABLE", SQL_NTS);
            logmsg("SQLTables = %s\n", retStr(rc));
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                logDiag(SQL_HANDLE_STMT, hstmt, "SQLTables");
            } else {
                SQLSMALLINT numCols = 0;
                SQLNumResultCols(hstmt, &numCols);
                logmsg("NumResultCols = %d\n", (int)numCols);
                /* Fetch a few rows */
                for (int i = 0; i < 5; i++) {
                    rc = SQLFetch(hstmt);
                    logmsg("  Fetch[%d] = %s\n", i, retStr(rc));
                    if (rc != SQL_SUCCESS) break;
                    SQLCHAR buf[256] = {0}; SQLLEN ind = 0;
                    SQLGetData(hstmt, 3, SQL_C_CHAR, buf, sizeof(buf), &ind); /* TABLE_NAME */
                    logmsg("    TABLE_NAME = %s\n", buf);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    }

    /* Step 6: Simple query (single column, uses Z_READ_TABLE) */
    testQuery(hdbc, "Step 6", "SELECT MATNR FROM MARA");

    /* Step 7: Full query (uses Z_READ_TABLE) */
    testQuery(hdbc, "Step 7", "SELECT * FROM MARA");

    /* Step 8: Complex query (uses Z_EXECUTE_SQL) */
    testQuery(hdbc, "Step 8", "SELECT MATNR, ERNAM FROM MARA WHERE MATNR LIKE 'A%'");

    /* Cleanup */
    logmsg("\n--- Cleanup ---\n");
    SQLDisconnect(hdbc);
    logmsg("SQLDisconnect done\n");
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    logmsg("SQLFreeHandle(DBC) done\n");
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
    logmsg("SQLFreeHandle(ENV) done\n");

done:
    logmsg("\n=== Test Complete ===\n");
    if (g_log) fclose(g_log);
    MessageBoxA(NULL,
        "Test v2 complete. Check C:\\Scripts\\SAP_ODBC\\test_odbc2.log",
        "ODBC Test v2", MB_OK | MB_SETFOREGROUND);
    return 0;
}