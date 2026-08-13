#include "sap_odbc.h"

// Handle registry — maps ODBC handles to our internal structs
static std::map<SQLHANDLE, SapConnection*> g_connections;
static std::map<SQLHANDLE, SapStatement*> g_statements;

SapConnection* getConnectionHandle(SQLHANDLE h) {
    auto it = g_connections.find(h);
    return (it != g_connections.end()) ? it->second : nullptr;
}

SapStatement* getStatementHandle(SQLHANDLE h) {
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
    g_errors[handle] = ei;
}

void clearError(SQLHANDLE handle, SQLSMALLINT handleType) {
    g_errors.erase(handle);
}

// ============================================================
// ODBC Driver Entry Point
// ============================================================

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            RfcInit();
            break;
    }
    return TRUE;
}

// ============================================================
// Environment Handle
// ============================================================

SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE* OutputHandle) {
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
            g_connections[h] = conn;
            *OutputHandle = h;
            return SQL_SUCCESS;
        }

        case SQL_HANDLE_STMT: {
            SapConnection* conn = getConnectionHandle(InputHandle);
            if (!conn) return SQL_ERROR;
            SapStatement* stmt = new SapStatement();
            stmt->connection = conn;
            stmt->current_row = 0;
            stmt->row_count = 0;
            stmt->executed = false;
            stmt->metadata_mode = "";
            SQLHANDLE h = (SQLHANDLE)stmt;
            g_statements[h] = stmt;
            *OutputHandle = h;
            return SQL_SUCCESS;
        }

        default:
            return SQL_ERROR;
    }
}

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle) {
    switch (HandleType) {
        case SQL_HANDLE_ENV:
            return SQL_SUCCESS;

        case SQL_HANDLE_DBC: {
            SapConnection* conn = getConnectionHandle(Handle);
            if (conn) {
                rfcDisconnect(conn);
                g_connections.erase(Handle);
                delete conn;
            }
            return SQL_SUCCESS;
        }

        case SQL_HANDLE_STMT: {
            SapStatement* stmt = getStatementHandle(Handle);
            if (stmt) {
                g_statements.erase(Handle);
                delete stmt;
            }
            return SQL_SUCCESS;
        }

        default:
            return SQL_ERROR;
    }
}

// ============================================================
// Connection
// ============================================================

SQLRETURN SQL_API SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd,
    SQLCHAR* szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLCHAR* szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {

    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);
    if (!conn) return SQL_ERROR;

    std::string connstr((char*)szConnStrIn, cbConnStrIn);
    conn->params = parseConnectionString(connstr);

    if (!rfcConnect(conn)) {
        return returnSqlError((SQLHANDLE)hdbc, SQL_HANDLE_DBC, conn->error_msg);
    }

    if (pcbConnStrOut) *pcbConnStrOut = (SQLSMALLINT)connstr.length();
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLConnect(SQLHDBC hdbc, SQLCHAR* szDSN, SQLSMALLINT cbDSN,
    SQLCHAR* szUID, SQLSMALLINT cbUID, SQLCHAR* szAuthStr, SQLSMALLINT cbAuthStr) {

    // DSN-based connection: read from registry
    // For now, return SQL_ERROR — use SQLDriverConnect instead
    return SQL_ERROR;
}

SQLRETURN SQL_API SQLDisconnect(SQLHDBC hdbc) {
    SapConnection* conn = getConnectionHandle((SQLHANDLE)hdbc);
    if (!conn) return SQL_ERROR;
    rfcDisconnect(conn);
    return SQL_SUCCESS;
}

// ============================================================
// Statement Execution
// ============================================================

SQLRETURN SQL_API SQLExecDirect(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLSMALLINT cbSqlStr) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) return SQL_ERROR;

    // Get SQL text
    if (cbSqlStr == SQL_NTS) {
        stmt->sql = std::string((char*)szSqlStr);
    } else {
        stmt->sql = std::string((char*)szSqlStr, cbSqlStr);
    }

    // Execute via RFC
    std::string error;
    if (!rfcExecuteSql(stmt->connection, stmt->sql.c_str(),
                       stmt->columns, stmt->rows, stmt->row_count, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return SQL_ERROR;
    }

    stmt->current_row = 0;
    stmt->executed = true;
    stmt->metadata_mode = "";
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLPrepare(SQLHSTMT hstmt, SQLCHAR* szSqlStr, SQLSMALLINT cbSqlStr) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) return SQL_ERROR;

    if (cbSqlStr == SQL_NTS) {
        stmt->sql = std::string((char*)szSqlStr);
    } else {
        stmt->sql = std::string((char*)szSqlStr, cbSqlStr);
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLExecute(SQLHSTMT hstmt) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) return SQL_ERROR;

    std::string error;
    if (!rfcExecuteSql(stmt->connection, stmt->sql.c_str(),
                       stmt->columns, stmt->rows, stmt->row_count, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return SQL_ERROR;
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
static std::vector<std::string> splitRow(const std::string& row, int num_cols) {
    std::vector<std::string> fields;
    fields.reserve(num_cols);
    std::string current;
    int col = 0;
    for (size_t i = 0; i < row.length() && col < num_cols; i++) {
        if (row[i] == '|') {
            fields.push_back(current);
            current.clear();
            col++;
        } else {
            current += row[i];
        }
    }
    // Last field
    if (col < num_cols) {
        fields.push_back(current);
    }
    // Pad if needed
    while ((int)fields.size() < num_cols) {
        fields.push_back("");
    }
    return fields;
}

SQLRETURN SQL_API SQLFetch(SQLHSTMT hstmt) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;

    // Metadata mode: use meta_rows
    if (!stmt->metadata_mode.empty()) {
        if (stmt->current_row >= (int)stmt->meta_rows.size()) {
            return SQL_NO_DATA;
        }
        stmt->current_row++;
        return SQL_SUCCESS;
    }

    // Normal query mode: use rows (pipe-delimited)
    if (stmt->current_row >= (int)stmt->rows.size()) {
        return SQL_NO_DATA;
    }

    stmt->current_row++;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetData(SQLHSTMT hstmt, SQLUSMALLINT icol,
    SQLSMALLINT fCType, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;

    // Metadata mode: return pre-built string values from meta_rows
    if (!stmt->metadata_mode.empty()) {
        if (icol < 1 || icol > (SQLUSMALLINT)stmt->meta_rows[0].size()) return SQL_ERROR;
        int row_idx = stmt->current_row - 1;
        if (row_idx < 0 || row_idx >= (int)stmt->meta_rows.size()) return SQL_ERROR;
        std::string value = stmt->meta_rows[row_idx][icol - 1];
        if (pcbValue) *pcbValue = (SQLLEN)value.length();
        if (cbValueMax > 0 && rgbValue) {
            strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
        }
        return SQL_SUCCESS;
    }

    // Normal query mode
    if (icol < 1 || icol > (SQLUSMALLINT)stmt->columns.size()) return SQL_ERROR;

    int row_idx = stmt->current_row - 1;
    if (row_idx < 0 || row_idx >= (int)stmt->rows.size()) return SQL_ERROR;

    std::vector<std::string> fields = splitRow(stmt->rows[row_idx], (int)stmt->columns.size());
    std::string value = fields[icol - 1];

    // Return as string (SQL_C_CHAR) — simplest approach
    if (fCType == SQL_C_CHAR || fCType == SQL_C_DEFAULT) {
        if (pcbValue) *pcbValue = (SQLLEN)value.length();
        if (cbValueMax > 0) {
            strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
        }
        return SQL_SUCCESS;
    }

    // Return as integer
    if (fCType == SQL_C_LONG || fCType == SQL_C_SLONG || fCType == SQL_C_ULONG) {
        int val = atoi(value.c_str());
        *(int*)rgbValue = val;
        if (pcbValue) *pcbValue = sizeof(int);
        return SQL_SUCCESS;
    }

    // Return as double
    if (fCType == SQL_C_DOUBLE) {
        double val = atof(value.c_str());
        *(double*)rgbValue = val;
        if (pcbValue) *pcbValue = sizeof(double);
        return SQL_SUCCESS;
    }

    // Default: return as string
    if (pcbValue) *pcbValue = (SQLLEN)value.length();
    if (cbValueMax > 0) {
        strncpy_s((char*)rgbValue, cbValueMax, value.c_str(), cbValueMax - 1);
    }
    return SQL_SUCCESS;
}

// ============================================================
// Result Set — Metadata
// ============================================================

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT hstmt, SQLSMALLINT* pccol) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;
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

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;

    // Metadata mode
    if (!stmt->metadata_mode.empty()) {
        if (icol < 1 || icol > (SQLUSMALLINT)stmt->meta_columns.size()) return SQL_ERROR;
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
    if (icol < 1 || icol > (SQLUSMALLINT)stmt->columns.size()) return SQL_ERROR;

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

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;

    // Metadata mode
    if (!stmt->metadata_mode.empty()) {
        if (iCol < 1 || iCol > (SQLUSMALLINT)stmt->meta_columns.size()) return SQL_ERROR;
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
    if (iCol < 1 || iCol > (SQLUSMALLINT)stmt->columns.size()) return SQL_ERROR;

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
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength) {
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV henv, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength) {
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT DataType) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) return SQL_ERROR;

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

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->connection || !stmt->connection->connected) return SQL_ERROR;

    // Extract table name pattern
    std::string tablePattern = "%";
    if (szTableName) {
        if (cbTableName == SQL_NTS) tablePattern = std::string((char*)szTableName);
        else tablePattern = std::string((char*)szTableName, cbTableName);
    }

    stmt->metadata_mode = "TABLES";
    stmt->meta_table_name = "";
    stmt->current_row = 0;

    std::string error;
    if (!metaGetTables(stmt->connection, tablePattern, stmt->meta_columns, stmt->meta_rows, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return SQL_ERROR;
    }

    stmt->executed = true;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLColumns(SQLHSTMT hstmt, SQLCHAR* szCatalogName, SQLSMALLINT cbCatalogName,
    SQLCHAR* szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR* szTableName, SQLSMALLINT cbTableName,
    SQLCHAR* szColumnName, SQLSMALLINT cbColumnName) {

    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->connection || !stmt->connection->connected) return SQL_ERROR;

    // Extract table name (required for SQLColumns)
    std::string tableName;
    if (szTableName) {
        if (cbTableName == SQL_NTS) tableName = std::string((char*)szTableName);
        else tableName = std::string((char*)szTableName, cbTableName);
    }

    if (tableName.empty() || tableName == "%") {
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, "SQLColumns requires a specific table name");
        return SQL_ERROR;
    }

    stmt->metadata_mode = "COLUMNS";
    stmt->meta_table_name = tableName;
    stmt->current_row = 0;

    std::string error;
    if (!metaGetColumns(stmt->connection, tableName, stmt->meta_columns, stmt->meta_rows, error)) {
        stmt->executed = false;
        setError((SQLHANDLE)hstmt, SQL_HANDLE_STMT, error);
        return SQL_ERROR;
    }

    stmt->executed = true;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
    SQLSMALLINT cbInfoValueMax, SQLSMALLINT* pcbInfoValue) {

    switch (fInfoType) {
        case SQL_DRIVER_NAME:
            if (rgbInfoValue && cbInfoValueMax > 0)
                strncpy_s((char*)rgbInfoValue, cbInfoValueMax, "sapodbcabap.dll", cbInfoValueMax - 1);
            if (pcbInfoValue) *pcbInfoValue = 14;
            return SQL_SUCCESS;
        case SQL_DBMS_NAME:
            if (rgbInfoValue && cbInfoValueMax > 0)
                strncpy_s((char*)rgbInfoValue, cbInfoValueMax, "SAP via Z_EXECUTE_SQL", cbInfoValueMax - 1);
            if (pcbInfoValue) *pcbInfoValue = 21;
            return SQL_SUCCESS;
        case SQL_DBMS_VER:
            if (rgbInfoValue && cbInfoValueMax > 0)
                strncpy_s((char*)rgbInfoValue, cbInfoValueMax, "1.00.0000", cbInfoValueMax - 1);
            if (pcbInfoValue) *pcbInfoValue = 9;
            return SQL_SUCCESS;
        case SQL_DRIVER_ODBC_VER:
            if (rgbInfoValue && cbInfoValueMax > 0)
                strncpy_s((char*)rgbInfoValue, cbInfoValueMax, "03.80", cbInfoValueMax - 1);
            if (pcbInfoValue) *pcbInfoValue = 5;
            return SQL_SUCCESS;
        case SQL_DRIVER_VER:
            if (rgbInfoValue && cbInfoValueMax > 0)
                strncpy_s((char*)rgbInfoValue, cbInfoValueMax, "01.00.0000", cbInfoValueMax - 1);
            if (pcbInfoValue) *pcbInfoValue = 9;
            return SQL_SUCCESS;
        default:
            if (rgbInfoValue) memset(rgbInfoValue, 0, cbInfoValueMax);
            if (pcbInfoValue) *pcbInfoValue = 0;
            return SQL_SUCCESS;
    }
}

SQLRETURN SQL_API SQLGetFunctions(SQLHDBC hdbc, SQLUSMALLINT fFunction, SQLUSMALLINT* pfSupported) {
    if (pfSupported) *pfSupported = SQL_TRUE;
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLRowCount(SQLHSTMT hstmt, SQLLEN* pcrow) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt || !stmt->executed) return SQL_ERROR;
    if (!stmt->metadata_mode.empty()) {
        *pcrow = (SQLLEN)stmt->meta_rows.size();
    } else {
        *pcrow = stmt->row_count;
    }
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLMoreResults(SQLHSTMT hstmt) {
    return SQL_NO_DATA;
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT hstmt) {
    return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT hstmt) {
    SapStatement* stmt = getStatementHandle((SQLHANDLE)hstmt);
    if (!stmt) return SQL_ERROR;
    stmt->current_row = 0;
    stmt->executed = false;
    stmt->metadata_mode = "";
    stmt->columns.clear();
    stmt->rows.clear();
    stmt->meta_columns.clear();
    stmt->meta_rows.clear();
    return SQL_SUCCESS;
}

// Bind parameter / bind col — simplified stubs
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
    SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef, SQLSMALLINT ibScale,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    return SQL_SUCCESS;  // Not fully implemented
}

SQLRETURN SQL_API SQLBindCol(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
    SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN* pcbValue) {
    return SQL_SUCCESS;  // Not fully implemented — use SQLGetData instead
}

// Error handling
SQLRETURN SQL_API SQLError(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    SQLCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {
    // Return "no data" — no error stored
    return SQL_NO_DATA;
}

SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
    SQLCHAR* szSqlState, SQLINTEGER* pfNativeError, SQLCHAR* szErrorMsg,
    SQLSMALLINT cbErrorMsgMax, SQLSMALLINT* pcbErrorMsg) {

    if (RecNumber > 1) return SQL_NO_DATA;

    auto it = g_errors.find(Handle);
    if (it == g_errors.end() || !it->second.has_error) return SQL_NO_DATA;

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
    return SQL_ERROR;
}

SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT hstmt, SQLSMALLINT Operation) {
    return SQL_ERROR;
}

// Column count for catalogs etc
SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value,
    SQLINTEGER BufferLength, SQLINTEGER* StringLength) {
    return SQL_ERROR;
}

// Misc stubs
SQLRETURN SQL_API SQLNativeSql(SQLHDBC hdbc, SQLCHAR* szSqlStrIn, SQLINTEGER cbSqlStrIn,
    SQLCHAR* szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER* pcbSqlStr) {
    // Pass through — we accept native SQL
    if (szSqlStr && cbSqlStrMax > 0) {
        strncpy_s((char*)szSqlStr, cbSqlStrMax, (char*)szSqlStrIn, cbSqlStrMax - 1);
    }
    if (pcbSqlStr) *pcbSqlStr = cbSqlStrIn;
    return SQL_SUCCESS;
}