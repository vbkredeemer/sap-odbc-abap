#include "sap_odbc.h"
#include <algorithm>
#include <sstream>
#include <cctype>

// ============================================================
// SQL Parser: detect if query is a simple table read
// Returns true if: SELECT [fields] FROM table [WHERE ...]
// Returns false if: JOIN, GROUP BY, ORDER BY, DISTINCT, UNION, subquery, etc.
// ============================================================

// Uppercase a string
static std::string toUpper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Trim whitespace
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Check if a keyword exists as a whole word in the SQL
static bool containsKeyword(const std::string& sql_upper, const std::string& keyword) {
    size_t pos = 0;
    while ((pos = sql_upper.find(keyword, pos)) != std::string::npos) {
        // Check word boundaries
        bool left_ok = (pos == 0 || !isalpha(sql_upper[pos - 1]));
        bool right_ok = (pos + keyword.length() >= sql_upper.length() || !isalpha(sql_upper[pos + keyword.length()]));
        if (left_ok && right_ok) return true;
        pos += keyword.length();
    }
    return false;
}

bool isSimpleTableRead(const std::string& sql, std::string& table, 
                       std::string& whereClause, std::string& fields) {
    std::string sql_upper = toUpper(sql);
    std::string sql_trim = trim(sql);
    
    // Must start with SELECT
    if (sql_upper.substr(0, 6) != "SELECT") return false;
    
    // Must NOT contain these keywords
    if (containsKeyword(sql_upper, "JOIN")) return false;
    if (containsKeyword(sql_upper, "GROUP BY")) return false;
    if (containsKeyword(sql_upper, "DISTINCT")) return false;
    if (containsKeyword(sql_upper, "UNION")) return false;
    if (containsKeyword(sql_upper, "HAVING")) return false;
    if (containsKeyword(sql_upper, "OFFSET")) return false;
    if (containsKeyword(sql_upper, "FETCH")) return false;
    if (containsKeyword(sql_upper, "LIMIT")) return false;
    if (containsKeyword(sql_upper, "CASE")) return false;
    if (containsKeyword(sql_upper, "COUNT")) return false;
    if (containsKeyword(sql_upper, "SUM")) return false;
    if (containsKeyword(sql_upper, "MIN")) return false;
    if (containsKeyword(sql_upper, "MAX")) return false;
    if (containsKeyword(sql_upper, "AVG")) return false;
    if (containsKeyword(sql_upper, "AS ")) return false;
    
    // Check for subquery (parentheses in FROM clause)
    size_t from_pos = sql_upper.find(" FROM ");
    if (from_pos == std::string::npos) return false;
    
    // Check for parentheses (subqueries, function calls)
    if (sql.find('(') != std::string::npos) return false;
    
    // Extract fields (between SELECT and FROM)
    fields = trim(sql_trim.substr(6, from_pos - 6));
    
    // Find WHERE or end of string
    size_t where_pos = sql_upper.find(" WHERE ");
    size_t order_pos = sql_upper.find(" ORDER BY ");
    
    // Table name is between FROM and WHERE/ORDER BY/end
    size_t table_start = from_pos + 6; // length of " FROM "
    size_t table_end;
    
    if (where_pos != std::string::npos) {
        table_end = where_pos;
    } else if (order_pos != std::string::npos) {
        table_end = order_pos;
    } else {
        table_end = sql_trim.length();
    }
    
    table = trim(sql_trim.substr(table_start, table_end - table_start));
    
    // Extract WHERE clause if present
    if (where_pos != std::string::npos) {
        size_t where_start = where_pos + 7; // length of " WHERE "
        size_t where_end;
        if (order_pos != std::string::npos && order_pos > where_pos) {
            where_end = order_pos;
        } else {
            where_end = sql_trim.length();
        }
        whereClause = trim(sql_trim.substr(where_start, where_end - where_start));
        // Remove trailing semicolon
        if (!whereClause.empty() && whereClause.back() == ';') {
            whereClause.pop_back();
        }
    } else {
        whereClause = "";
    }
    
    // Validate: table must be a single word (no commas = no multiple tables)
    if (table.find(',') != std::string::npos) return false;
    if (table.find(' ') != std::string::npos) return false;
    
    // Must have a table name
    if (table.empty()) return false;
    
    return true;
}

// ============================================================
// Chunked table read via Z_READ_TABLE
// Calls Z_READ_TABLE repeatedly with increasing ROWSKIPS
// ============================================================

bool rfcReadTableChunked(SapConnection* conn,
                         const std::string& table,
                         const std::string& whereClause,
                         const std::string& fields,
                         const std::string& orderBy,
                         int chunkSize,
                         std::vector<ColumnMeta>& columns,
                         std::vector<std::string>& rows,
                         int& total_row_count,
                         std::string& error) {
    if (!conn->connected) {
        error = "Not connected";
        return false;
    }

    total_row_count = 0;
    rows.clear();
    columns.clear();

    int skip = 0;
    bool has_more = true;
    bool first_chunk = true;
    int max_iterations = 500; // safety limit (500 * 10000 = 5M rows)

    while (has_more && max_iterations > 0) {
        max_iterations--;

        RFC_ERROR_INFO error_info;

        // Get function description
        RFC_FUNCTION_DESC_HANDLE func_desc = RfcGetFunctionDesc(conn->rfc_conn, TO_SAP_UC("Z_READ_TABLE"), &error_info);
        if (func_desc == NULL) {
            error = "Cannot get function desc Z_READ_TABLE: " + fromSapUc(error_info.message, 256);
            return false;
        }

        RFC_FUNCTION_HANDLE func = RfcCreateFunction(func_desc, &error_info);
        if (func == NULL) {
            error = "Cannot create RFC function Z_READ_TABLE: " + fromSapUc(error_info.message, 256);
            return false;
        }

        RFC_RC rc;

        // Set IV_TABLE
        std::wstring wtable = toSapUc(table);
        rc = RfcSetString(func, TO_SAP_UC("IV_TABLE"), wtable.c_str(), (unsigned)wtable.length(), &error_info);
        if (rc != RFC_OK) { error = "Cannot set IV_TABLE"; RfcDestroyFunction(func, NULL); return false; }

        // Set IV_WHERE (optional)
        if (!whereClause.empty()) {
            std::wstring wwhere = toSapUc(whereClause);
            rc = RfcSetString(func, TO_SAP_UC("IV_WHERE"), wwhere.c_str(), (unsigned)wwhere.length(), &error_info);
            if (rc != RFC_OK) { error = "Cannot set IV_WHERE"; RfcDestroyFunction(func, NULL); return false; }
        }

        // Set IV_FIELDS (optional, default '*')
        if (!fields.empty()) {
            std::wstring wfields = toSapUc(fields);
            rc = RfcSetString(func, TO_SAP_UC("IV_FIELDS"), wfields.c_str(), (unsigned)wfields.length(), &error_info);
            if (rc != RFC_OK) { error = "Cannot set IV_FIELDS"; RfcDestroyFunction(func, NULL); return false; }
        }

        // Set IV_ORDERBY (optional)
        if (!orderBy.empty()) {
            std::wstring worder = toSapUc(orderBy);
            rc = RfcSetString(func, TO_SAP_UC("IV_ORDERBY"), worder.c_str(), (unsigned)worder.length(), &error_info);
            if (rc != RFC_OK) { error = "Cannot set IV_ORDERBY"; RfcDestroyFunction(func, NULL); return false; }
        }

        // Set IV_ROWSKIPS
        rc = RfcSetInt(func, TO_SAP_UC("IV_ROWSKIPS"), skip, &error_info);
        if (rc != RFC_OK) { error = "Cannot set IV_ROWSKIPS"; RfcDestroyFunction(func, NULL); return false; }

        // Set IV_ROWCOUNT
        rc = RfcSetInt(func, TO_SAP_UC("IV_ROWCOUNT"), chunkSize, &error_info);
        if (rc != RFC_OK) { error = "Cannot set IV_ROWCOUNT"; RfcDestroyFunction(func, NULL); return false; }

        // Execute
        rc = RfcInvoke(conn->rfc_conn, func, &error_info);
        if (rc != RFC_OK) {
            error = "RFC invoke Z_READ_TABLE failed: " + fromSapUc(error_info.message, 256);
            RfcDestroyFunction(func, NULL);
            return false;
        }

        // Check for errors
        SAP_UC ev_error[4096];
        unsigned ev_error_len = 0;
        rc = RfcGetString(func, TO_SAP_UC("EV_ERROR"), ev_error, 4096, &ev_error_len, &error_info);
        if (rc == RFC_OK && ev_error_len > 0) {
            std::string err_str = fromSapUc(ev_error, ev_error_len);
            if (!err_str.empty()) {
                error = "SAP error: " + err_str;
                RfcDestroyFunction(func, NULL);
                return false;
            }
        }

        // Check has_more
        SAP_UC has_more_buf[2];
        unsigned has_more_len = 0;
        rc = RfcGetString(func, TO_SAP_UC("EV_HAS_MORE"), has_more_buf, 2, &has_more_len, &error_info);
        if (rc == RFC_OK && has_more_len > 0 && has_more_buf[0] == 'X') {
            has_more = true;
        } else {
            has_more = false;
        }

        // Get row count
        int row_count = 0;
        RfcGetInt(func, TO_SAP_UC("EV_ROW_COUNT"), &row_count, &error_info);

        // On first chunk, read ET_FIELDS for column metadata
        if (first_chunk) {
            RFC_TABLE_HANDLE fields_table = NULL;
            rc = RfcGetTable(func, TO_SAP_UC("ET_FIELDS"), &fields_table, &error_info);
            if (rc == RFC_OK && fields_table != NULL) {
                unsigned field_count = 0;
                RfcGetRowCount(fields_table, &field_count, &error_info);
                columns.clear();
                columns.reserve(field_count);

                for (unsigned i = 0; i < field_count; i++) {
                    RfcMoveTo(fields_table, i, &error_info);
                    RFC_STRUCTURE_HANDLE row = RfcGetCurrentRow(fields_table, &error_info);

                    ColumnMeta col;
                    memset(&col, 0, sizeof(col));

                    SAP_UC buf[256];
                    unsigned len = 0;

                    RfcGetString(row, TO_SAP_UC("FIELDNAME"), buf, 256, &len, &error_info);
                    std::string fname = fromSapUc(buf, len);
                    strncpy_s(col.fieldname, sizeof(col.fieldname), fname.c_str(), sizeof(col.fieldname) - 1);

                    RfcGetString(row, TO_SAP_UC("DATATYPE"), buf, 256, &len, &error_info);
                    std::string dtype = fromSapUc(buf, len);
                    strncpy_s(col.datatype, sizeof(col.datatype), dtype.c_str(), sizeof(col.datatype) - 1);

                    RfcGetInt(row, TO_SAP_UC("LENGTH"), &col.length, &error_info);
                    RfcGetInt(row, TO_SAP_UC("DECIMALS"), &col.decimals, &error_info);
                    RfcGetInt(row, TO_SAP_UC("COLPOS"), &col.colpos, &error_info);

                    // Map ABAP type to ODBC SQL type (same as rfcExecuteSql)
                    if (strcmp(col.datatype, "C") == 0 || strcmp(col.datatype, "CHAR") == 0)
                        col.sql_type = SQL_VARCHAR;
                    else if (strcmp(col.datatype, "N") == 0)
                        col.sql_type = SQL_VARCHAR;
                    else if (strcmp(col.datatype, "I") == 0 || strcmp(col.datatype, "INT4") == 0)
                        col.sql_type = SQL_INTEGER;
                    else if (strcmp(col.datatype, "INT2") == 0)
                        col.sql_type = SQL_SMALLINT;
                    else if (strcmp(col.datatype, "INT1") == 0)
                        col.sql_type = SQL_TINYINT;
                    else if (strcmp(col.datatype, "P") == 0 || strcmp(col.datatype, "PACKED") == 0)
                        col.sql_type = SQL_DECIMAL;
                    else if (strcmp(col.datatype, "F") == 0 || strcmp(col.datatype, "FLOAT") == 0)
                        col.sql_type = SQL_DOUBLE;
                    else if (strcmp(col.datatype, "D") == 0 || strcmp(col.datatype, "DATE") == 0)
                        col.sql_type = SQL_TYPE_DATE;
                    else if (strcmp(col.datatype, "T") == 0 || strcmp(col.datatype, "TIME") == 0)
                        col.sql_type = SQL_TYPE_TIME;
                    else if (strcmp(col.datatype, "X") == 0 || strcmp(col.datatype, "RAW") == 0)
                        col.sql_type = SQL_VARBINARY;
                    else if (strcmp(col.datatype, "STRING") == 0)
                        col.sql_type = SQL_WVARCHAR;
                    else
                        col.sql_type = SQL_VARCHAR;

                    columns.push_back(col);
                }
            }
            first_chunk = false;
        }

        // Read ET_DATA for this chunk
        RFC_TABLE_HANDLE data_table = NULL;
        rc = RfcGetTable(func, TO_SAP_UC("ET_DATA"), &data_table, &error_info);
        if (rc == RFC_OK && data_table != NULL) {
            unsigned data_count = 0;
            RfcGetRowCount(data_table, &data_count, &error_info);

            for (unsigned i = 0; i < data_count; i++) {
                RfcMoveTo(data_table, i, &error_info);
                RFC_STRUCTURE_HANDLE row = RfcGetCurrentRow(data_table, &error_info);

                SAP_UC rowdata[ROW_DATA_SIZE + 1];
                unsigned rowdata_len = 0;
                RfcGetString(row, TO_SAP_UC("ROWDATA"), rowdata, ROW_DATA_SIZE, &rowdata_len, &error_info);
                rows.push_back(fromSapUc(rowdata, rowdata_len));
            }
        }

        RfcDestroyFunction(func, NULL);

        // Advance skip position
        skip += chunkSize;
        total_row_count += row_count;

        // If we got fewer rows than chunkSize, we're done
        if (row_count < chunkSize) {
            has_more = false;
        }
    }

    return true;
}