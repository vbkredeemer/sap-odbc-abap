#pragma once

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <map>

// SAP NWRFC SDK
#include "sapnwrfc.h"

// RFC helper functions (defined in rfc_wrapper.cpp, used by chunked_read.cpp)
std::wstring toSapUc(const std::string& s);
std::string fromSapUc(const SAP_UC* src, int len);

// Convert C string literal to SAP_UC (wide string on Windows/Unicode)
#define TO_SAP_UC(str) ((const SAP_UC*)(L##str))

// Maximum rows per RFC call (matches Java driver's IV_MAX_ROWS)
#define MAX_ROWS_DEFAULT 30000
#define ROW_DATA_SIZE 10000

// Column metadata from ET_FIELDS
struct ColumnMeta {
    char fieldname[31];
    char datatype[11];
    int length;
    int decimals;
    int colpos;
    SQLSMALLINT sql_type;
};

// Connection parameters
struct ConnectionParams {
    std::string host;
    std::string sysnr;
    std::string client;
    std::string user;
    std::string password;
    std::string lang;
    int max_rows;
};

// Error info stored per handle
struct SapErrorInfo {
    std::string sqlstate;
    std::string message;
    int native_error;
    bool has_error;
};

// Connection handle (extends ODBC handle)
struct SapConnection {
    ConnectionParams params;
    RFC_CONNECTION_HANDLE rfc_conn;
    bool connected;
    std::string error_msg;
    SapErrorInfo last_error;
};

// Statement handle
struct SapStatement {
    SapConnection* connection;
    std::vector<ColumnMeta> columns;
    std::vector<std::string> rows;  // pipe-delimited row data
    int current_row;
    int row_count;
    bool executed;
    std::string sql;
    // Metadata mode: "TABLES", "COLUMNS", "TYPES", "TYPEINFO", or "" for normal queries
    std::string metadata_mode;
    // For SQLColumns: which table's columns to describe
    std::string meta_table_name;
    // Metadata result columns (different from query columns)
    std::vector<ColumnMeta> meta_columns;
    // Metadata result rows (pre-built, not pipe-delimited)
    std::vector<std::vector<std::string>> meta_rows;
};

// ODBC handle types
#define SQL_HANDLE_SAP_CONNECTION 100
#define SQL_HANDLE_SAP_STATEMENT  101

// Helper: get connection/statement from ODBC handle
SapConnection* getConnectionHandle(SQLHANDLE h);
SapStatement* getStatementHandle(SQLHANDLE h);

// Error storage helpers
void setError(SQLHANDLE handle, SQLSMALLINT handleType, const std::string& msg, const char* state = "HY000");
void clearError(SQLHANDLE handle, SQLSMALLINT handleType);

// RFC wrapper functions
bool rfcConnect(SapConnection* conn);
void rfcDisconnect(SapConnection* conn);
bool rfcExecuteSql(SapConnection* conn, const char* sql, 
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::string>& rows,
                   int& row_count,
                   std::string& error);

// Metadata functions (query DD02V/DD03VT via Z_EXECUTE_SQL)
bool metaGetTables(SapConnection* conn, const std::string& tablePattern,
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::vector<std::string>>& rows,
                   std::string& error);
bool metaGetColumns(SapConnection* conn, const std::string& tableName,
                    std::vector<ColumnMeta>& columns,
                    std::vector<std::vector<std::string>>& rows,
                    std::string& error);

// Chunked table read via Z_READ_TABLE
// Automatically loops with ROWSKIPS/ROWCOUNT until all data is fetched
bool rfcReadTableChunked(SapConnection* conn,
                         const std::string& table,
                         const std::string& whereClause,
                         const std::string& fields,
                         const std::string& orderBy,
                         int chunkSize,
                         std::vector<ColumnMeta>& columns,
                         std::vector<std::string>& rows,
                         int& total_row_count,
                         std::string& error);

// SQL parser: detect if query is a simple table read (no JOIN/GROUP BY/etc.)
bool isSimpleTableRead(const std::string& sql, std::string& table, 
                       std::string& whereClause, std::string& fields);

// Config parsing
ConnectionParams parseConnectionString(const std::string& connstr);
SQLRETURN returnSqlError(SQLHANDLE handle, SQLSMALLINT handleType, 
                         const std::string& message, const char* state = "HY000");