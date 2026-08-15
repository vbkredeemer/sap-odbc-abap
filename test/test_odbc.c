/*
 * test_odbc.c — Simple ODBC test program
 * Uses the ODBC Driver Manager (like Excel does) to connect to our driver.
 * Logs every step to C:\Scripts\SAP_ODBC\test_odbc.log
 *
 * Build: x86_64-w64-mingw32-gcc -o test_odbc.exe test_odbc.c -lodbc32
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
        default: {
            static char buf[32];
            sprintf(buf, "%d", (int)rc);
            return buf;
        }
    }
}

static void logDiag(SQLSMALLINT htype, SQLHANDLE h, const char* func) {
    SQLCHAR state[6] = {0};
    SQLINTEGER native = 0;
    SQLCHAR msg[1024] = {0};
    SQLSMALLINT msglen = 0;
    SQLRETURN rc = SQLGetDiagRec(htype, h, 1, state, &native, msg, sizeof(msg), &msglen);
    if (rc == SQL_SUCCESS) {
        logmsg("  Diag: state=[%s] native=%d msg=[%s]\n", state, (int)native, msg);
    } else {
        logmsg("  Diag: SQLGetDiagRec returned %s (no error info available)\n", retStr(rc));
    }
}

int main(int argc, char* argv[]) {
    /* Open log file */
    g_log = fopen("C:\\Scripts\\SAP_ODBC\\test_odbc.log", "w");
    if (!g_log) {
        g_log = fopen("test_odbc.log", "w");
    }
    if (!g_log) {
        MessageBoxA(NULL, "Cannot open log file", "Test ODBC", MB_OK);
        return 1;
    }

    logmsg("=== ODBC Test Program ===\n");
    logmsg("Date: %s %s\n", __DATE__, __TIME__);

    /* Check ODBC Driver Manager version */
    SQLCHAR dmVer[256] = {0};
    SQLSMALLINT dmVerLen = 0;
    /* SQLGetInfo needs a connection, so let's just print the ODBC version */
    logmsg("ODBC headers: SQL_OV_ODBC3=%d\n", SQL_OV_ODBC3);

    /* Step 1: Allocate environment handle */
    logmsg("\n--- Step 1: SQLAllocHandle(SQL_HANDLE_ENV) ---\n");
    SQLHENV henv = SQL_NULL_HENV;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    logmsg("SQLAllocHandle(ENV) = %s, henv=%p\n", retStr(rc), (void*)henv);
    if (rc != SQL_SUCCESS) {
        logmsg("FATAL: Cannot allocate environment handle\n");
        logDiag(SQL_HANDLE_ENV, henv, "SQLAllocHandle");
        goto done;
    }

    /* Step 2: Set ODBC version to 3.x */
    logmsg("\n--- Step 2: SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3) ---\n");
    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    logmsg("SQLSetEnvAttr = %s\n", retStr(rc));
    if (rc != SQL_SUCCESS) {
        logDiag(SQL_HANDLE_ENV, henv, "SQLSetEnvAttr");
        goto done;
    }

    /* Step 3: Allocate connection handle */
    logmsg("\n--- Step 3: SQLAllocHandle(SQL_HANDLE_DBC) ---\n");
    SQLHDBC hdbc = SQL_NULL_HDBC;
    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    logmsg("SQLAllocHandle(DBC) = %s, hdbc=%p\n", retStr(rc), (void*)hdbc);
    if (rc != SQL_SUCCESS) {
        logDiag(SQL_HANDLE_ENV, henv, "SQLAllocHandle");
        goto done;
    }

    /* Step 4: Check which functions the driver supports */
    logmsg("\n--- Step 4: SQLGetFunctions(SQL_API_ODBC3_ALL_FUNCTIONS) ---\n");
    SQLUSMALLINT funcBuf[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
    memset(funcBuf, 0, sizeof(funcBuf));
    rc = SQLGetFunctions(hdbc, SQL_API_ODBC3_ALL_FUNCTIONS, funcBuf);
    logmsg("SQLGetFunctions(ODBC3_ALL) = %s\n", retStr(rc));
    if (rc == SQL_SUCCESS) {
        /* Check a few key functions */
        int funcs[] = {
            SQL_API_SQLALLOCHANDLE, SQL_API_SQLFREEHANDLE,
            SQL_API_SQLCONNECT, SQL_API_SQLDRIVERCONNECT, SQL_API_SQLEXECDIRECT,
            SQL_API_SQLFETCH, SQL_API_SQLGETDATA, SQL_API_SQLNUMRESULTCOLS,
            SQL_API_SQLDESCRIBECOL, SQL_API_SQLGETINFO, SQL_API_SQLGETFUNCTIONS,
            SQL_API_SQLTABLES, SQL_API_SQLCOLUMNS, SQL_API_SQLGETTYPEINFO,
            SQL_API_SQLBINDCOL, SQL_API_SQLPREPARE, SQL_API_SQLEXECUTE,
            SQL_API_SQLSETCONNECTATTR, SQL_API_SQLSETSTMTATTR,
            SQL_API_SQLGETCONNECTATTR, SQL_API_SQLGETSTMTATTR,
            SQL_API_SQLGETDIAGREC, SQL_API_SQLERROR,
            SQL_API_SQLDISCONNECT, SQL_API_SQLNATIVESQL,
            SQL_API_SQLENDTRAN, SQL_API_SQLMORERESULTS,
            SQL_API_SQLCANCEL, SQL_API_SQLCLOSECURSOR,
            SQL_API_SQLGETENVATTR, SQL_API_SQLSETENVATTR
        };
        const char* fnames[] = {
            "SQLAllocHandle", "SQLFreeHandle",
            "SQLConnect", "SQLDriverConnect", "SQLExecDirect",
            "SQLFetch", "SQLGetData", "SQLNumResultCols",
            "SQLDescribeCol", "SQLGetInfo", "SQLGetFunctions",
            "SQLTables", "SQLColumns", "SQLGetTypeInfo",
            "SQLBindCol", "SQLPrepare", "SQLExecute",
            "SQLSetConnectAttr", "SQLSetStmtAttr",
            "SQLGetConnectAttr", "SQLGetStmtAttr",
            "SQLGetDiagRec", "SQLError",
            "SQLDisconnect", "SQLNativeSql",
            "SQLEndTran", "SQLMoreResults",
            "SQLCancel", "SQLCloseCursor",
            "SQLGetEnvAttr", "SQLSetEnvAttr"
        };
        for (int i = 0; i < (int)(sizeof(funcs)/sizeof(funcs[0])); i++) {
            int supported = SQL_FUNC_EXISTS(funcBuf, funcs[i]);
            logmsg("  %s (id=%d): %s\n", fnames[i], funcs[i],
                   supported == SQL_TRUE ? "TRUE" : "FALSE");
        }
    } else {
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLGetFunctions");
    }

    /* Step 5: Connect using SQLDriverConnect */
    logmsg("\n--- Step 5: SQLDriverConnect ---\n");
    /* Try DSN=SAP_DAA first, fallback to direct connection string */
    const char* connStr = (argc > 1) ? argv[1] : "DSN=SAP_DAA";
    logmsg("Connection string: %s\n", connStr);
    SQLCHAR outConn[1024] = {0};
    SQLSMALLINT outConnLen = 0;
    rc = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connStr, SQL_NTS,
                          outConn, sizeof(outConn), &outConnLen, SQL_DRIVER_NOPROMPT);
    logmsg("SQLDriverConnect = %s\n", retStr(rc));
    logmsg("Output connection string: [%s]\n", outConn);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        logmsg("\nTrying SQLConnect instead...\n");
        /* Try SQLConnect with DSN=SAP_DAA */
        rc = SQLConnect(hdbc, (SQLCHAR*)"SAP_DAA", SQL_NTS, NULL, 0, NULL, 0);
        logmsg("SQLConnect = %s\n", retStr(rc));
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            logDiag(SQL_HANDLE_DBC, hdbc, "SQLConnect");
            logmsg("FATAL: Cannot connect to data source\n");
            goto cleanup;
        }
    }

    /* Step 6: Get some info */
    logmsg("\n--- Step 6: SQLGetInfo ---\n");
    struct {
        SQLUSMALLINT type;
        const char* name;
    } infos[] = {
        {SQL_DBMS_NAME, "SQL_DBMS_NAME"},
        {SQL_DBMS_VER, "SQL_DBMS_VER"},
        {SQL_DRIVER_NAME, "SQL_DRIVER_NAME"},
        {SQL_DRIVER_ODBC_VER, "SQL_DRIVER_ODBC_VER"},
        {SQL_DRIVER_VER, "SQL_DRIVER_VER"},
        {SQL_DATA_SOURCE_NAME, "SQL_DATA_SOURCE_NAME"},
        {SQL_SERVER_NAME, "SQL_SERVER_NAME"},
        {SQL_DATA_SOURCE_READ_ONLY, "SQL_DATA_SOURCE_READ_ONLY"},
    };
    for (int i = 0; i < (int)(sizeof(infos)/sizeof(infos[0])); i++) {
        SQLCHAR buf[256] = {0};
        SQLSMALLINT bufLen = 0;
        rc = SQLGetInfo(hdbc, infos[i].type, buf, sizeof(buf), &bufLen);
        logmsg("  %s = %s (rc=%s)\n", infos[i].name, buf, retStr(rc));
    }

    /* Step 7: Allocate statement handle */
    logmsg("\n--- Step 7: SQLAllocHandle(SQL_HANDLE_STMT) ---\n");
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    logmsg("SQLAllocHandle(STMT) = %s, hstmt=%p\n", retStr(rc), (void*)hstmt);
    if (rc != SQL_SUCCESS) {
        logDiag(SQL_HANDLE_DBC, hdbc, "SQLAllocHandle");
        goto cleanup;
    }

    /* Step 8: Execute query */
    logmsg("\n--- Step 8: SQLExecDirect('SELECT * FROM MARA') ---\n");
    rc = SQLExecDirect(hstmt, (SQLCHAR*)"SELECT * FROM MARA", SQL_NTS);
    logmsg("SQLExecDirect = %s\n", retStr(rc));
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        logDiag(SQL_HANDLE_STMT, hstmt, "SQLExecDirect");
        logmsg("Trying 'SELECT MATNR FROM MARA' instead...\n");
        rc = SQLExecDirect(hstmt, (SQLCHAR*)"SELECT MATNR FROM MARA", SQL_NTS);
        logmsg("SQLExecDirect = %s\n", retStr(rc));
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            logDiag(SQL_HANDLE_STMT, hstmt, "SQLExecDirect");
            goto cleanup;
        }
    }

    /* Step 9: Get number of result columns */
    logmsg("\n--- Step 9: SQLNumResultCols ---\n");
    SQLSMALLINT numCols = 0;
    rc = SQLNumResultCols(hstmt, &numCols);
    logmsg("SQLNumResultCols = %s, numCols=%d\n", retStr(rc), (int)numCols);
    if (rc != SQL_SUCCESS) logDiag(SQL_HANDLE_STMT, hstmt, "SQLNumResultCols");

    /* Step 10: Describe columns */
    logmsg("\n--- Step 10: SQLDescribeCol ---\n");
    for (SQLSMALLINT i = 1; i <= numCols && i <= 10; i++) {
        SQLCHAR colName[256] = {0};
        SQLSMALLINT colNameLen = 0, dataType = 0, nullable = 0;
        SQLULEN colSize = 0;
        SQLSMALLINT decimalDigits = 0;
        rc = SQLDescribeCol(hstmt, i, colName, sizeof(colName), &colNameLen,
                           &dataType, &colSize, &decimalDigits, &nullable);
        logmsg("  Col %d: name=[%s] type=%d size=%lu rc=%s\n",
               (int)i, colName, (int)dataType, (unsigned long)colSize, retStr(rc));
    }

    /* Step 11: Fetch rows */
    logmsg("\n--- Step 11: SQLFetch / SQLGetData ---\n");
    int rowCount = 0;
    while (rowCount < 5) {
        rc = SQLFetch(hstmt);
        logmsg("  SQLFetch = %s\n", retStr(rc));
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            if (rc == SQL_NO_DATA) {
                logmsg("  No more data (SQL_NO_DATA)\n");
            } else {
                logDiag(SQL_HANDLE_STMT, hstmt, "SQLFetch");
            }
            break;
        }
        rowCount++;
        /* Get data for first 3 columns */
        for (SQLUSMALLINT i = 1; i <= 3 && i <= (SQLUSMALLINT)numCols; i++) {
            SQLCHAR buf[1024] = {0};
            SQLLEN ind = 0;
            rc = SQLGetData(hstmt, i, SQL_C_CHAR, buf, sizeof(buf), &ind);
            logmsg("    Col %d: [%s] (ind=%ld, rc=%s)\n",
                   (int)i, buf, (long)ind, retStr(rc));
        }
    }
    logmsg("Fetched %d rows\n", rowCount);

    /* Step 12: Get row count */
    logmsg("\n--- Step 12: SQLRowCount ---\n");
    SQLLEN rowCount2 = 0;
    rc = SQLRowCount(hstmt, &rowCount2);
    logmsg("SQLRowCount = %s, rows=%ld\n", retStr(rc), (long)rowCount2);

cleanup:
    /* Cleanup */
    logmsg("\n--- Cleanup ---\n");
    if (hstmt != SQL_NULL_HSTMT) {
        rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        logmsg("SQLFreeHandle(STMT) = %s\n", retStr(rc));
    }
    if (hdbc != SQL_NULL_HDBC) {
        rc = SQLDisconnect(hdbc);
        logmsg("SQLDisconnect = %s\n", retStr(rc));
        rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        logmsg("SQLFreeHandle(DBC) = %s\n", retStr(rc));
    }
    if (henv != SQL_NULL_HENV) {
        rc = SQLFreeHandle(SQL_HANDLE_ENV, henv);
        logmsg("SQLFreeHandle(ENV) = %s\n", retStr(rc));
    }

done:
    logmsg("\n=== Test Complete ===\n");
    if (g_log) fclose(g_log);

    /* Show message box */
    MessageBoxA(NULL,
        "Test complete. Check C:\\Scripts\\SAP_ODBC\\test_odbc.log for results.",
        "ODBC Test", MB_OK | MB_SETFOREGROUND);
    return 0;
}