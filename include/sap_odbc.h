#pragma once

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <map>

// SAP NWRFC SDK
#include "sapnwrfc.h"

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

// Connection handle (extends ODBC handle)
struct SapConnection {
    ConnectionParams params;
    RFC_CONNECTION_HANDLE rfc_conn;
    bool connected;
    std::string error_msg;
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
};

// ODBC handle types
#define SQL_HANDLE_SAP_CONNECTION 100
#define SQL_HANDLE_SAP_STATEMENT  101

// Helper: get connection/statement from ODBC handle
SapConnection* getConnectionHandle(SQLHANDLE h);
SapStatement* getStatementHandle(SQLHANDLE h);

// RFC wrapper functions
bool rfcConnect(SapConnection* conn);
void rfcDisconnect(SapConnection* conn);
bool rfcExecuteSql(SapConnection* conn, const char* sql, 
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::string>& rows,
                   int& row_count,
                   std::string& error);

// Config parsing
ConnectionParams parseConnectionString(const std::string& connstr);
SQLRETURN returnSqlError(SQLHANDLE handle, SQLSMALLINT handleType, 
                         const std::string& message, const char* state = "HY000");