#include "sap_odbc.h"
#include "resource.h"
#include <odbcinst.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <set>

// Handle registry — maps ODBC handles to our internal structs
static std::map<SQLHANDLE, SapConnection*> g_connections;
static std::map<SQLHANDLE, SapStatement*> g_statements;

// Forward declaration (defined later in this file)
static std::string regReadString(HKEY hKey, const char* valueName);

// Critical section to protect g_connections, g_statements, g_errors maps
static CRITICAL_SECTION g_handleLock;

// RAII guard for automatic EnterCriticalSection/LeaveCriticalSection
struct LockGuard {
    CRITICAL_SECTION* cs;
    LockGuard(CRITICAL_SECTION* c) : cs(c) { EnterCriticalSection(cs); }
    ~LockGuard() { LeaveCriticalSection(cs); }
};

// ============================================================
// Debug Logging — writes to C:\Scripts\SAP_ODBC\sap_odbc_debug.log
// Logging is OFF by default. Set registry DWORD "LogEnable=1"
// under HKLM\SOFTWARE\ODBC\ODBC.INI\<DSN> to enable.
// Only errors, SQLExecDirect, SQLDriverConnect, and DllMain are logged.
// ============================================================

// Cached logging state: 0 = not checked, 1 = enabled, -1 = disabled
static int g_logEnabled = 0;       // 0 = unknown, 1 = on, -1 = off
static std::string g_logDsn;       // DSN name for registry lookups
static CRITICAL_SECTION g_logLock; // protects g_logEnabled / g_logDsn

static bool isLogEnabled() {
    // Fast path: already determined
    if (g_logEnabled != 0) return g_logEnabled == 1;

    // Slow path: read registry
    LockGuard guard(&g_logLock);
    // Double-check after acquiring lock
    if (g_logEnabled != 0) return g_logEnabled == 1;

    if (g_logDsn.empty()) {
        g_logEnabled = -1;
        return false;
    }

    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + g_logDsn;
    HKEY hKey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) {
        g_logEnabled = -1;
        return false;
    }

    DWORD val = 0;
    DWORD size = sizeof(val);
    DWORD type = 0;
    rc = RegQueryValueExA(hKey, "LogEnable", NULL, &type, (LPBYTE)&val, &size);
    RegCloseKey(hKey);

    if (rc == ERROR_SUCCESS && (type == REG_DWORD || type == REG_BINARY) && val == 1) {
        g_logEnabled = 1;
        return true;
    }
    g_logEnabled = -1;
    return false;
}

static void setLogDsn(const std::string& dsn) {
    LockGuard guard(&g_logLock);
    if (g_logDsn.empty() && !dsn.empty()) {
        g_logDsn = dsn;
        g_logEnabled = 0; // force re-evaluation
    }
}

// ============================================================
// Queried Tables Logging — appends unique table names to a file
// Enabled when registry value "TableLogPath" is set (non-empty)
// in the DSN registry key. File format: one table name per line.
// Python sync client reads the file, deduplicates, and builds
// the replication config.
// ============================================================

static std::string g_tableLogPath;          // file path from registry (empty = off)
static std::set<std::string> g_loggedTables; // in-memory dedup set
static CRITICAL_SECTION g_tableLogLock;      // protects g_tableLogPath / g_loggedTables

// Set the table log path from the DSN registry key.
// Called during SQLDriverConnect / SQLConnect after the DSN is known.
static void setTableLogPath(const std::string& dsn) {
    if (dsn.empty()) return;
    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + dsn;
    HKEY hKey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) return;
    std::string path = regReadString(hKey, "TableLogPath");
    RegCloseKey(hKey);
    LockGuard guard(&g_tableLogLock);
    g_tableLogPath = path;  // empty string if not found → logging off
}

// Log a queried table name to the CSV file (if logging is enabled).
// Uses g_loggedTables for in-memory deduplication — each unique table
// is appended only once per DLL session.
static void logQueriedTable(const std::string& tableName) {
    if (tableName.empty()) return;
    LockGuard guard(&g_tableLogLock);
    if (g_tableLogPath.empty()) return;  // logging disabled
    if (g_loggedTables.count(tableName)) return;  // already logged
    g_loggedTables.insert(tableName);
    FILE* f = fopen(g_tableLogPath.c_str(), "a");
    if (!f) return;
    fprintf(f, "%s\n", tableName.c_str());
    fflush(f);
    fclose(f);
}

static void odbcLog(const char* func, const char* msg, SQLINTEGER rc) {
    if (!isLogEnabled()) return;

    FILE* f = fopen("C:\\Scripts\\SAP_ODBC\\sap_odbc_debug.log", "a");
    if (!f) {
        // Fallback to %TEMP% if C:\Scripts\SAP_ODBC doesn't exist
        char logPath[MAX_PATH];
        GetTempPathA(MAX_PATH, logPath);
        strcat(logPath, "sap_odbc_debug.log");
        f = fopen(logPath, "a");
    }
    if (!f) return;
    time_t now = time(NULL);
    struct tm* ltm = localtime(&now);
    char ts[32] = {0};
    if (ltm) strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", ltm);
    const char* rcStr;
    char rcBuf[32];
    switch (rc) {
        case SQL_SUCCESS:           rcStr = "SQL_SUCCESS"; break;
        case SQL_SUCCESS_WITH_INFO: rcStr = "SQL_SUCCESS_WITH_INFO"; break;
        case SQL_ERROR:             rcStr = "SQL_ERROR"; break;
        case SQL_NO_DATA:           rcStr = "SQL_NO_DATA"; break;
        case SQL_NEED_DATA:         rcStr = "SQL_NEED_DATA"; break;
        default: snprintf(rcBuf, sizeof(rcBuf), "%d", (int)rc); rcStr = rcBuf; break;
    }
    fprintf(f, "[%s] %s: %s (%s)\n", ts, func, msg, rcStr);
    fflush(f);
    fclose(f);
}

// FuncLogger is now a no-op shell — no entry/exit logging.
// It only provides the .ret() method so existing code compiles unchanged.
struct FuncLogger {
    const char* func;
    SQLINTEGER rc;
    FuncLogger(const char* f) : func(f), rc(0) {}
    ~FuncLogger() {}
    SQLRETURN ret(SQLRETURN r) { rc = (SQLINTEGER)(r); return r; }
};

SapConnection* getConnectionHandle(SQLHANDLE h) {
    LockGuard guard(&g_handleLock);
    auto it = g_connections.find(h);
    return (it != g_connections.end()) ? it->second : nullptr;
}

SapStatement* getStatementHandle(SQLHANDLE h) {
    LockGuard guard(&g_handleLock);
    auto it = g_statements.find(h);
    return (it != g_statements.end()) ? it->second : nullptr;
}

// Error storage — store last error per handle
static std::map<SQLHANDLE, SapErrorInfo> g_errors;

void setError(SQLHANDLE handle, SQLSMALLINT handleType, const std::string& msg, const char* state) {
    SapErrorInfo ei;
    ei.sqlstate = state;
    ei.message = msg;
    ei.native_error = 1;
    ei.has_error = true;
    LockGuard guard(&g_handleLock);
    g_errors[handle] = ei;
}

void clearError(SQLHANDLE handle, SQLSMALLINT handleType) {
    LockGuard guard(&g_handleLock);
    g_errors.erase(handle);
}

// ============================================================
// ODBC Driver Entry Point
// ============================================================

// DLL module handle — saved in DllMain for DialogBoxParam
static HINSTANCE g_hInstance = NULL;

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInstance = (HINSTANCE)hModule;
            InitializeCriticalSection(&g_handleLock);
            InitializeCriticalSection(&g_logLock);
            InitializeCriticalSection(&g_tableLogLock);
            RfcInit();
            // DllMain logging is always active (not gated by LogEnable)
            // because the DSN isn't known yet at attach time.
            // We use a direct file write instead of odbcLog() to avoid
            // the registry check.
            {
                FILE* f = fopen("C:\\Scripts\\SAP_ODBC\\sap_odbc_debug.log", "a");
                if (f) { fprintf(f, "[DllMain] DLL_PROCESS_ATTACH - DLL loaded\n"); fflush(f); fclose(f); }
            }
            break;
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&g_handleLock);
            DeleteCriticalSection(&g_logLock);
            DeleteCriticalSection(&g_tableLogLock);
            {
                FILE* f = fopen("C:\\Scripts\\SAP_ODBC\\sap_odbc_debug.log", "a");
                if (f) { fprintf(f, "[DllMain] DLL_PROCESS_DETACH - DLL unloaded\n"); fflush(f); fclose(f); }
            }
            break;
    }
    return TRUE;
}

// ============================================================
// Environment Handle
// ============================================================

SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE* OutputHandle) {
    FuncLogger logger("SQLAllocHandle");
    if (!OutputHandle) {
        setError(InputHandle, SQL_HANDLE_ENV, "SQLAllocHandle: OutputHandle pointer is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }
    switch (HandleType) {
        case SQL_HANDLE_ENV:
            *OutputHandle = (SQLHANDLE)1;  // dummy env handle
            return SQL_SUCCESS;

        case SQL_HANDLE_DBC: {
            SapConnection* conn = new SapConnection();
            conn->connected = false;
            conn->rfc_conn = NULL;
            conn->params.max_rows = MAX_ROWS_DEFAULT;
            conn->params.lang = "EN";
            SQLHANDLE h = (SQLHANDLE)conn;
            { LockGuard guard(&g_handleLock); g_connections[h] = conn; }
            *OutputHandle = h;
            return SQL_SUCCESS;
        }

        case SQL_HANDLE_STMT: {
            SapConnection* conn = getConnectionHandle(InputHandle);
            if (!conn) {
                setError(InputHandle, SQL_HANDLE_DBC, "SQLAllocHandle: Invalid connection handle for statement allocation", "HY000");
                return logger.ret(SQL_ERROR);
            }
            SapStatement* stmt = new SapStatement();
            stmt->connection = conn;
            stmt->current_row = 0;
            stmt->row_count = 0;
            stmt->executed = false;
            stmt->metadata_mode = "";
            SQLHANDLE h = (SQLHANDLE)stmt;
            { LockGuard guard(&g_handleLock); g_statements[h] = stmt; }
            *OutputHandle = h;
            return SQL_SUCCESS;
        }

        default:
            setError(InputHandle, SQL_HANDLE_ENV, "SQLAllocHandle: Unsupported handle type", "HY000");
            return logger.ret(SQL_ERROR);
    }
}

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle) {
    FuncLogger logger("SQLFreeHandle");
    switch (HandleType) {
        case SQL_HANDLE_ENV:
            return SQL_SUCCESS;

        case SQL_HANDLE_DBC: {
            SapConnection* conn = getConnectionHandle(Handle);
            if (conn) {
                rfcDisconnect(conn);
                { LockGuard guard(&g_handleLock); g_connections.erase(Handle); }
                clearError(Handle, SQL_HANDLE_DBC);
                delete conn;
            }
            return SQL_SUCCESS;
        }

        case SQL_HANDLE_STMT: {
            SapStatement* stmt = getStatementHandle(Handle);
            if (stmt) {
                { LockGuard guard(&g_handleLock); g_statements.erase(Handle); }
                clearError(Handle, SQL_HANDLE_STMT);
                delete stmt;
            }
            return SQL_SUCCESS;
        }

        default:
            setError(Handle, SQL_HANDLE_STMT, "SQLFreeHandle: Unsupported handle type", "HY000");
            return logger.ret(SQL_ERROR);
    }
}

// ============================================================
// Connection
// ============================================================

// Helper: convert a potentially SQL_NTS-terminated SQLCHAR* to std::string
static std::string sqlCharToString(SQLCHAR* sz, SQLSMALLINT cb) {
    if (!sz) return "";
    if (cb == SQL_NTS || cb < 0) return std::string((char*)sz);
    return std::string((char*)sz, cb);
}

SQLRETURN SQL_API SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd,
    SQLCHAR* szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLCHAR* szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
    FuncLogger logger("SQLDriverConnect");

    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);
    if (!conn) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLDriverConnect: Invalid connection handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    // Handle SQL_NTS (-3): use strlen instead of the length parameter
    std::string connstr = sqlCharToString(szConnStrIn, cbConnStrIn);
    conn->params = parseConnectionString(connstr);

    // Enable registry-gated logging if DSN is specified in the connection string
    if (!conn->params.dsn_name.empty()) {
        setLogDsn(conn->params.dsn_name);
        setTableLogPath(conn->params.dsn_name);
    }
    // Log SQLDriverConnect entry (only if logging is enabled)
    odbcLog("SQLDriverConnect", "entry", 0);

    // fDriverCompletion handling:
    //   SQL_DRIVER_PROMPT   — always prompt (we don't have a dialog here, so proceed)
    //   SQL_DRIVER_COMPLETE — prompt only if not enough info (we have connstr, proceed)
    //   SQL_DRIVER_NOPROMPT — never prompt, proceed
    // In all cases, attempt connection with the provided connection string.

    if (!rfcConnect(conn)) {
        return logger.ret(returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, conn->error_msg));
    }

    // Copy the connection string to the output buffer (echo back what we used)
    if (szConnStrOut && cbConnStrOutMax > 0) {
        strncpy_s((char*)szConnStrOut, cbConnStrOutMax, connstr.c_str(), cbConnStrOutMax - 1);
    }
    if (pcbConnStrOut) *pcbConnStrOut = (SQLSMALLINT)connstr.length();
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLConnect(SQLHDBC hdbc, SQLCHAR* szDSN, SQLSMALLINT cbDSN,
    SQLCHAR* szUID, SQLSMALLINT cbUID, SQLCHAR* szAuthStr, SQLSMALLINT cbAuthStr) {
    FuncLogger logger("SQLConnect");

    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);
    if (!conn) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLConnect: Invalid connection handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    // Get DSN name
    std::string dsn = sqlCharToString(szDSN, cbDSN);
    if (dsn.empty()) {
        return logger.ret(returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLConnect: DSN name is required"));
    }
    conn->params.dsn_name = dsn;

    // Enable registry-gated logging for this DSN
    setLogDsn(dsn);
    setTableLogPath(dsn);

    // Read DSN parameters from registry: HKLM\SOFTWARE\ODBC\ODBC.INI\<DSN>
    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + dsn;
    HKEY hKey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(),
                            0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) {
        return logger.ret(returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC,
                              "SQLConnect: DSN '" + dsn + "' not found in registry"));
    }

    conn->params.host    = regReadString(hKey, "Host");
    conn->params.sysnr   = regReadString(hKey, "SysNr");
    conn->params.client  = regReadString(hKey, "Client");
    conn->params.user     = regReadString(hKey, "User");
    conn->params.password = regReadString(hKey, "Password");
    conn->params.lang     = regReadString(hKey, "Lang");
    if (conn->params.lang.empty()) conn->params.lang = "EN";

    std::string maxRowsStr = regReadString(hKey, "MaxRows");
    conn->params.max_rows = maxRowsStr.empty() ? MAX_ROWS_DEFAULT : atoi(maxRowsStr.c_str());

    RegCloseKey(hKey);

    // Override User/Password if szUID/szAuthStr are provided
    std::string uid = sqlCharToString(szUID, cbUID);
    if (!uid.empty()) conn->params.user = uid;

    std::string auth = sqlCharToString(szAuthStr, cbAuthStr);
    if (!auth.empty()) conn->params.password = auth;

    if (conn->params.host.empty()) {
        return logger.ret(returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC,
                              "SQLConnect: No Host configured for DSN '" + dsn + "'"));
    }

    if (!rfcConnect(conn)) {
        return logger.ret(returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, conn->error_msg));
    }

    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLDisconnect(SQLHDBC hdbc) {
    FuncLogger logger("SQLDisconnect");
    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);
    if (!conn) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLDisconnect: Invalid connection handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    rfcDisconnect(conn);
    // rfcDisconnect already sets connected=false and rfc_conn=NULL
    return SQL_SUCCESS;
}

// ============================================================
// Statement Execution
// ============================================================

extern "C" SQLRETURN SQL_API SQLExecDirect(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    FuncLogger logger("SQLExecDirect");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLExecDirect: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    // Check that the statement has a valid, connected connection
    if (!stmt->connection || !stmt->connection->connected) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLExecDirect: Connection is not established", "08002");
        return logger.ret(SQL_ERROR);
    }

    // Get SQL text
    if (!szSqlStr) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLExecDirect: SQL string is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }

    if (cbSqlStr == SQL_NTS) {
        stmt->sql = std::string((char*)szSqlStr);
    } else if (cbSqlStr > 0) {
        stmt->sql = std::string((char*)szSqlStr, cbSqlStr);
    } else {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLExecDirect: Invalid SQL string length", "HY090");
        return logger.ret(SQL_ERROR);
    }

    // Log the SQL text (only if logging is enabled via registry)
    odbcLog("SQLExecDirect", (std::string("SQL: ") + stmt->sql).c_str(), 0);

    // Check if this is a simple table read (no JOIN/GROUP BY/etc.)
    std::string table, whereClause, fields;
    bool isSimple = isSimpleTableRead(stmt->sql, table, whereClause, fields);
    if (isSimple) {
        // Use chunked Z_READ_TABLE for flat table reads
        std::string error;
        std::string orderBy = "";
        int chunkSize = 10000;

        if (!rfcReadTableChunked(stmt->connection, table, whereClause, fields, orderBy,
                                 chunkSize, stmt->columns, stmt->rows, stmt->row_count, error)) {
            stmt->executed = false;
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
            odbcLog("SQLExecDirect", (std::string("ERROR: rfcReadTableChunked failed: ") + error).c_str(), SQL_ERROR);
            return logger.ret(SQL_ERROR);
        }

        stmt->current_row = 0;
        stmt->executed = true;
        stmt->metadata_mode = "";
        // Log the queried table name (if TableLogPath is set in registry)
        logQueriedTable(table);
        return SQL_SUCCESS;
    }

    // Complex query — use Z_EXECUTE_SQL (ADBC, server-side joins)
    std::string error;
    if (!rfcExecuteSql(stmt->connection, stmt->sql.c_str(),
                       stmt->columns, stmt->rows, stmt->row_count, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        odbcLog("SQLExecDirect", (std::string("ERROR: rfcExecuteSql failed: ") + error).c_str(), SQL_ERROR);
        return logger.ret(SQL_ERROR);
    }

    stmt->current_row = 0;
    stmt->executed = true;
    stmt->metadata_mode = "";
    return SQL_SUCCESS;
}

extern "C" SQLRETURN SQL_API SQLPrepare(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    FuncLogger logger("SQLPrepare");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLPrepare: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    if (!stmt->connection || !stmt->connection->connected) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLPrepare: Connection is not established", "08002");
        return logger.ret(SQL_ERROR);
    }

    if (!szSqlStr) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLPrepare: SQL string is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }

    if (cbSqlStr == SQL_NTS) {
        stmt->sql = std::string((char*)szSqlStr);
    } else if (cbSqlStr > 0) {
        stmt->sql = std::string((char*)szSqlStr, cbSqlStr);
    } else {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLPrepare: Invalid SQL string length", "HY090");
        return logger.ret(SQL_ERROR);
    }

    // Reset statement state for new prepared SQL
    stmt->executed = false;
    stmt->current_row = 0;
    stmt->metadata_mode = "";
    stmt->columns.clear();
    stmt->rows.clear();
    stmt->meta_columns.clear();
    stmt->meta_rows.clear();

    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLExecute(SQLHSTMT hstmt) {
    FuncLogger logger("SQLExecute");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLExecute: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    if (!stmt->connection || !stmt->connection->connected) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLExecute: Connection is not established", "08002");
        return logger.ret(SQL_ERROR);
    }

    // I-10 fix: SQLExecute should use isSimpleTableRead optimization, same as SQLExecDirect
    std::string table, whereClause, fields;
    if (isSimpleTableRead(stmt->sql, table, whereClause, fields)) {
        // Use chunked Z_READ_TABLE for flat table reads
        std::string error;
        std::string orderBy = "";
        int chunkSize = 10000;
        if (!rfcReadTableChunked(stmt->connection, table, whereClause, fields, orderBy,
                                 chunkSize, stmt->columns, stmt->rows, stmt->row_count, error)) {
            stmt->executed = false;
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
            return logger.ret(SQL_ERROR);
        }
        stmt->current_row = 0;
        stmt->executed = true;
        stmt->metadata_mode = "";
        return SQL_SUCCESS;
    }

    // Complex query — use Z_EXECUTE_SQL (ADBC, server-side joins)
    std::string error;
    if (!rfcExecuteSql(stmt->connection, stmt->sql.c_str(),
                       stmt->columns, stmt->rows, stmt->row_count, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return logger.ret(SQL_ERROR);
    }

    stmt->current_row = 0;
    stmt->executed = true;
    stmt->metadata_mode = "";
    return SQL_SUCCESS;
}

// ============================================================
// Result Set — Fetching
// ============================================================

// Split pipe-delimited row into fields
// Each field is also trimmed of trailing whitespace (from SAP CHAR field padding)
static std::vector<std::string> splitRow(const std::string& row, int num_cols) {
    std::vector<std::string> fields;
    fields.reserve(num_cols);
    std::string current;
    int col = 0;
    for (size_t i = 0; i < row.length() && col < num_cols; i++) {
        if (row[i] == '|') {
            // Trim trailing whitespace from this field
            while (!current.empty() && (current.back() == ' ' || current.back() == '\t' || current.back() == '\r' || current.back() == '\n')) {
                current.pop_back();
            }
            fields.push_back(current);
            current.clear();
            col++;
        } else {
            current += row[i];
        }
    }
    // Last field — trim trailing whitespace
    while (!current.empty() && (current.back() == ' ' || current.back() == '\t' || current.back() == '\r' || current.back() == '\n')) {
        current.pop_back();
    }
    if (col < num_cols) {
        fields.push_back(current);
    }
    // Pad if needed
    while ((int)fields.size() < num_cols) {
        fields.push_back("");
    }
    return fields;
}

// Fill bound column buffers after a row has been fetched (SQLBindCol support)
static void fillBoundColumns(SapStatement* stmt) {
    if (stmt->bindings.empty()) return;
    int row_idx = stmt->current_row - 1;
    if (row_idx < 0) return;

    for (auto& b : stmt->bindings) {
        if (!b.rgbValue) continue;

        std::string value;
        // Determine the value for this column at the current row
        if (!stmt->metadata_mode.empty()) {
            // Metadata mode: meta_rows is a vector of vector<string>
            if (row_idx < (int)stmt->meta_rows.size() &&
                b.col >= 1 && b.col <= (SQLUSMALLINT)stmt->meta_rows[row_idx].size()) {
                value = stmt->meta_rows[row_idx][b.col - 1];
            }
        } else {
            // Normal query mode: rows are pipe-delimited, split them
            if (row_idx < (int)stmt->rows.size() &&
                b.col >= 1 && b.col <= (SQLUSMALLINT)stmt->columns.size()) {
                std::vector<std::string> fields = splitRow(stmt->rows[row_idx], (int)stmt->columns.size());
                value = fields[b.col - 1];
            }
        }

        // Copy value to bound buffer based on C type
        if (b.fCType == SQL_C_CHAR || b.fCType == SQL_C_DEFAULT) {
            if (b.pcbValue) *b.pcbValue = (SQLLEN)value.length();
            if (b.cbValueMax > 0 && b.rgbValue) {
                strncpy_s((char*)b.rgbValue, b.cbValueMax, value.c_str(), b.cbValueMax - 1);
            }
        } else if (b.fCType == SQL_C_WCHAR) {
            // QlikView binds columns with SQL_C_WCHAR — convert ANSI to UTF-16
            int maxChars = (int)(b.cbValueMax / sizeof(SQLWCHAR));
            // Query required wide chars (includes null terminator)
            int wideLen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)value.c_str(), -1, NULL, 0);
            if (b.rgbValue && b.cbValueMax > 0) {
                if (wideLen > maxChars) {
                    // Truncation: copy what fits, null-terminate
                    if (maxChars > 0) {
                        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)value.c_str(), -1, (LPWSTR)b.rgbValue, maxChars);
                        ((SQLWCHAR*)b.rgbValue)[maxChars - 1] = 0;
                    }
                    if (b.pcbValue) *b.pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
                } else {
                    // Fits — copy whole string including null terminator
                    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)value.c_str(), -1, (LPWSTR)b.rgbValue, maxChars);
                    if (b.pcbValue) *b.pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
                }
            } else {
                // Caller just wants the length
                if (b.pcbValue) *b.pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
            }
        } else if (b.fCType == SQL_C_LONG || b.fCType == SQL_C_SLONG || b.fCType == SQL_C_ULONG) {
            long val = atol(value.c_str());
            if (b.rgbValue) *(long*)b.rgbValue = val;
            if (b.pcbValue) *b.pcbValue = sizeof(long);
        } else if (b.fCType == SQL_C_DOUBLE || b.fCType == SQL_C_FLOAT) {
            double val = atof(value.c_str());
            if (b.rgbValue) *(double*)b.rgbValue = val;
            if (b.pcbValue) *b.pcbValue = sizeof(double);
        } else if (b.fCType == SQL_C_SHORT || b.fCType == SQL_C_SSHORT || b.fCType == SQL_C_USHORT) {
            short val = (short)atoi(value.c_str());
            if (b.rgbValue) *(short*)b.rgbValue = val;
            if (b.pcbValue) *b.pcbValue = sizeof(short);
        } else if (b.fCType == SQL_C_TYPE_DATE) {
            // Parse "YYYY-MM-DD" into DATE_STRUCT
            DATE_STRUCT ds;
            memset(&ds, 0, sizeof(ds));
            if (value.length() >= 10) {
                ds.year  = (SQLSMALLINT)atoi(value.substr(0, 4).c_str());
                ds.month = (SQLUSMALLINT)atoi(value.substr(5, 2).c_str());
                ds.day   = (SQLUSMALLINT)atoi(value.substr(8, 2).c_str());
            }
            if (b.rgbValue) *(DATE_STRUCT*)b.rgbValue = ds;
            if (b.pcbValue) *b.pcbValue = sizeof(DATE_STRUCT);
        } else if (b.fCType == SQL_C_TYPE_TIME) {
            // Parse "HH:MM:SS" into TIME_STRUCT
            TIME_STRUCT ts;
            memset(&ts, 0, sizeof(ts));
            if (value.length() >= 8) {
                ts.hour   = (SQLUSMALLINT)atoi(value.substr(0, 2).c_str());
                ts.minute = (SQLUSMALLINT)atoi(value.substr(3, 2).c_str());
                ts.second = (SQLUSMALLINT)atoi(value.substr(6, 2).c_str());
            }
            if (b.rgbValue) *(TIME_STRUCT*)b.rgbValue = ts;
            if (b.pcbValue) *b.pcbValue = sizeof(TIME_STRUCT);
        } else {
            // Default: treat as string
            if (b.pcbValue) *b.pcbValue = (SQLLEN)value.length();
            if (b.cbValueMax > 0 && b.rgbValue) {
                strncpy_s((char*)b.rgbValue, b.cbValueMax, value.c_str(), b.cbValueMax - 1);
            }
        }
    }
}

SQLRETURN SQL_API SQLFetch(SQLHSTMT hstmt) {
    FuncLogger logger("SQLFetch");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLFetch: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLFetch: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }

    // Metadata mode: use meta_rows
    if (!stmt->metadata_mode.empty()) {
        if (stmt->current_row >= (int)stmt->meta_rows.size()) {
            return logger.ret(SQL_NO_DATA);
        }
        stmt->current_row++;
        // Fill bound column buffers
        fillBoundColumns(stmt);
        return SQL_SUCCESS;
    }

    // Normal query mode: use rows (pipe-delimited)
    if (stmt->current_row >= (int)stmt->rows.size()) {
        return logger.ret(SQL_NO_DATA);
    }

    stmt->current_row++;
    // Fill bound column buffers
    fillBoundColumns(stmt);
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetData(SQLHSTMT hstmt, SQLUSMALLINT icol,
    SQLSMALLINT fCType, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    FuncLogger logger("SQLGetData");

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }

    // Metadata mode: return pre-built string values from meta_rows
    if (!stmt->metadata_mode.empty()) {
        if (stmt->meta_rows.empty()) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: No metadata rows available", "HY000");
            return logger.ret(SQL_ERROR);
        }
        if (icol < 1 || icol > (SQLUSMALLINT)stmt->meta_rows[0].size()) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Column index out of range in metadata mode", "07009");
            return logger.ret(SQL_ERROR);
        }
        int row_idx = stmt->current_row - 1;
        if (row_idx < 0 || row_idx >= (int)stmt->meta_rows.size()) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Row index out of range in metadata mode", "HY109");
            return logger.ret(SQL_ERROR);
        }
        std::string value = stmt->meta_rows[row_idx][icol - 1];
        if (pcbValue) *pcbValue = (SQLLEN)value.length();
        if (cbValueMax > 0 && rgbValue) {
            strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
            // Check for truncation
            if ((SQLLEN)value.length() >= cbValueMax) {
                setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "Data truncated", "01004");
                return logger.ret(SQL_SUCCESS_WITH_INFO);
            }
        }
        return SQL_SUCCESS;
    }

    // Normal query mode
    if (stmt->columns.empty()) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: No columns in result set", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (icol < 1 || icol > (SQLUSMALLINT)stmt->columns.size()) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Column index out of range", "07009");
        return logger.ret(SQL_ERROR);
    }

    int row_idx = stmt->current_row - 1;
    if (row_idx < 0 || row_idx >= (int)stmt->rows.size()) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetData: Row index out of range", "HY109");
        return logger.ret(SQL_ERROR);
    }

    std::vector<std::string> fields = splitRow(stmt->rows[row_idx], (int)stmt->columns.size());
    std::string value = fields[icol - 1];

    // Return as string (SQL_C_CHAR) — simplest approach
    if (fCType == SQL_C_CHAR || fCType == SQL_C_DEFAULT) {
        if (pcbValue) *pcbValue = (SQLLEN)value.length();
        if (cbValueMax > 0 && rgbValue) {
            strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
            // Check for truncation
            if ((SQLLEN)value.length() >= cbValueMax) {
                setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "Data truncated", "01004");
                return logger.ret(SQL_SUCCESS_WITH_INFO);
            }
        }
        return SQL_SUCCESS;
    }

    // Return as integer
    if (fCType == SQL_C_LONG || fCType == SQL_C_SLONG || fCType == SQL_C_ULONG) {
        int val = atoi(value.c_str());
        if (rgbValue) *(int*)rgbValue = val;
        if (pcbValue) *pcbValue = sizeof(int);
        return SQL_SUCCESS;
    }

    // Return as double
    if (fCType == SQL_C_DOUBLE) {
        double val = atof(value.c_str());
        if (rgbValue) *(double*)rgbValue = val;
        if (pcbValue) *pcbValue = sizeof(double);
        return SQL_SUCCESS;
    }

    // Return as DATE_STRUCT (SQL_C_TYPE_DATE) — parse "YYYY-MM-DD"
    if (fCType == SQL_C_TYPE_DATE) {
        DATE_STRUCT ds;
        memset(&ds, 0, sizeof(ds));
        if (value.length() >= 10) {
            ds.year  = (SQLSMALLINT)atoi(value.substr(0, 4).c_str());
            ds.month = (SQLUSMALLINT)atoi(value.substr(5, 2).c_str());
            ds.day   = (SQLUSMALLINT)atoi(value.substr(8, 2).c_str());
        }
        if (rgbValue) *(DATE_STRUCT*)rgbValue = ds;
        if (pcbValue) *pcbValue = sizeof(DATE_STRUCT);
        return SQL_SUCCESS;
    }

    // Return as TIME_STRUCT (SQL_C_TYPE_TIME) — parse "HH:MM:SS"
    if (fCType == SQL_C_TYPE_TIME) {
        TIME_STRUCT ts;
        memset(&ts, 0, sizeof(ts));
        if (value.length() >= 8) {
            ts.hour   = (SQLUSMALLINT)atoi(value.substr(0, 2).c_str());
            ts.minute = (SQLUSMALLINT)atoi(value.substr(3, 2).c_str());
            ts.second = (SQLUSMALLINT)atoi(value.substr(6, 2).c_str());
        }
        if (rgbValue) *(TIME_STRUCT*)rgbValue = ts;
        if (pcbValue) *pcbValue = sizeof(TIME_STRUCT);
        return SQL_SUCCESS;
    }

    // Default: return as string
    if (pcbValue) *pcbValue = (SQLLEN)value.length();
    if (cbValueMax > 0 && rgbValue) {
        strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
        // Check for truncation
        if ((SQLLEN)value.length() >= cbValueMax) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "Data truncated", "01004");
            return logger.ret(SQL_SUCCESS_WITH_INFO);
        }
    }
    return SQL_SUCCESS;
}

// ============================================================
// Result Set — Metadata
// ============================================================

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT hstmt, SQLSMALLINT* pccol) {
    FuncLogger logger("SQLNumResultCols");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLNumResultCols: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLNumResultCols: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }
    if (!pccol) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLNumResultCols: pccol is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->metadata_mode.empty()) {
        *pccol = (SQLSMALLINT)stmt->meta_columns.size();
    } else {
        *pccol = (SQLSMALLINT)stmt->columns.size();
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT icol,
    SQLCHAR* szColName, SQLSMALLINT cbColNameMax, SQLSMALLINT* pcbColName,
    SQLSMALLINT* pfSqlType, SQLULEN* pcbColDef,
    SQLSMALLINT* pibScale, SQLSMALLINT* pfNullable) {
    FuncLogger logger("SQLDescribeCol");

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLDescribeCol: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLDescribeCol: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }

    // Metadata mode
    if (!stmt->metadata_mode.empty()) {
        if (icol < 1 || icol > (SQLUSMALLINT)stmt->meta_columns.size()) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLDescribeCol: Column index out of range in metadata mode", "07009");
            return logger.ret(SQL_ERROR);
        }
        ColumnMeta& col = stmt->meta_columns[icol - 1];
        if (szColName && cbColNameMax > 0) {
            strncpy_s((char*)szColName, cbColNameMax, col.fieldname, cbColNameMax - 1);
        }
        if (pcbColName) *pcbColName = (SQLSMALLINT)strlen(col.fieldname);
        if (pfSqlType) *pfSqlType = col.sql_type;
        if (pcbColDef) *pcbColDef = col.length;
        if (pibScale) *pibScale = (SQLSMALLINT)col.decimals;
        if (pfNullable) *pfNullable = SQL_NULLABLE_UNKNOWN;
        return SQL_SUCCESS;
    }

    // Normal query mode
    if (icol < 1 || icol > (SQLUSMALLINT)stmt->columns.size()) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLDescribeCol: Column index out of range", "07009");
        return logger.ret(SQL_ERROR);
    }

    ColumnMeta& col = stmt->columns[icol - 1];

    if (szColName && cbColNameMax > 0) {
        strncpy_s((char*)szColName, cbColNameMax, col.fieldname, cbColNameMax - 1);
    }
    if (pcbColName) *pcbColName = (SQLSMALLINT)strlen(col.fieldname);
    if (pfSqlType) *pfSqlType = col.sql_type;
    if (pcbColDef) *pcbColDef = col.length;
    if (pibScale) *pibScale = (SQLSMALLINT)col.decimals;
    if (pfNullable) *pfNullable = SQL_NULLABLE_UNKNOWN;

    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLColAttribute(SQLHSTMT hstmt, SQLUSMALLINT iCol,
    SQLUSMALLINT iFieldIdentifier, SQLPOINTER pCharAttr, SQLSMALLINT cbCharAttrMax,
    SQLSMALLINT* pcbCharAttr, SQLLEN* pNumAttr) {
    FuncLogger logger("SQLColAttribute");

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColAttribute: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColAttribute: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }

    // Handle iCol==0 — return bookmark/descriptor-count info per ODBC spec
    if (iCol == 0) {
        if (iFieldIdentifier == SQL_DESC_COUNT) {
            if (pNumAttr) *pNumAttr = (SQLLEN)(!stmt->metadata_mode.empty() ? stmt->meta_columns.size() : stmt->columns.size());
            return SQL_SUCCESS;
        }
        // For other attributes with col 0, return 0/empty
        if (pNumAttr) *pNumAttr = 0;
        if (pCharAttr && cbCharAttrMax > 0) ((char*)pCharAttr)[0] = '\0';
        if (pcbCharAttr) *pcbCharAttr = 0;
        return SQL_SUCCESS;
    }

    // Metadata mode
    if (!stmt->metadata_mode.empty()) {
        if (iCol < 1 || iCol > (SQLUSMALLINT)stmt->meta_columns.size()) {
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColAttribute: Column index out of range in metadata mode", "07009");
            return logger.ret(SQL_ERROR);
        }
        ColumnMeta& col = stmt->meta_columns[iCol - 1];
        switch (iFieldIdentifier) {
            case SQL_DESC_NAME:
            case SQL_DESC_LABEL:
                if (pCharAttr && cbCharAttrMax > 0)
                    strncpy_s((char*)pCharAttr, cbCharAttrMax, col.fieldname, cbCharAttrMax - 1);
                if (pcbCharAttr) *pcbCharAttr = (SQLSMALLINT)strlen(col.fieldname);
                break;
            case SQL_DESC_TYPE:
            case SQL_DESC_CONCISE_TYPE:
                if (pNumAttr) *pNumAttr = col.sql_type;
                break;
            case SQL_DESC_LENGTH:
            case SQL_DESC_DISPLAY_SIZE:
                if (pNumAttr) *pNumAttr = col.length;
                break;
            case SQL_DESC_NULLABLE:
                if (pNumAttr) *pNumAttr = SQL_NULLABLE_UNKNOWN;
                break;
            case SQL_DESC_COUNT:
                if (pNumAttr) *pNumAttr = (SQLLEN)stmt->meta_columns.size();
                break;
            default:
                if (pNumAttr) *pNumAttr = 0;
                break;
        }
        return SQL_SUCCESS;
    }

    // Normal query mode
    if (iCol < 1 || iCol > (SQLUSMALLINT)stmt->columns.size()) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColAttribute: Column index out of range", "07009");
        return logger.ret(SQL_ERROR);
    }

    ColumnMeta& col = stmt->columns[iCol - 1];

    switch (iFieldIdentifier) {
        case SQL_DESC_NAME:
        case SQL_DESC_LABEL:
            if (pCharAttr && cbCharAttrMax > 0) {
                strncpy_s((char*)pCharAttr, cbCharAttrMax, col.fieldname, cbCharAttrMax - 1);
            }
            if (pcbCharAttr) *pcbCharAttr = (SQLSMALLINT)strlen(col.fieldname);
            break;
        case SQL_DESC_TYPE:
        case SQL_DESC_CONCISE_TYPE:
            if (pNumAttr) *pNumAttr = col.sql_type;
            break;
        case SQL_DESC_LENGTH:
            if (pNumAttr) *pNumAttr = col.length;
            break;
        case SQL_DESC_DISPLAY_SIZE:
            if (pNumAttr) *pNumAttr = col.length;
            break;
        case SQL_DESC_NULLABLE:
            if (pNumAttr) *pNumAttr = SQL_NULLABLE_UNKNOWN;
            break;
        case SQL_DESC_COUNT:
            if (pNumAttr) *pNumAttr = (SQLLEN)stmt->columns.size();
            break;
        default:
            if (pNumAttr) *pNumAttr = 0;
            break;
    }
    return SQL_SUCCESS;
}

// ============================================================
// Stubs for functions we don't fully implement
// ============================================================

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType) {
    return SQL_SUCCESS;  // No transaction support
}

SQLRETURN SQL_API SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength) {
    FuncLogger logger("SQLSetConnectAttr");
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength) {
    FuncLogger logger("SQLSetStmtAttr");
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV henv, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength) {
    FuncLogger logger("SQLSetEnvAttr");
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT DataType) {
    FuncLogger logger("SQLGetTypeInfo");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetTypeInfo: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }

    // Clear any previous result state
    stmt->metadata_mode = "TYPEINFO";
    stmt->current_row = 0;

    // Build type info columns (JDBC standard):
    // TYPE_NAME, DATA_TYPE, COLUMN_SIZE, LITERAL_PREFIX, LITERAL_SUFFIX,
    // CREATE_PARAMS, NULLABLE, CASE_SENSITIVE, SEARCHABLE, UNSIGNED_ATTRIBUTE,
    // FIXED_PREC_SCALE, AUTO_UNIQUE_VALUE, LOCAL_TYPE_NAME, MINIMUM_SCALE,
    // MAXIMUM_SCALE, SQL_DATA_TYPE, SQL_DATETIME_SUB, NUM_PREC_RADIX, INTERVAL_PRECISION
    stmt->meta_columns.clear();
    ColumnMeta c;
    auto addCol = [&](const char* name, SQLSMALLINT type, int len) {
        memset(&c, 0, sizeof(c));
        strncpy_s(c.fieldname, sizeof(c.fieldname), name, sizeof(c.fieldname) - 1);
        c.sql_type = type; c.length = len;
        stmt->meta_columns.push_back(c);
    };
    addCol("TYPE_NAME", SQL_VARCHAR, 128);
    addCol("DATA_TYPE", SQL_SMALLINT, 5);
    addCol("COLUMN_SIZE", SQL_INTEGER, 10);
    addCol("LITERAL_PREFIX", SQL_VARCHAR, 1);
    addCol("LITERAL_SUFFIX", SQL_VARCHAR, 1);
    addCol("CREATE_PARAMS", SQL_VARCHAR, 128);
    addCol("NULLABLE", SQL_SMALLINT, 5);
    addCol("CASE_SENSITIVE", SQL_SMALLINT, 5);
    addCol("SEARCHABLE", SQL_SMALLINT, 5);
    addCol("UNSIGNED_ATTRIBUTE", SQL_SMALLINT, 5);
    addCol("FIXED_PREC_SCALE", SQL_SMALLINT, 5);
    addCol("AUTO_UNIQUE_VALUE", SQL_SMALLINT, 5);
    addCol("LOCAL_TYPE_NAME", SQL_VARCHAR, 128);
    addCol("MINIMUM_SCALE", SQL_SMALLINT, 5);
    addCol("MAXIMUM_SCALE", SQL_SMALLINT, 5);
    addCol("SQL_DATA_TYPE", SQL_SMALLINT, 5);
    addCol("SQL_DATETIME_SUB", SQL_SMALLINT, 5);
    addCol("NUM_PREC_RADIX", SQL_INTEGER, 10);
    // C-2 fix: add the 19th column INTERVAL_PRECISION
    addCol("INTERVAL_PRECISION", SQL_SMALLINT, 5);

    // Type info rows
    auto addType = [&](const char* name, int dt, int size, const char* prefix,
                       const char* suffix, const char* params, int nullable,
                       int case_sens, int searchable, int unsigned_attr,
                       int fixed_prec, int auto_unique) {
        std::vector<std::string> row;
        row.push_back(name);
        row.push_back(std::to_string(dt));
        row.push_back(std::to_string(size));
        row.push_back(prefix);
        row.push_back(suffix);
        row.push_back(params);
        row.push_back(std::to_string(nullable));
        row.push_back(std::to_string(case_sens));
        row.push_back(std::to_string(searchable));
        row.push_back(std::to_string(unsigned_attr));
        row.push_back(std::to_string(fixed_prec));
        row.push_back(std::to_string(auto_unique));
        row.push_back(name);
        row.push_back("0");
        row.push_back("0");
        row.push_back(std::to_string(dt));
        row.push_back("");
        row.push_back("10");
        // C-2 fix: 19th value for INTERVAL_PRECISION
        row.push_back("");
        stmt->meta_rows.push_back(row);
    };

    // Filter by DataType if specified
    if (DataType == SQL_ALL_TYPES || DataType == 0) {
        addType("VARCHAR", SQL_VARCHAR, 8000, "'", "'", "length", 1, 1, 3, 1, 0, 0);
        addType("INTEGER", SQL_INTEGER, 10, "", "", "", 1, 0, 3, 0, 0, 0);
        addType("SMALLINT", SQL_SMALLINT, 5, "", "", "", 1, 0, 3, 0, 0, 0);
        addType("DOUBLE", SQL_DOUBLE, 15, "", "", "", 1, 0, 3, 0, 0, 0);
        addType("DECIMAL", SQL_DECIMAL, 38, "", "", "precision,scale", 1, 0, 3, 0, 1, 0);
        addType("DATE", SQL_TYPE_DATE, 10, "'", "'", "", 1, 0, 3, 1, 0, 0);
        addType("TIME", SQL_TYPE_TIME, 8, "'", "'", "", 1, 0, 3, 1, 0, 0);
        addType("VARBINARY", SQL_VARBINARY, 8000, "0x", "", "length", 1, 0, 3, 1, 0, 0);
    } else {
        switch (DataType) {
            case SQL_VARCHAR: addType("VARCHAR", SQL_VARCHAR, 8000, "'", "'", "length", 1, 1, 3, 1, 0, 0); break;
            case SQL_INTEGER: addType("INTEGER", SQL_INTEGER, 10, "", "", "", 1, 0, 3, 0, 0, 0); break;
            case SQL_SMALLINT: addType("SMALLINT", SQL_SMALLINT, 5, "", "", "", 1, 0, 3, 0, 0, 0); break;
            case SQL_DOUBLE: addType("DOUBLE", SQL_DOUBLE, 15, "", "", "", 1, 0, 3, 0, 0, 0); break;
            case SQL_DECIMAL: addType("DECIMAL", SQL_DECIMAL, 38, "", "", "precision,scale", 1, 0, 3, 0, 1, 0); break;
            case SQL_TYPE_DATE: addType("DATE", SQL_TYPE_DATE, 10, "'", "'", "", 1, 0, 3, 1, 0, 0); break;
            case SQL_TYPE_TIME: addType("TIME", SQL_TYPE_TIME, 8, "'", "'", "", 1, 0, 3, 1, 0, 0); break;
            case SQL_VARBINARY: addType("VARBINARY", SQL_VARBINARY, 8000, "0x", "", "length", 1, 0, 3, 1, 0, 0); break;
        }
    }

    stmt->executed = true;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLTables(SQLHSTMT hstmt, SQLCHAR* szCatalogName, SQLSMALLINT cbCatalogName,
    SQLCHAR* szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR* szTableName, SQLSMALLINT cbTableName,
    SQLCHAR* szTableType, SQLSMALLINT cbTableType) {
    FuncLogger logger("SQLTables");

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLTables: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->connection || !stmt->connection->connected) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLTables: Connection is not established", "08002");
        return logger.ret(SQL_ERROR);
    }

    // Extract table name pattern
    std::string tablePattern = "%";
    if (szTableName) {
        // I-7 fix: handle negative cbTableName (treat as SQL_NTS)
        if (cbTableName == SQL_NTS || cbTableName < 0) tablePattern = std::string((char*)szTableName);
        else tablePattern = std::string((char*)szTableName, cbTableName);
    }

    // I-6 fix: check szTableType filter — if provided and doesn't contain "TABLE", return empty result
    if (szTableType) {
        std::string tableType;
        if (cbTableType == SQL_NTS || cbTableType < 0) tableType = std::string((char*)szTableType);
        else tableType = std::string((char*)szTableType, cbTableType);
        // Convert to uppercase for case-insensitive comparison
        std::string tableTypeUpper = tableType;
        for (auto& c : tableTypeUpper) c = (char)toupper((unsigned char)c);
        // If the filter is non-empty and doesn't contain "TABLE", return empty result set
        if (!tableTypeUpper.empty() && tableTypeUpper.find("TABLE") == std::string::npos) {
            stmt->metadata_mode = "TABLES";
            stmt->meta_columns.clear();
            stmt->meta_rows.clear();
            stmt->current_row = 0;
            stmt->executed = true;
            return SQL_SUCCESS;
        }
    }

    stmt->metadata_mode = "TABLES";
    stmt->meta_table_name = "";
    stmt->current_row = 0;

    std::string error;
    if (!metaGetTables(stmt->connection, tablePattern, stmt->meta_columns, stmt->meta_rows, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return logger.ret(SQL_ERROR);
    }

    stmt->executed = true;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLColumns(SQLHSTMT hstmt, SQLCHAR* szCatalogName, SQLSMALLINT cbCatalogName,
    SQLCHAR* szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR* szTableName, SQLSMALLINT cbTableName,
    SQLCHAR* szColumnName, SQLSMALLINT cbColumnName) {
    FuncLogger logger("SQLColumns");

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColumns: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->connection || !stmt->connection->connected) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColumns: Connection is not established", "08002");
        return logger.ret(SQL_ERROR);
    }

    // Extract table name (required for SQLColumns)
    std::string tableName;
    if (szTableName) {
        // I-8 fix: handle negative cbTableName (treat as SQL_NTS)
        if (cbTableName == SQL_NTS || cbTableName < 0) tableName = std::string((char*)szTableName);
        else tableName = std::string((char*)szTableName, cbTableName);
    }

    if (tableName.empty() || tableName == "%") {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColumns requires a specific table name");
        return logger.ret(SQL_ERROR);
    }

    stmt->metadata_mode = "COLUMNS";
    stmt->meta_table_name = tableName;
    stmt->current_row = 0;

    std::string error;
    if (!metaGetColumns(stmt->connection, tableName, stmt->meta_columns, stmt->meta_rows, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return logger.ret(SQL_ERROR);
    }

    stmt->executed = true;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT* pcbInfoValue) {
    FuncLogger logger("SQLGetInfo");

    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);

    // If hdbc is non-null but conn is NULL, the handle is invalid — set error but don't crash
    if (!conn && hdbc != SQL_NULL_HDBC) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLGetInfo: Invalid connection handle", "HY000");
    }

    // Helper lambda for string-type info values
    auto setStr = [&](const std::string& val) {
        if (rgbInfoValue && cbInfoValueMax > 0)
            strncpy_s((char*)rgbInfoValue, cbInfoValueMax, val.c_str(), cbInfoValueMax - 1);
        if (pcbInfoValue) *pcbInfoValue = (SQLSMALLINT)val.length();
    };
    // Helper lambda for SQLUSMALLINT (bitmask) info values
    auto setUshort = [&](SQLUSMALLINT val) {
        if (rgbInfoValue) *(SQLUSMALLINT*)rgbInfoValue = val;
        if (pcbInfoValue) *pcbInfoValue = sizeof(SQLUSMALLINT);
    };
    // Helper lambda for SQLUINTEGER info values
    auto setUint = [&](SQLUINTEGER val) {
        if (rgbInfoValue) *(SQLUINTEGER*)rgbInfoValue = val;
        if (pcbInfoValue) *pcbInfoValue = sizeof(SQLUINTEGER);
    };

    switch (fInfoType) {
        // --- String-type info values ---
        case SQL_DRIVER_NAME:
            setStr("sapodbcabap.dll");
            return SQL_SUCCESS;
        case SQL_DBMS_NAME:
            setStr("SAP via Z_EXECUTE_SQL");
            return SQL_SUCCESS;
        case SQL_DBMS_VER:
            setStr("1.00.0000");
            return SQL_SUCCESS;
        case SQL_DRIVER_ODBC_VER:
            setStr("03.80");
            return SQL_SUCCESS;
        case SQL_DRIVER_VER:
            setStr("01.00.0000");
            return SQL_SUCCESS;
        case SQL_SERVER_NAME:
            setStr(conn ? conn->params.host : "");
            return SQL_SUCCESS;
        case SQL_DATA_SOURCE_NAME:
            setStr(conn ? conn->params.dsn_name : "");
            return SQL_SUCCESS;
        case SQL_USER_NAME:
            setStr(conn ? conn->params.user : "");
            return SQL_SUCCESS;
        case SQL_CATALOG_NAME:
            setStr("");
            return SQL_SUCCESS;
        case SQL_TABLE_TERM:
            setStr("table");
            return SQL_SUCCESS;
        case SQL_CATALOG_TERM:
            setStr("catalog");
            return SQL_SUCCESS;
        case SQL_SCHEMA_TERM:
            setStr("schema");
            return SQL_SUCCESS;
        case SQL_IDENTIFIER_QUOTE_CHAR:
            setStr("\"");
            return SQL_SUCCESS;
        case SQL_SEARCH_PATTERN_ESCAPE:
            setStr("\\");
            return SQL_SUCCESS;
        case SQL_CATALOG_NAME_SEPARATOR:
            setStr(".");
            return SQL_SUCCESS;
        case SQL_QUALIFIER_LOCATION:
            setStr("");
            return SQL_SUCCESS;
        case SQL_COLUMN_ALIAS:
            setStr("Y");
            return SQL_SUCCESS;
        case SQL_ORDER_BY_COLUMNS_IN_SELECT:
            setStr("Y");
            return SQL_SUCCESS;

        // --- Bitmask/integer-type info values ---
        case SQL_CATALOG_USAGE:
            setUshort(0);  // No catalog support in SAP
            return SQL_SUCCESS;
        case SQL_SCHEMA_USAGE:
            setUshort(SQL_SU_TABLE_DEFINITION | SQL_SU_PROCEDURE_INVOCATION);
            return SQL_SUCCESS;
        case SQL_SUBQUERIES:
            setUshort(SQL_SQ_CORRELATED_SUBQUERIES | SQL_SQ_COMPARISON | SQL_SQ_EXISTS |
                     SQL_SQ_IN | SQL_SQ_QUANTIFIED);
            return SQL_SUCCESS;
        case SQL_UNION:
            setUshort(SQL_U_UNION | SQL_U_UNION_ALL);
            return SQL_SUCCESS;
        case SQL_GROUP_BY:
            setUshort(SQL_GB_GROUP_BY_CONTAINS_SELECT);
            return SQL_SUCCESS;
        case SQL_QUOTED_IDENTIFIER_CASE:
            setUshort(SQL_IC_SENSITIVE);
            return SQL_SUCCESS;
        case SQL_IDENTIFIER_CASE:
            setUshort(SQL_IC_MIXED);
            return SQL_SUCCESS;

        // --- SQLUINTEGER info values ---
        case SQL_MAXIMUM_CATALOG_NAME_LENGTH:
            setUint(128);
            return SQL_SUCCESS;
        case SQL_MAXIMUM_SCHEMA_NAME_LENGTH:
            setUint(128);
            return SQL_SUCCESS;
        case SQL_MAX_TABLE_NAME_LEN:
            setUint(128);
            return SQL_SUCCESS;
        case SQL_MAXIMUM_COLUMN_NAME_LENGTH:
            setUint(30);
            return SQL_SUCCESS;
        case SQL_MAX_COLUMNS_IN_TABLE:
            setUint(1024);
            return SQL_SUCCESS;
        case SQL_MAXIMUM_COLUMNS_IN_SELECT:
            setUint(1024);
            return SQL_SUCCESS;
        case SQL_MAXIMUM_IDENTIFIER_LENGTH:
            setUint(128);
            return SQL_SUCCESS;
        case SQL_MAXIMUM_STATEMENT_LENGTH:
            setUint(0);  // No fixed limit
            return SQL_SUCCESS;
        case SQL_CURSOR_COMMIT_BEHAVIOR:
            setUint(SQL_CB_CLOSE);
            return SQL_SUCCESS;
        case SQL_CURSOR_ROLLBACK_BEHAVIOR:
            setUint(SQL_CB_CLOSE);
            return SQL_SUCCESS;
        case SQL_DEFAULT_TXN_ISOLATION:
            setUint(SQL_TXN_READ_UNCOMMITTED);
            return SQL_SUCCESS;
        case SQL_TXN_ISOLATION_OPTION:
            setUint(SQL_TXN_READ_UNCOMMITTED);
            return SQL_SUCCESS;
        case SQL_CONCAT_NULL_BEHAVIOR:
            setUint(SQL_CB_NULL);
            return SQL_SUCCESS;
        case SQL_NULL_COLLATION:
            setUint(SQL_NC_START);
            return SQL_SUCCESS;
        case SQL_GETDATA_EXTENSIONS:
            setUint(SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BOUND);
            return SQL_SUCCESS;
        case SQL_SQL_CONFORMANCE:
            setUint(SQL_SC_SQL92_ENTRY);
            return SQL_SUCCESS;
        case SQL_OJ_CAPABILITIES:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_FILE_USAGE:
            setUint(SQL_FILE_NOT_SUPPORTED);
            return SQL_SUCCESS;
        case SQL_AGGREGATE_FUNCTIONS:
            setUint(SQL_AF_ALL | SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_DISTINCT |
                    SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM);
            return SQL_SUCCESS;
        case SQL_NUMERIC_FUNCTIONS:
            setUint(SQL_FN_NUM_ABS | SQL_FN_NUM_FLOOR |
                    SQL_FN_NUM_POWER | SQL_FN_NUM_ROUND | SQL_FN_NUM_SIGN |
                    SQL_FN_NUM_TRUNCATE);
            return SQL_SUCCESS;
        case SQL_STRING_FUNCTIONS:
            setUint(SQL_FN_STR_LENGTH | SQL_FN_STR_REPLACE | SQL_FN_STR_SUBSTRING |
                    SQL_FN_STR_LCASE | SQL_FN_STR_UCASE);
            return SQL_SUCCESS;
        case SQL_SYSTEM_FUNCTIONS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_TIMEDATE_FUNCTIONS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_TIMEDATE_ADD_INTERVALS:
        case SQL_TIMEDATE_DIFF_INTERVALS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_DATETIME_LITERALS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_CONVERT_FUNCTIONS:
            setUint(SQL_FN_CVT_CONVERT | SQL_FN_CVT_CAST);
            return SQL_SUCCESS;
        case SQL_CREATE_TABLE:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_DROP_TABLE:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_ALTER_TABLE:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_CREATE_VIEW:
        case SQL_DROP_VIEW:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_INSERT_STATEMENT:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_MAX_DRIVER_CONNECTIONS:
            setUint(1);
            return SQL_SUCCESS;
        case SQL_MAX_CONCURRENT_ACTIVITIES:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_ACCESSIBLE_PROCEDURES:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_ACCESSIBLE_TABLES:
            setStr("Y");
            return SQL_SUCCESS;
        case SQL_PROCEDURES:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_PROCEDURE_TERM:
            setStr("procedure");
            return SQL_SUCCESS;
        case SQL_OUTER_JOINS:
            setStr("Y");
            return SQL_SUCCESS;
        case SQL_MULTIPLE_ACTIVE_TXN:
            setStr("Y");
            return SQL_SUCCESS;
        case SQL_DATA_SOURCE_READ_ONLY:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_MULT_RESULT_SETS:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_NEED_LONG_DATA_LEN:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_BATCH_ROW_COUNT:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_BATCH_SUPPORT:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_ASYNC_MODE:
            setUint(SQL_AM_NONE);
            return SQL_SUCCESS;
        case SQL_MAX_ROW_SIZE:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_MAX_TABLES_IN_SELECT:
            setUint(32);
            return SQL_SUCCESS;
        case SQL_POS_OPERATIONS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_LOCK_TYPES:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_POSITIONED_STATEMENTS:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_STATIC_SENSITIVITY:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_ROW_UPDATES:
            setStr("N");
            return SQL_SUCCESS;
        case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
            setUint(SQL_CA1_ABSOLUTE | SQL_CA1_LOCK_NO_CHANGE);
            return SQL_SUCCESS;
        case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_SCROLL_CONCURRENCY:
            setUint(SQL_SCCO_READ_ONLY);
            return SQL_SUCCESS;
        case SQL_SCROLL_OPTIONS:
            setUint(SQL_SO_FORWARD_ONLY);
            return SQL_SUCCESS;
        case SQL_TXN_CAPABLE:
            setUint(SQL_TC_NONE);
            return SQL_SUCCESS;
        case SQL_STATIC_CURSOR_ATTRIBUTES1:
            setUint(SQL_CA1_ABSOLUTE | SQL_CA1_NEXT);
            return SQL_SUCCESS;
        case SQL_STATIC_CURSOR_ATTRIBUTES2:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_BOOKMARK_PERSISTENCE:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_KEYSET_CURSOR_ATTRIBUTES1:
        case SQL_KEYSET_CURSOR_ATTRIBUTES2:
        case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
        case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
            setUint(0);
            return SQL_SUCCESS;
        case SQL_SQL92_DATETIME_FUNCTIONS:
        case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
        case SQL_SQL92_PREDICATES:
        case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
        case SQL_SQL92_STRING_FUNCTIONS:
        case SQL_SQL92_VALUE_EXPRESSIONS:
            setUint(0);
            return SQL_SUCCESS;

        default:
            // For unknown info types, return SQL_SUCCESS with empty/zero value
            if (rgbInfoValue && cbInfoValueMax > 0) memset(rgbInfoValue, 0, cbInfoValueMax);
            if (pcbInfoValue) *pcbInfoValue = 0;
            return SQL_SUCCESS;
    }
}

SQLRETURN SQL_API SQLGetFunctions(SQLHDBC hdbc, SQLUSMALLINT fFunction, SQLUSMALLINT* pfSupported) {
    FuncLogger logger("SQLGetFunctions");
    if (!pfSupported) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLGetFunctions: pfSupported pointer is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }

    // Only report functions that are actually implemented in this driver.
    // Returning SQL_TRUE for unimplemented functions causes Excel/Power Query
    // to call them and crash (NULL pointer dereference / SafeHandle closed).

    // Handle SQL_API_ODBC3_ALL_FUNCTIONS (999) — returns a bitmap array
    // Excel/Power Query uses this to query all functions at once.
    // The bitmap is an array of 250 SQLUSMALLINT values, treated as a 4000-bit bitmap.
    // Each function ID maps to: bit (id & 0x0F) in word (id >> 4).
    if (fFunction == SQL_API_ODBC3_ALL_FUNCTIONS) {
        SQLUSMALLINT* arr = pfSupported;
        // Zero the entire array first
        memset(arr, 0, SQL_API_ODBC3_ALL_FUNCTIONS_SIZE * sizeof(SQLUSMALLINT));
        // Helper macro to SET a function bit in the bitmap
        #define SET_FUNC(fn) do { \
            int _w = (fn) >> 4; \
            int _b = (fn) & 0x0F; \
            if (_w < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE) \
                arr[_w] |= (1 << _b); \
        } while(0)
        SET_FUNC(SQL_API_SQLALLOCHANDLE);
        SET_FUNC(SQL_API_SQLFREEHANDLE);
        SET_FUNC(SQL_API_SQLDRIVERCONNECT);
        SET_FUNC(SQL_API_SQLCONNECT);
        SET_FUNC(SQL_API_SQLDISCONNECT);
        SET_FUNC(SQL_API_SQLEXECDIRECT);
        SET_FUNC(SQL_API_SQLEXECUTE);
        SET_FUNC(SQL_API_SQLPREPARE);
        SET_FUNC(SQL_API_SQLFETCH);
        SET_FUNC(SQL_API_SQLGETDATA);
        SET_FUNC(SQL_API_SQLNUMRESULTCOLS);
        SET_FUNC(SQL_API_SQLDESCRIBECOL);
        SET_FUNC(SQL_API_SQLCOLATTRIBUTE);
        SET_FUNC(SQL_API_SQLROWCOUNT);
        SET_FUNC(SQL_API_SQLGETINFO);
        SET_FUNC(SQL_API_SQLGETFUNCTIONS);
        SET_FUNC(SQL_API_SQLGETTYPEINFO);
        SET_FUNC(SQL_API_SQLTABLES);
        SET_FUNC(SQL_API_SQLCOLUMNS);
        SET_FUNC(SQL_API_SQLSETCONNECTATTR);
        SET_FUNC(SQL_API_SQLSETSTMTATTR);
        SET_FUNC(SQL_API_SQLSETENVATTR);
        SET_FUNC(SQL_API_SQLGETCONNECTATTR);
        SET_FUNC(SQL_API_SQLGETSTMTATTR);
        SET_FUNC(SQL_API_SQLGETENVATTR);
        SET_FUNC(SQL_API_SQLERROR);
        SET_FUNC(SQL_API_SQLGETDIAGREC);
        SET_FUNC(SQL_API_SQLGETDIAGFIELD);
        SET_FUNC(SQL_API_SQLNATIVESQL);
        SET_FUNC(SQL_API_SQLENDTRAN);
        SET_FUNC(SQL_API_SQLMORERESULTS);
        SET_FUNC(SQL_API_SQLCANCEL);
        SET_FUNC(SQL_API_SQLCLOSECURSOR);
        SET_FUNC(SQL_API_SQLBINDCOL);
        #undef SET_FUNC
        return SQL_SUCCESS;
    }

    // Handle SQL_API_ALL_FUNCTIONS (ODBC 2.x — array of 100 elements)
    if (fFunction == SQL_API_ALL_FUNCTIONS) {
        SQLUSMALLINT* arr = pfSupported;
        memset(arr, 0, 100 * sizeof(SQLUSMALLINT));
        // ODBC 2.x function IDs — set TRUE for implemented ones
        if (SQL_API_SQLALLOCHANDLE < 100) arr[SQL_API_SQLALLOCHANDLE] = SQL_TRUE;
        if (SQL_API_SQLFREEHANDLE < 100) arr[SQL_API_SQLFREEHANDLE] = SQL_TRUE;
        if (SQL_API_SQLDRIVERCONNECT < 100) arr[SQL_API_SQLDRIVERCONNECT] = SQL_TRUE;
        if (SQL_API_SQLCONNECT < 100) arr[SQL_API_SQLCONNECT] = SQL_TRUE;
        if (SQL_API_SQLDISCONNECT < 100) arr[SQL_API_SQLDISCONNECT] = SQL_TRUE;
        if (SQL_API_SQLEXECDIRECT < 100) arr[SQL_API_SQLEXECDIRECT] = SQL_TRUE;
        if (SQL_API_SQLEXECUTE < 100) arr[SQL_API_SQLEXECUTE] = SQL_TRUE;
        if (SQL_API_SQLPREPARE < 100) arr[SQL_API_SQLPREPARE] = SQL_TRUE;
        if (SQL_API_SQLFETCH < 100) arr[SQL_API_SQLFETCH] = SQL_TRUE;
        if (SQL_API_SQLGETDATA < 100) arr[SQL_API_SQLGETDATA] = SQL_TRUE;
        if (SQL_API_SQLNUMRESULTCOLS < 100) arr[SQL_API_SQLNUMRESULTCOLS] = SQL_TRUE;
        if (SQL_API_SQLDESCRIBECOL < 100) arr[SQL_API_SQLDESCRIBECOL] = SQL_TRUE;
        if (SQL_API_SQLROWCOUNT < 100) arr[SQL_API_SQLROWCOUNT] = SQL_TRUE;
        if (SQL_API_SQLGETINFO < 100) arr[SQL_API_SQLGETINFO] = SQL_TRUE;
        if (SQL_API_SQLGETFUNCTIONS < 100) arr[SQL_API_SQLGETFUNCTIONS] = SQL_TRUE;
        if (SQL_API_SQLGETTYPEINFO < 100) arr[SQL_API_SQLGETTYPEINFO] = SQL_TRUE;
        if (SQL_API_SQLTABLES < 100) arr[SQL_API_SQLTABLES] = SQL_TRUE;
        if (SQL_API_SQLCOLUMNS < 100) arr[SQL_API_SQLCOLUMNS] = SQL_TRUE;
        if (SQL_API_SQLERROR < 100) arr[SQL_API_SQLERROR] = SQL_TRUE;
        if (SQL_API_SQLNATIVESQL < 100) arr[SQL_API_SQLNATIVESQL] = SQL_TRUE;
        if (SQL_API_SQLBINDCOL < 100) arr[SQL_API_SQLBINDCOL] = SQL_TRUE;
        if (SQL_API_SQLCANCEL < 100) arr[SQL_API_SQLCANCEL] = SQL_TRUE;
        if (SQL_API_SQLTRANSACT < 100) arr[SQL_API_SQLTRANSACT] = SQL_TRUE;
        return SQL_SUCCESS;
    }

    switch (fFunction) {
        // --- Core functions we implement ---
        case SQL_API_SQLALLOCHANDLE:
        case SQL_API_SQLFREEHANDLE:
        case SQL_API_SQLDRIVERCONNECT:
        case SQL_API_SQLCONNECT:
        case SQL_API_SQLDISCONNECT:
        case SQL_API_SQLEXECDIRECT:
        case SQL_API_SQLEXECUTE:
        case SQL_API_SQLPREPARE:
        case SQL_API_SQLFETCH:
        case SQL_API_SQLGETDATA:
        case SQL_API_SQLNUMRESULTCOLS:
        case SQL_API_SQLDESCRIBECOL:
        case SQL_API_SQLCOLATTRIBUTE:
        case SQL_API_SQLROWCOUNT:
        case SQL_API_SQLGETINFO:
        case SQL_API_SQLGETFUNCTIONS:
        case SQL_API_SQLGETTYPEINFO:
        case SQL_API_SQLTABLES:
        case SQL_API_SQLCOLUMNS:
        case SQL_API_SQLSETCONNECTATTR:
        case SQL_API_SQLSETSTMTATTR:
        case SQL_API_SQLSETENVATTR:
        case SQL_API_SQLGETCONNECTATTR:
        case SQL_API_SQLGETSTMTATTR:
        case SQL_API_SQLGETENVATTR:
        case SQL_API_SQLERROR:
        case SQL_API_SQLGETDIAGREC:
        case SQL_API_SQLGETDIAGFIELD:
        case SQL_API_SQLNATIVESQL:
            *pfSupported = SQL_TRUE;
            break;

        // --- Functions we stub but return success (safe no-ops) ---
        case SQL_API_SQLENDTRAN:      // SQLEndTran — returns SQL_SUCCESS (no txn support)
        case SQL_API_SQLMORERESULTS:  // SQLMoreResults — returns SQL_NO_DATA
        case SQL_API_SQLCANCEL:       // SQLCancel — returns SQL_SUCCESS
        case SQL_API_SQLCLOSECURSOR:  // SQLCloseCursor — implemented, resets statement
        case SQL_API_SQLBINDCOL:     // SQLBindCol — implemented, stores column bindings
            *pfSupported = SQL_TRUE;
            break;

        // --- Functions NOT implemented — must return SQL_FALSE ---
        case SQL_API_SQLBINDPARAMETER:
        case SQL_API_SQLSETPOS:
        case SQL_API_SQLBULKOPERATIONS:
        case SQL_API_SQLFETCHSCROLL:
        case SQL_API_SQLPARAMDATA:
        case SQL_API_SQLPUTDATA:
        case SQL_API_SQLBROWSECONNECT:
        case SQL_API_SQLCOLUMNPRIVILEGES:
        case SQL_API_SQLFOREIGNKEYS:
        case SQL_API_SQLPRIMARYKEYS:
        case SQL_API_SQLPROCEDURECOLUMNS:
        case SQL_API_SQLPROCEDURES:
        case SQL_API_SQLTABLEPRIVILEGES:
        case SQL_API_SQLTRANSACT:
            *pfSupported = SQL_FALSE;
            break;

        default:
            // Unknown function ID — return FALSE to be safe
            *pfSupported = SQL_FALSE;
            break;
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLRowCount(SQLHSTMT hstmt, SQLLEN* pcrow) {
    FuncLogger logger("SQLRowCount");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLRowCount: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->executed) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLRowCount: Statement has not been executed", "HY010");
        return logger.ret(SQL_ERROR);
    }
    if (!pcrow) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT,
                 "SQLRowCount: pcrow is NULL", "HY009");
        return logger.ret(SQL_ERROR);
    }
    if (!stmt->metadata_mode.empty()) {
        *pcrow = (SQLLEN)stmt->meta_rows.size();
    } else {
        *pcrow = stmt->row_count;
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLMoreResults(SQLHSTMT hstmt) {
    FuncLogger logger("SQLMoreResults");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLMoreResults: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    return logger.ret(SQL_NO_DATA);
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT hstmt) {
    FuncLogger logger("SQLCancel");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLCancel: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT hstmt) {
    FuncLogger logger("SQLCloseCursor");
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLCloseCursor: Invalid statement handle", "HY000");
        return logger.ret(SQL_ERROR);
    }
    stmt->current_row = 0;
    stmt->executed = false;
    stmt->metadata_mode = "";
    stmt->columns.clear();
    stmt->rows.clear();
    stmt->meta_columns.clear();
    stmt->meta_rows.clear();
    stmt->bindings.clear();
    return SQL_SUCCESS;
}

// Bind parameter — not supported, return IM001
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
    SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef, SQLSMALLINT ibScale,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLBindParameter: Invalid statement handle", "HY000");
        return SQL_ERROR;
    }
    setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLBindParameter not supported", "IM001");
    return SQL_ERROR;
}

// Bind col — store binding info for SQLFetch to fill bound buffers
SQLRETURN SQL_API SQLBindCol(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLBindCol: Invalid statement handle", "HY000");
        return SQL_ERROR;
    }
    // Check if binding already exists for this column — if so, replace it
    for (auto& b : stmt->bindings) {
        if (b.col == icol) {
            b.fCType = fCType;
            b.rgbValue = rgbValue;
            b.cbValueMax = cbValueMax;
            b.pcbValue = pcbValue;
            return SQL_SUCCESS;
        }
    }
    // Add new binding
    ColBinding b;
    b.col = icol;
    b.fCType = fCType;
    b.rgbValue = rgbValue;
    b.cbValueMax = cbValueMax;
    b.pcbValue = pcbValue;
    stmt->bindings.push_back(b);
    return SQL_SUCCESS;
}

// Error handling
SQLRETURN SQL_API SQLError(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    SQLCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    FuncLogger logger("SQLError");

    // SQLError (ODBC 2.x) — use the first non-null handle among hstmt/hdbc/henv
    SQLHANDLE handle = (hstmt != SQL_NULL_HSTMT) ? (SQLHANDLE)hstmt :
                       (hdbc  != SQL_NULL_HDBC)  ? (SQLHANDLE)hdbc  :
                       (SQLHANDLE)henv;

    LockGuard guard(&g_handleLock);
    auto it = g_errors.find(handle);
    if (it == g_errors.end() || !it->second.has_error) return logger.ret(SQL_NO_DATA);

    SapErrorInfo& ei = it->second;

    if (szSqlState) {
        strncpy_s((char*)szSqlState, 6, ei.sqlstate.c_str(), 5);
    }
    if (pfNativeError) *pfNativeError = ei.native_error;
    if (szErrorMsg && cbErrorMsgMax > 0) {
        strncpy_s((char*)szErrorMsg, cbErrorMsgMax, ei.message.c_str(), cbErrorMsgMax - 1);
    }
    if (pcbErrorMsg) *pcbErrorMsg = (SQLSMALLINT)ei.message.length();

    // SQLError semantics: clear the error after reading
    ei.has_error = false;

    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    FuncLogger logger("SQLGetDiagRec");

    if (RecNumber != 1) return logger.ret(SQL_NO_DATA);

    LockGuard guard(&g_handleLock);
    auto it = g_errors.find(Handle);
    if (it == g_errors.end() || !it->second.has_error) return logger.ret(SQL_NO_DATA);

    SapErrorInfo& ei = it->second;

    if (szSqlState) {
        strncpy_s((char*)szSqlState, 6, ei.sqlstate.c_str(), 5);
    }
    if (pfNativeError) *pfNativeError = ei.native_error;
    if (szErrorMsg && cbErrorMsgMax > 0) {
        strncpy_s((char*)szErrorMsg, cbErrorMsgMax, ei.message.c_str(), cbErrorMsgMax - 1);
    }
    if (pcbErrorMsg) *pcbErrorMsg = (SQLSMALLINT)ei.message.length();

    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfoPtr, SQLSMALLINT BufferLength,
    SQLSMALLINT* StringLengthPtr) {
    return SQL_NO_DATA;
}

// Cursor functions
SQLRETURN SQL_API SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLSetPos: Invalid statement handle", "HY000");
        return SQL_ERROR;
    }
    setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLSetPos: Operation not supported", "IM001");
    return SQL_ERROR;
}

SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT hstmt, SQLSMALLINT Operation) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLBulkOperations: Invalid statement handle", "HY000");
        return SQL_ERROR;
    }
    setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLBulkOperations: Operation not supported", "IM001");
    return SQL_ERROR;
}

// Column count for catalogs etc
SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value,
    SQLINTEGER BufferLength, SQLINTEGER* StringLength) {
    FuncLogger logger("SQLGetConnectAttr");
    // Return SQL_SUCCESS for common attributes QlikSense may query.
    switch (Attribute) {
        case SQL_ATTR_AUTOCOMMIT:
            // Must return SQL_AUTOCOMMIT_ON (1), not 0 — QlikSense checks this
            if (Value) *(SQLUINTEGER*)Value = SQL_AUTOCOMMIT_ON;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_TRACE:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_ACCESS_MODE:
        case SQL_ATTR_CONNECTION_TIMEOUT:
        case SQL_ATTR_LOGIN_TIMEOUT:
        case SQL_ATTR_ODBC_CURSORS:
        case SQL_ATTR_PACKET_SIZE:
        case SQL_ATTR_QUIET_MODE:
        case SQL_ATTR_TRANSLATE_LIB:
        case SQL_ATTR_TRANSLATE_OPTION:
        case SQL_ATTR_TXN_ISOLATION:
            if (Value) memset(Value, 0, BufferLength);
            if (StringLength) *StringLength = 0;
            return SQL_SUCCESS;
        default:
            // Return success for any other attribute to avoid breaking the driver
            if (Value && BufferLength > 0) memset(Value, 0, BufferLength);
            if (StringLength) *StringLength = 0;
            return SQL_SUCCESS;
    }
}

SQLRETURN SQL_API SQLGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value,
    SQLINTEGER BufferLength, SQLINTEGER* StringLength) {
    FuncLogger logger("SQLGetStmtAttr");
    // Return SQL_SUCCESS for common statement attributes QlikSense may query.
    switch (Attribute) {
        case SQL_ATTR_APP_ROW_DESC:
        case SQL_ATTR_APP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
        case SQL_ATTR_IMP_PARAM_DESC:
            // The DM requires valid descriptor handles. We return a dummy
            // descriptor (just a non-NULL pointer). The DM manages its own
            // internal descriptor state; it just needs a non-NULL handle.
            // We allocate a static dummy descriptor per statement.
            {
                // Use the statement handle itself as the "descriptor handle"
                // This is safe because the DM only checks for non-NULL.
                SQLHANDLE dummyDesc = (SQLHANDLE)hstmt;
                if (Value) *(SQLHANDLE*)Value = dummyDesc;
                if (StringLength) *StringLength = sizeof(SQLHANDLE);
                return SQL_SUCCESS;
            }
        case SQL_ATTR_CURSOR_TYPE:
            if (Value) *(SQLUINTEGER*)Value = SQL_CURSOR_FORWARD_ONLY;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_CONCURRENCY:
            if (Value) *(SQLUINTEGER*)Value = SQL_CONCUR_READ_ONLY;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_NOSCAN:
        case SQL_ATTR_QUERY_TIMEOUT:
        case SQL_ATTR_MAX_LENGTH:
        case SQL_ATTR_MAX_ROWS:
        case SQL_ATTR_ASYNC_ENABLE:
        case SQL_ATTR_RETRIEVE_DATA:
        case SQL_ATTR_ROW_BIND_TYPE:
        case SQL_ATTR_ROW_NUMBER:
        case SQL_ATTR_ROW_STATUS_PTR:
        case SQL_ATTR_ROWS_FETCHED_PTR:
        case SQL_ATTR_ROW_ARRAY_SIZE:
        case SQL_ATTR_KEYSET_SIZE:
            if (Value) *(SQLUINTEGER*)Value = 0;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_CURSOR_SCROLLABLE:
            if (Value) *(SQLUINTEGER*)Value = SQL_NONSCROLLABLE;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_CURSOR_SENSITIVITY:
            if (Value) *(SQLUINTEGER*)Value = SQL_INSENSITIVE;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_PARAM_BIND_TYPE:
        case SQL_ATTR_PARAM_STATUS_PTR:
        case SQL_ATTR_PARAMS_PROCESSED_PTR:
        case SQL_ATTR_PARAMSET_SIZE:
            if (Value) *(SQLUINTEGER*)Value = 0;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        default:
            // Unknown attribute — return success with 0 to avoid breaking the driver
            if (Value && BufferLength > 0) memset(Value, 0, BufferLength);
            if (StringLength) *StringLength = 0;
            return SQL_SUCCESS;
    }
}

SQLRETURN SQL_API SQLGetEnvAttr(SQLHENV henv, SQLINTEGER Attribute, SQLPOINTER Value,
    SQLINTEGER BufferLength, SQLINTEGER* StringLength) {
    FuncLogger logger("SQLGetEnvAttr");
    // Return SQL_SUCCESS for common environment attributes
    switch (Attribute) {
        case SQL_ATTR_ODBC_VERSION:
            // Return SQL_OV_ODBC3 (3) — we support ODBC 3.x
            if (Value) *(SQLUINTEGER*)Value = SQL_OV_ODBC3;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_CONNECTION_POOLING:
            if (Value) *(SQLUINTEGER*)Value = SQL_CP_OFF;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_OUTPUT_NTS:
            if (Value) *(SQLUINTEGER*)Value = SQL_TRUE;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        case SQL_ATTR_CP_MATCH:
            if (Value) *(SQLUINTEGER*)Value = SQL_CP_STRICT_MATCH;
            if (StringLength) *StringLength = sizeof(SQLUINTEGER);
            return SQL_SUCCESS;
        default:
            // Unknown attribute — return success with 0
            if (Value && BufferLength > 0) memset(Value, 0, BufferLength);
            if (StringLength) *StringLength = 0;
            return SQL_SUCCESS;
    }
}

SQLRETURN SQL_API SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR* szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLCHAR* szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT* pcbConnStrOut) {
    // We don't support browse connect — return SQL_ERROR
    setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLBrowseConnect: Operation not supported", "IM001");
    return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetCursorName(SQLHSTMT hstmt, SQLCHAR* szCursor, SQLSMALLINT cbCursorMax,
    SQLSMALLINT* pcbCursor) {
    // We don't support named cursors — return SQL_ERROR
    setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLGetCursorName: Named cursors not supported", "IM001");
    return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetCursorName(SQLHSTMT hstmt, SQLCHAR* szCursor, SQLSMALLINT cbCursor) {
    // We don't support named cursors — return SQL_ERROR
    setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLSetCursorName: Named cursors not supported", "IM001");
    return SQL_ERROR;
}

// Misc stubs
SQLRETURN SQL_API SQLNativeSql(SQLHDBC hdbc, SQLCHAR* szSqlStrIn, SQLINTEGER cbSqlStrIn,
    SQLCHAR* szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER* pcbSqlStr) {
    // Pass through — we accept native SQL
    if (!szSqlStrIn) {
        setError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, "SQLNativeSql: Input SQL string is NULL", "HY009");
        return SQL_ERROR;
    }
    if (szSqlStr && cbSqlStrMax > 0) {
        strncpy_s((char*)szSqlStr, cbSqlStrMax, (char*)szSqlStrIn, cbSqlStrMax - 1);
    }
    if (pcbSqlStr) *pcbSqlStr = (cbSqlStrIn == SQL_NTS) ? (SQLINTEGER)strlen((char*)szSqlStrIn) : cbSqlStrIn;
    return SQL_SUCCESS;
}

// ============================================================
// DSN Configuration (ConfigDSN)
// ============================================================

// Parse lpszAttributes (null-separated "key=value" pairs, double-null terminated)
// into a map of key -> value.
static std::map<std::string, std::string> parseAttributes(LPCSTR lpszAttributes) {
    std::map<std::string, std::string> attrs;
    if (!lpszAttributes) return attrs;

    const char* p = lpszAttributes;
    while (*p) {
        std::string pair(p);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
            attrs[key] = val;
        }
        p += pair.length() + 1;  // skip past the null terminator
    }
    return attrs;
}

// Helper: write a string value to a registry key.
static BOOL regWriteString(HKEY hKey, const char* valueName, const std::string& value) {
    LONG rc = RegSetValueExA(hKey, valueName, 0, REG_SZ,
                             (const BYTE*)value.c_str(),
                             (DWORD)(value.length() + 1));
    return (rc == ERROR_SUCCESS) ? TRUE : FALSE;
}

// Helper: read a string value from a registry key.
static std::string regReadString(HKEY hKey, const char* valueName) {
    char buf[1024] = {0};
    DWORD size = sizeof(buf);
    DWORD type = 0;
    LONG rc = RegQueryValueExA(hKey, valueName, NULL, &type, (LPBYTE)buf, &size);
    if (rc == ERROR_SUCCESS && type == REG_SZ) {
        return std::string(buf);
    }
    return "";
}

// ============================================================
// DSN Configuration Dialog (DialogBoxParam + .rc resource)
// ============================================================

// Context passed to the dialog procedure
struct ConfigDlgContext {
    std::string dsnName;
    std::string driver;
    WORD fRequest;  // ODBC_ADD_DSN or ODBC_CONFIG_DSN
};

// Read text from an edit control into a std::string
static std::string getEditText(HWND hDlg, int controlId) {
    HWND hEdit = GetDlgItem(hDlg, controlId);
    if (!hEdit) return "";
    int len = GetWindowTextLengthA(hEdit);
    if (len <= 0) return "";
    std::string buf(len + 1, '\0');
    int actual = GetWindowTextA(hEdit, &buf[0], len + 1);
    buf.resize(actual);
    return buf;
}

// Set text in an edit control
static void setEditText(HWND hDlg, int controlId, const std::string& text) {
    SetDlgItemTextA(hDlg, controlId, text.c_str());
}

// ---- DialogBoxParam-based modal dialog implementation ----
// Uses a .rc resource (DLG_CONFIG) compiled with windres.
// This replaces the CreateWindowEx approach which had issues with
// control creation and modal behavior in odbcad32.exe.

// Dialog procedure for ConfigDSN
static INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT message,
                                       WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            ConfigDlgContext* ctx =
                reinterpret_cast<ConfigDlgContext*>(lParam);
            SetWindowLongPtrA(hDlg, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(ctx));

            // Set window title
            std::string title = "Configure SAP ODBC DSN";
            if (ctx->fRequest == ODBC_ADD_DSN)
                title = "Add SAP ODBC DSN";
            SetWindowTextA(hDlg, title.c_str());

            // Read current values from registry (for CONFIG) or use defaults
            std::string host, sysnr, client, user, password, lang, maxrows, logpath;
            std::string dsn = ctx->dsnName;

            if (ctx->fRequest == ODBC_CONFIG_DSN) {
                std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + ctx->dsnName;
                HKEY hKey;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(),
                                  0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    host = regReadString(hKey, "Host");
                    sysnr = regReadString(hKey, "SysNr");
                    client = regReadString(hKey, "Client");
                    user = regReadString(hKey, "User");
                    password = regReadString(hKey, "Password");
                    lang = regReadString(hKey, "Lang");
                    maxrows = regReadString(hKey, "MaxRows");
                    logpath = regReadString(hKey, "TableLogPath");
                    RegCloseKey(hKey);
                }
            }

            // Set initial field values
            setEditText(hDlg, IDC_DSN_EDIT, dsn);
            setEditText(hDlg, IDC_HOST_EDIT, host);
            setEditText(hDlg, IDC_SYSNR_EDIT, sysnr);
            setEditText(hDlg, IDC_CLIENT_EDIT, client);
            setEditText(hDlg, IDC_USER_EDIT, user);
            setEditText(hDlg, IDC_PASSWORD_EDIT, password);
            setEditText(hDlg, IDC_LANG_EDIT, lang.empty() ? "EN" : lang);
            setEditText(hDlg, IDC_MAXROWS_EDIT, maxrows.empty() ? "30000" : maxrows);
            setEditText(hDlg, IDC_LOGPATH_EDIT, logpath);

            // For CONFIG, DSN name is not editable
            if (ctx->fRequest == ODBC_CONFIG_DSN) {
                HWND hDsn = GetDlgItem(hDlg, IDC_DSN_EDIT);
                if (hDsn) EnableWindow(hDsn, FALSE);
            }

            return TRUE;  // Let system set focus to the first control
        }

        case WM_COMMAND: {
            WORD cmdId = LOWORD(wParam);
            switch (cmdId) {
                case IDC_TEST_BTN: {
                    ConnectionParams params;
                    params.host = getEditText(hDlg, IDC_HOST_EDIT);
                    params.sysnr = getEditText(hDlg, IDC_SYSNR_EDIT);
                    params.client = getEditText(hDlg, IDC_CLIENT_EDIT);
                    params.user = getEditText(hDlg, IDC_USER_EDIT);
                    params.password = getEditText(hDlg, IDC_PASSWORD_EDIT);
                    params.lang = getEditText(hDlg, IDC_LANG_EDIT);
                    std::string maxRowsStr = getEditText(hDlg, IDC_MAXROWS_EDIT);
                    params.max_rows = maxRowsStr.empty() ? MAX_ROWS_DEFAULT : atoi(maxRowsStr.c_str());

                    SapConnection conn;
                    conn.params = params;
                    conn.connected = false;
                    conn.rfc_conn = NULL;

                    if (rfcConnect(&conn)) {
                        MessageBoxA(hDlg, "Connection successful!", "Test Connection",
                                    MB_OK | MB_ICONINFORMATION);
                        rfcDisconnect(&conn);
                    } else {
                        std::string msg = "Connection failed: " + conn.error_msg;
                        MessageBoxA(hDlg, msg.c_str(), "Test Connection",
                                    MB_OK | MB_ICONERROR);
                    }
                    return TRUE;
                }

                case IDOK: {
                    std::string dsn = getEditText(hDlg, IDC_DSN_EDIT);
                    std::string host = getEditText(hDlg, IDC_HOST_EDIT);
                    std::string sysnr = getEditText(hDlg, IDC_SYSNR_EDIT);
                    std::string client = getEditText(hDlg, IDC_CLIENT_EDIT);
                    std::string user = getEditText(hDlg, IDC_USER_EDIT);
                    std::string password = getEditText(hDlg, IDC_PASSWORD_EDIT);
                    std::string lang = getEditText(hDlg, IDC_LANG_EDIT);
                    std::string maxrows = getEditText(hDlg, IDC_MAXROWS_EDIT);
                    std::string logpath = getEditText(hDlg, IDC_LOGPATH_EDIT);

                    if (dsn.empty()) {
                        MessageBoxA(hDlg, "DSN name is required.", "Error",
                                    MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    ConfigDlgContext* ctx = reinterpret_cast<ConfigDlgContext*>(
                        GetWindowLongPtrA(hDlg, GWLP_USERDATA));

                    // Build registry path
                    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + dsn;
                    HKEY hKey;
                    DWORD disposition = 0;
                    LONG rc = RegCreateKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0,
                                              NULL, REG_OPTION_NON_VOLATILE,
                                              KEY_WRITE, NULL, &hKey, &disposition);
                    if (rc != ERROR_SUCCESS) {
                        MessageBoxA(hDlg, "Cannot create/open registry key.", "Error",
                                    MB_OK | MB_ICONERROR);
                        EndDialog(hDlg, FALSE);
                        return TRUE;
                    }

                    // Write Driver — must be the DLL path, not the driver name.
                    if (ctx && !ctx->driver.empty()) {
                        std::string instPath = "SOFTWARE\\ODBC\\ODBCINST.INI\\" + ctx->driver;
                        HKEY hInstKey;
                        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, instPath.c_str(),
                                          0, KEY_READ, &hInstKey) == ERROR_SUCCESS) {
                            std::string dllPath = regReadString(hInstKey, "Driver");
                            RegCloseKey(hInstKey);
                            if (!dllPath.empty()) {
                                regWriteString(hKey, "Driver", dllPath);
                            }
                        }
                    }

                    // Write all connection params
                    regWriteString(hKey, "Host", host);
                    regWriteString(hKey, "SysNr", sysnr);
                    regWriteString(hKey, "Client", client);
                    regWriteString(hKey, "User", user);
                    regWriteString(hKey, "Password", password);
                    regWriteString(hKey, "Lang", lang.empty() ? "EN" : lang);
                    regWriteString(hKey, "MaxRows", maxrows.empty() ? "30000" : maxrows);
                    regWriteString(hKey, "TableLogPath", logpath);

                    RegCloseKey(hKey);

                    // Register in ODBC Data Sources list
                    HKEY hSourcesKey;
                    rc = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                          "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",
                                          0, NULL, REG_OPTION_NON_VOLATILE,
                                          KEY_WRITE, NULL, &hSourcesKey, &disposition);
                    if (rc == ERROR_SUCCESS) {
                        regWriteString(hSourcesKey, dsn.c_str(), ctx ? ctx->driver.c_str() : "SAP via Z_EXECUTE_SQL");
                        RegCloseKey(hSourcesKey);
                    }

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                }

                case IDCANCEL: {
                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
            }
            break;
        }

        case WM_CLOSE: {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }
    }
    return FALSE;  // Let DefDlgProc handle everything else
}

// Show the config dialog using DialogBoxParam with the .rc resource
static BOOL showConfigDialog(HWND hwndParent, ConfigDlgContext& ctx) {
    INT_PTR result = DialogBoxParam(
        g_hInstance,
        MAKEINTRESOURCE(DLG_CONFIG),
        hwndParent,
        ConfigDlgProc,
        reinterpret_cast<LPARAM>(&ctx));
    return (result == IDOK || result == TRUE) ? TRUE : FALSE;
}

BOOL SQL_API ConfigDSN(HWND hwndParent, WORD fRequest, LPCSTR lpszDriver,
                       LPCSTR lpszAttributes) {
    // Parse attributes
    std::map<std::string, std::string> attrs = parseAttributes(lpszAttributes);

    // DSN name is required
    auto it = attrs.find("DSN");
    if (it == attrs.end() || it->second.empty()) {
        return FALSE;
    }
    std::string dsnName = it->second;

    // Build registry path: HKLM\SOFTWARE\ODBC\ODBC.INI\<DSN>
    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + dsnName;

    switch (fRequest) {
        case ODBC_ADD_DSN:
        case ODBC_CONFIG_DSN: {
            // Show the configuration dialog
            ConfigDlgContext ctx;
            ctx.dsnName = dsnName;
            ctx.driver = (lpszDriver && *lpszDriver) ? std::string(lpszDriver) : "";
            ctx.fRequest = fRequest;

            BOOL dialogResult = showConfigDialog(hwndParent, ctx);
            return dialogResult;
        }

        case ODBC_REMOVE_DSN: {
            // Delete the DSN registry key and all its values
            LONG rc = RegDeleteKeyA(HKEY_LOCAL_MACHINE, regPath.c_str());
            if (rc != ERROR_SUCCESS) {
                return FALSE;
            }

            // Also remove from ODBC Data Sources list
            HKEY hSourcesKey;
            rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                               "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",
                               0, KEY_SET_VALUE, &hSourcesKey);
            if (rc == ERROR_SUCCESS) {
                RegDeleteValueA(hSourcesKey, dsnName.c_str());
                RegCloseKey(hSourcesKey);
            }

            return TRUE;
        }

        default:
            // Unknown fRequest
            return FALSE;
    }
    return FALSE;
}

// ============================================================
// A/W Aliases — ODBC Driver Manager looks for SQLxxxA and SQLxxxW
// variants. These are simple wrappers that forward to the main
// implementations.
// ============================================================
extern "C" {

SQLRETURN SQL_API SQLDriverConnectA(SQLHDBC hdbc, SQLHWND hwnd,
    SQLCHAR* szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLCHAR* szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
    return SQLDriverConnect(hdbc, hwnd, szConnStrIn, cbConnStrIn,
        szConnStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
}
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC hdbc, SQLHWND hwnd,
    SQLWCHAR* szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLWCHAR* szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
    // Convert wide to narrow, call ANSI version
    int len = cbConnStrIn == SQL_NTS ? lstrlenW(szConnStrIn) : cbConnStrIn;
    SQLCHAR* narrow = (SQLCHAR*)_alloca(len + 1);
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)szConnStrIn, len, (LPSTR)narrow, len, NULL, NULL);
    narrow[len] = 0;
    SQLCHAR outBuf[1024] = {0};
    SQLSMALLINT outLen = 0;
    SQLRETURN rc = SQLDriverConnect(hdbc, hwnd, narrow, SQL_NTS, outBuf, sizeof(outBuf), &outLen, fDriverCompletion);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        // Convert output to wide
        int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)outBuf, -1, (LPWSTR)szConnStrOut, cbConnStrOutMax);
        if (pcbConnStrOut) *pcbConnStrOut = (SQLSMALLINT)(wlen - 1);
    }
    return rc;
}

SQLRETURN SQL_API SQLConnectA(SQLHDBC hdbc, SQLCHAR* szDSN, SQLSMALLINT cbDSN,
    SQLCHAR* szUID, SQLSMALLINT cbUID, SQLCHAR* szAuthStr, SQLSMALLINT cbAuthStr) {
    return SQLConnect(hdbc, szDSN, cbDSN, szUID, cbUID, szAuthStr, cbAuthStr);
}
SQLRETURN SQL_API SQLConnectW(SQLHDBC hdbc, SQLWCHAR* szDSN, SQLSMALLINT cbDSN,
    SQLWCHAR* szUID, SQLSMALLINT cbUID, SQLWCHAR* szAuthStr, SQLSMALLINT cbAuthStr) {
    return SQLConnect(hdbc, (SQLCHAR*)szDSN, cbDSN, (SQLCHAR*)szUID, cbUID, (SQLCHAR*)szAuthStr, cbAuthStr);
}

SQLRETURN SQL_API SQLExecDirectA(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    return SQLExecDirect(hstmt, szSqlStr, cbSqlStr);
}
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT hstmt, SQLWCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    // Convert wide SQL to narrow
    int len = cbSqlStr == SQL_NTS ? lstrlenW(szSqlStr) : cbSqlStr;
    SQLCHAR* narrow = (SQLCHAR*)_alloca(len * 2 + 1);
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)szSqlStr, len, (LPSTR)narrow, len * 2, NULL, NULL);
    narrow[len] = 0;
    return SQLExecDirect(hstmt, narrow, SQL_NTS);
}

SQLRETURN SQL_API SQLPrepareA(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    return SQLPrepare(hstmt, szSqlStr, cbSqlStr);
}
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT hstmt, SQLWCHAR* szSqlStr, SQLINTEGER cbSqlStr) {
    int len = cbSqlStr == SQL_NTS ? lstrlenW(szSqlStr) : cbSqlStr;
    SQLCHAR* narrow = (SQLCHAR*)_alloca(len * 2 + 1);
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)szSqlStr, len, (LPSTR)narrow, len * 2, NULL, NULL);
    narrow[len] = 0;
    return SQLPrepare(hstmt, narrow, SQL_NTS);
}

SQLRETURN SQL_API SQLExecuteA(SQLHSTMT hstmt) { return SQLExecute(hstmt); }
SQLRETURN SQL_API SQLExecuteW(SQLHSTMT hstmt) { return SQLExecute(hstmt); }
SQLRETURN SQL_API SQLFetchA(SQLHSTMT hstmt) { return SQLFetch(hstmt); }
SQLRETURN SQL_API SQLFetchW(SQLHSTMT hstmt) { return SQLFetch(hstmt); }
SQLRETURN SQL_API SQLGetDataA(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    return SQLGetData(hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue);
}
SQLRETURN SQL_API SQLGetDataW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    /* SQLGetDataW must return UTF-16 strings when the caller requests SQL_C_WCHAR.
       The underlying SQLGetData only produces ANSI (SQL_C_CHAR) strings, so we
       call it with SQL_C_CHAR to get the ANSI bytes, then convert to wide.
       For non-string types (SQL_C_LONG, SQL_C_DOUBLE, etc.) the binary value
       is identical, so we just forward directly. */
    if (fCType != SQL_C_WCHAR) {
        /* Not a wide-char request — binary/numeric values are the same */
        return SQLGetData(hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue);
    }

    /* Wide-char request: call ANSI version to get the string, then convert */
    SQLCHAR ansiBuf[4096] = {0};
    SQLLEN ansiLen = 0;
    SQLRETURN rc = SQLGetData(hstmt, icol, SQL_C_CHAR, ansiBuf, sizeof(ansiBuf), &ansiLen);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return rc;
    }

    /* If SQLGetData reported truncation on the ANSI side, the full string is
       longer than our buffer.  ansiLen holds the total (untruncated) length.
       We still convert what we have — the caller will see SQL_SUCCESS_WITH_INFO. */
    SQLCHAR* ansiPtr = ansiBuf;
    SQLCHAR* heapBuf = NULL;
    if (rc == SQL_SUCCESS_WITH_INFO && ansiLen > (SQLLEN)(sizeof(ansiBuf) - 1)) {
        /* Allocate a heap buffer for the full ANSI string */
        heapBuf = (SQLCHAR*)malloc(ansiLen + 1);
        if (heapBuf) {
            SQLLEN heapLen = 0;
            SQLRETURN rc2 = SQLGetData(hstmt, icol, SQL_C_CHAR, heapBuf, ansiLen + 1, &heapLen);
            if (rc2 == SQL_SUCCESS || rc2 == SQL_SUCCESS_WITH_INFO) {
                heapBuf[ansiLen] = 0;
                ansiPtr = heapBuf;
            }
        }
    }

    if (rgbValue && cbValueMax > 0) {
        int maxChars = (int)(cbValueMax / sizeof(SQLWCHAR));
        /* First, query the required number of wide characters (excluding null) */
        int wideLen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ansiPtr, -1, NULL, 0);
        int copyChars = wideLen; /* includes null terminator */

        if (copyChars > maxChars) {
            /* Truncation: the wide string doesn't fit — copy what we can */
            if (maxChars > 0) {
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ansiPtr, -1, (LPWSTR)rgbValue, maxChars);
                /* Ensure null-termination */
                ((SQLWCHAR*)rgbValue)[maxChars - 1] = 0;
            }
            if (pcbValue) *pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
            if (heapBuf) free(heapBuf);
            setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "String data, right truncated", "01004");
            return SQL_SUCCESS_WITH_INFO;
        }

        /* Fits — copy the whole string including null terminator */
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ansiPtr, -1, (LPWSTR)rgbValue, maxChars);
        if (pcbValue) *pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
    } else {
        /* Caller just wants the length */
        int wideLen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ansiPtr, -1, NULL, 0);
        if (pcbValue) *pcbValue = (SQLLEN)((wideLen - 1) * sizeof(SQLWCHAR));
    }

    if (heapBuf) free(heapBuf);
    return rc;
}
SQLRETURN SQL_API SQLNumResultColsA(SQLHSTMT hstmt, SQLSMALLINT* pccol) { return SQLNumResultCols(hstmt, pccol); }
SQLRETURN SQL_API SQLNumResultColsW(SQLHSTMT hstmt, SQLSMALLINT* pccol) { return SQLNumResultCols(hstmt, pccol); }
SQLRETURN SQL_API SQLDescribeColA(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLCHAR* szColName,
    SQLSMALLINT cbColNameMax, SQLSMALLINT* pcbColName, SQLSMALLINT* pfSqlType,
    SQLUINTEGER* pcbColDef, SQLSMALLINT* pibScale, SQLSMALLINT* pfNullable) {
    return SQLDescribeCol(hstmt, icol, szColName, cbColNameMax, pcbColName, pfSqlType, (SQLULEN*)pcbColDef, pibScale, pfNullable);
}
SQLRETURN SQL_API SQLDescribeColW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLWCHAR* szColName,
    SQLSMALLINT cbColNameMax, SQLSMALLINT* pcbColName, SQLSMALLINT* pfSqlType,
    SQLULEN* pcbColDef, SQLSMALLINT* pibScale, SQLSMALLINT* pfNullable) {
    // Call ANSI version, convert result to wide
    SQLCHAR narrow[256] = {0};
    SQLSMALLINT narrowLen = 0;
    SQLRETURN rc = SQLDescribeCol(hstmt, icol, narrow, sizeof(narrow), &narrowLen, pfSqlType, pcbColDef, pibScale, pfNullable);
    if ((rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) && szColName && cbColNameMax > 0) {
        int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)narrow, -1, (LPWSTR)szColName, cbColNameMax);
        if (pcbColName) *pcbColName = (SQLSMALLINT)(wlen - 1);
    }
    return rc;
}
SQLRETURN SQL_API SQLColAttributeA(SQLHSTMT hstmt, SQLSMALLINT iCol, SQLSMALLINT iField,
    SQLPOINTER pCharAttr, SQLSMALLINT cbCharAttrMax, SQLSMALLINT* pcbCharAttr, SQLLEN* pNumAttr) {
    return SQLColAttribute(hstmt, (SQLUSMALLINT)iCol, (SQLUSMALLINT)iField, pCharAttr, cbCharAttrMax, pcbCharAttr, pNumAttr);
}
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT hstmt, SQLUSMALLINT iCol, SQLUSMALLINT iField,
    SQLPOINTER pCharAttr, SQLSMALLINT cbCharAttrMax, SQLSMALLINT* pcbCharAttr, SQLLEN* pNumAttr) {
    return SQLColAttribute(hstmt, iCol, iField, pCharAttr, cbCharAttrMax, pcbCharAttr, pNumAttr);
}
SQLRETURN SQL_API SQLRowCountA(SQLHSTMT hstmt, SQLLEN* pcrow) { return SQLRowCount(hstmt, pcrow); }
SQLRETURN SQL_API SQLRowCountW(SQLHSTMT hstmt, SQLLEN* pcrow) { return SQLRowCount(hstmt, pcrow); }
SQLRETURN SQL_API SQLFetchScrollA(SQLHSTMT hstmt, SQLSMALLINT fFetchOrientation, SQLLEN irow) { return SQLFetchScroll(hstmt, fFetchOrientation, irow); }
SQLRETURN SQL_API SQLFetchScrollW(SQLHSTMT hstmt, SQLSMALLINT fFetchOrientation, SQLLEN irow) { return SQLFetchScroll(hstmt, fFetchOrientation, irow); }
SQLRETURN SQL_API SQLMoreResultsA(SQLHSTMT hstmt) { return SQLMoreResults(hstmt); }
SQLRETURN SQL_API SQLMoreResultsW(SQLHSTMT hstmt) { return SQLMoreResults(hstmt); }
SQLRETURN SQL_API SQLCancelA(SQLHSTMT hstmt) { return SQLCancel(hstmt); }
SQLRETURN SQL_API SQLCancelW(SQLHSTMT hstmt) { return SQLCancel(hstmt); }
SQLRETURN SQL_API SQLCloseCursorA(SQLHSTMT hstmt) { return SQLCloseCursor(hstmt); }
SQLRETURN SQL_API SQLCloseCursorW(SQLHSTMT hstmt) { return SQLCloseCursor(hstmt); }
SQLRETURN SQL_API SQLBindColA(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    return SQLBindCol(hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue);
}
SQLRETURN SQL_API SQLBindColW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    return SQLBindCol(hstmt, icol, fCType, rgbValue, cbValueMax, pcbValue);
}
SQLRETURN SQL_API SQLDisconnectA(SQLHDBC hdbc) { return SQLDisconnect(hdbc); }
SQLRETURN SQL_API SQLDisconnectW(SQLHDBC hdbc) { return SQLDisconnect(hdbc); }
SQLRETURN SQL_API SQLGetInfoA(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT* pcbInfoValue) {
    return SQLGetInfo(hdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);
}
SQLRETURN SQL_API SQLGetInfoW(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT* pcbInfoValue) {
    /* SQLGetInfoW must return Unicode (UTF-16) strings for string-type info types.
       For numeric/flag types, the binary value is the same. */
    /* Determine if this is a string type or numeric type */
    SQLSMALLINT infoType = 0; /* 0 = unknown/numeric, 1 = string */
    switch (fInfoType) {
        case SQL_DRIVER_NAME: case SQL_DBMS_NAME: case SQL_DBMS_VER:
        case SQL_DRIVER_VER: case SQL_DRIVER_ODBC_VER:
        case SQL_DATA_SOURCE_NAME: case SQL_SERVER_NAME:
        case SQL_DATA_SOURCE_READ_ONLY: case SQL_IDENTIFIER_QUOTE_CHAR:
        case SQL_CATALOG_NAME_SEPARATOR: case SQL_CATALOG_TERM:
        case SQL_TABLE_TERM: case SQL_SCHEMA_TERM:
        case SQL_COLUMN_ALIAS:
        case SQL_SEARCH_PATTERN_ESCAPE: case SQL_SPECIAL_CHARACTERS:
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG: case SQL_OUTER_JOINS:
        case SQL_CURSOR_COMMIT_BEHAVIOR: case SQL_CURSOR_ROLLBACK_BEHAVIOR:
        case SQL_DATABASE_NAME: case SQL_PROCEDURE_TERM:
        case SQL_KEYWORDS: case SQL_CATALOG_NAME:
        case SQL_COLLATION_SEQ: case SQL_DM_VER:
            infoType = 1; /* string */
            break;
        default:
            /* Check by range: ODBC info types >= 10000 are typically string */
            if (fInfoType >= 10000) infoType = 1;
            break;
    }

    if (infoType == 1) {
        /* String type: call ANSI version, convert to wide */
        SQLCHAR ansiBuf[512] = {0};
        SQLSMALLINT ansiLen = 0;
        SQLRETURN rc = SQLGetInfo(hdbc, fInfoType, ansiBuf, sizeof(ansiBuf), &ansiLen);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
            if (rgbInfoValue && cbInfoValueMax > 0) {
                int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ansiBuf, -1,
                    (LPWSTR)rgbInfoValue, cbInfoValueMax / sizeof(SQLWCHAR));
                if (pcbInfoValue) *pcbInfoValue = (SQLSMALLINT)((wlen - 1) * sizeof(SQLWCHAR));
            }
        }
        return rc;
    } else {
        /* Numeric/flag type: binary value is the same */
        return SQLGetInfo(hdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);
    }
}
SQLRETURN SQL_API SQLGetTypeInfoA(SQLHSTMT hstmt, SQLSMALLINT fDataType) { return SQLGetTypeInfo(hstmt, fDataType); }
SQLRETURN SQL_API SQLGetTypeInfoW(SQLHSTMT hstmt, SQLSMALLINT fDataType) { return SQLGetTypeInfo(hstmt, fDataType); }
SQLRETURN SQL_API SQLTablesA(SQLHSTMT hstmt, SQLCHAR* ct, SQLSMALLINT ctLen,
    SQLCHAR* sch, SQLSMALLINT schLen, SQLCHAR* tab, SQLSMALLINT tabLen,
    SQLCHAR* tabType, SQLSMALLINT tabTypeLen) {
    return SQLTables(hstmt, ct, ctLen, sch, schLen, tab, tabLen, tabType, tabTypeLen);
}
SQLRETURN SQL_API SQLTablesW(SQLHSTMT hstmt, SQLWCHAR* ct, SQLSMALLINT ctLen,
    SQLWCHAR* sch, SQLSMALLINT schLen, SQLWCHAR* tab, SQLSMALLINT tabLen,
    SQLWCHAR* tabType, SQLSMALLINT tabTypeLen) {
    return SQLTables(hstmt, (SQLCHAR*)ct, ctLen, (SQLCHAR*)sch, schLen, (SQLCHAR*)tab, tabLen, (SQLCHAR*)tabType, tabTypeLen);
}
SQLRETURN SQL_API SQLColumnsA(SQLHSTMT hstmt, SQLCHAR* ct, SQLSMALLINT ctLen,
    SQLCHAR* sch, SQLSMALLINT schLen, SQLCHAR* tab, SQLSMALLINT tabLen,
    SQLCHAR* col, SQLSMALLINT colLen) {
    return SQLColumns(hstmt, ct, ctLen, sch, schLen, tab, tabLen, col, colLen);
}
SQLRETURN SQL_API SQLColumnsW(SQLHSTMT hstmt, SQLWCHAR* ct, SQLSMALLINT ctLen,
    SQLWCHAR* sch, SQLSMALLINT schLen, SQLWCHAR* tab, SQLSMALLINT tabLen,
    SQLWCHAR* col, SQLSMALLINT colLen) {
    return SQLColumns(hstmt, (SQLCHAR*)ct, ctLen, (SQLCHAR*)sch, schLen, (SQLCHAR*)tab, tabLen, (SQLCHAR*)col, colLen);
}
SQLRETURN SQL_API SQLErrorA(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    SQLCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    return SQLError(henv, hdbc, hstmt, szSqlState, pfNativeError, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
}
SQLRETURN SQL_API SQLErrorW(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    SQLWCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLWCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    return SQLError(henv, hdbc, hstmt, (SQLCHAR*)szSqlState, pfNativeError, (SQLCHAR*)szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
}
SQLRETURN SQL_API SQLGetDiagRecA(SQLSMALLINT hType, SQLHANDLE h, SQLSMALLINT iRec,
    SQLCHAR* szSqlState, SQLINTEGER* pfNative, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    return SQLGetDiagRec(hType, h, iRec, szSqlState, pfNative, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
}
SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT hType, SQLHANDLE h, SQLSMALLINT iRec,
    SQLWCHAR* szSqlState, SQLINTEGER* pfNative, SQLWCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    // Call ANSI version, convert results to wide
    SQLCHAR state[6] = {0};
    SQLCHAR msg[1024] = {0};
    SQLSMALLINT msgLen = 0;
    SQLRETURN rc = SQLGetDiagRec(hType, h, iRec, state, pfNative, msg, sizeof(msg), &msgLen);
    if (rc == SQL_SUCCESS) {
        if (szSqlState) MultiByteToWideChar(CP_ACP, 0, (LPCSTR)state, -1, (LPWSTR)szSqlState, 6);
        if (szErrorMsg && cbErrorMsgMax > 0) {
            int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)msg, -1, (LPWSTR)szErrorMsg, cbErrorMsgMax);
            if (pcbErrorMsg) *pcbErrorMsg = (SQLSMALLINT)(wlen - 1);
        }
    }
    return rc;
}
SQLRETURN SQL_API SQLSetConnectAttrA(SQLHDBC hdbc, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER strLen) {
    return SQLSetConnectAttr(hdbc, attr, val, strLen);
}
SQLRETURN SQL_API SQLSetConnectAttrW(SQLHDBC hdbc, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER strLen) {
    return SQLSetConnectAttr(hdbc, attr, val, strLen);
}
SQLRETURN SQL_API SQLSetStmtAttrA(SQLHSTMT hstmt, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER strLen) {
    return SQLSetStmtAttr(hstmt, attr, val, strLen);
}
SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER strLen) {
    return SQLSetStmtAttr(hstmt, attr, val, strLen);
}
SQLRETURN SQL_API SQLGetConnectAttrA(SQLHDBC hdbc, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER bufLen, SQLINTEGER* strLen) {
    return SQLGetConnectAttr(hdbc, attr, val, bufLen, strLen);
}
SQLRETURN SQL_API SQLGetConnectAttrW(SQLHDBC hdbc, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER bufLen, SQLINTEGER* strLen) {
    return SQLGetConnectAttr(hdbc, attr, val, bufLen, strLen);
}
SQLRETURN SQL_API SQLGetStmtAttrA(SQLHSTMT hstmt, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER bufLen, SQLINTEGER* strLen) {
    return SQLGetStmtAttr(hstmt, attr, val, bufLen, strLen);
}
SQLRETURN SQL_API SQLGetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER bufLen, SQLINTEGER* strLen) {
    return SQLGetStmtAttr(hstmt, attr, val, bufLen, strLen);
}

} // extern "C"