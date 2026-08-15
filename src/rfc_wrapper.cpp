#include "sap_odbc.h"
#include <sstream>

// Convert std::string (UTF-8) to SAP_UC (wchar_t on Windows) — helper
// SAP_UC is wchar_t on Windows (UTF-16), char16_t on some platforms
std::wstring toSapUc(const std::string& s) {
    if (s.empty()) return std::wstring();
    // Convert UTF-8 → UTF-16 using MultiByteToWideChar
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), NULL, 0);
    if (wlen <= 0) return std::wstring();
    std::wstring ws(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), &ws[0], wlen);
    return ws;
}

// Convert SAP_UC to std::string (UTF-8) — helper
std::string fromSapUc(const SAP_UC* src, int len) {
    if (!src || len <= 0) return "";
    // Find actual length (stop at null terminator if within len)
    int actualLen = 0;
    for (int i = 0; i < len; i++) {
        if (src[i] == 0) break;
        actualLen++;
    }
    if (actualLen == 0) return "";
    // Convert UTF-16 → UTF-8 using WideCharToMultiByte
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, src, actualLen, NULL, 0, NULL, NULL);
    if (utf8Len <= 0) return "";
    std::string s(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, src, actualLen, &s[0], utf8Len, NULL, NULL);
    return s;
}

bool rfcConnect(SapConnection* conn) {
    if (!conn) return false;
    RFC_ERROR_INFO error_info;

    std::wstring host = toSapUc(conn->params.host);
    std::wstring sysnr = toSapUc(conn->params.sysnr);
    std::wstring client = toSapUc(conn->params.client);
    std::wstring user = toSapUc(conn->params.user);
    std::wstring passwd = toSapUc(conn->params.password);
    std::wstring lang = toSapUc(conn->params.lang);

    RFC_CONNECTION_PARAMETER params[6];
    params[0].name = TO_SAP_UC("ASHOST");  params[0].value = host.c_str();
    params[1].name = TO_SAP_UC("SYSNR");   params[1].value = sysnr.c_str();
    params[2].name = TO_SAP_UC("CLIENT");  params[2].value = client.c_str();
    params[3].name = TO_SAP_UC("USER");    params[3].value = user.c_str();
    params[4].name = TO_SAP_UC("PASSWD");  params[4].value = passwd.c_str();
    params[5].name = TO_SAP_UC("LANG");    params[5].value = lang.c_str();

    conn->rfc_conn = RfcOpenConnection(params, 6, &error_info);
    if (conn->rfc_conn == NULL) {
        conn->error_msg = "RFC connection failed: " + fromSapUc(error_info.message, 256);
        conn->connected = false;
        return false;
    }
    conn->connected = true;
    return true;
}

void rfcDisconnect(SapConnection* conn) {
    if (!conn) return;
    if (conn->connected && conn->rfc_conn != NULL) {
        RfcCloseConnection(conn->rfc_conn, NULL);
        conn->rfc_conn = NULL;
        conn->connected = false;
    }
}

bool rfcExecuteSql(SapConnection* conn, const char* sql,
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::string>& rows,
                   int& row_count,
                   std::string& error) {
    if (!conn) {
        error = "Connection handle is NULL";
        return false;
    }
    if (!conn->connected || conn->rfc_conn == NULL) {
        error = "Not connected";
        return false;
    }

    RFC_ERROR_INFO error_info;

    // Get function description first, then create function
    RFC_FUNCTION_DESC_HANDLE func_desc = RfcGetFunctionDesc(conn->rfc_conn, TO_SAP_UC("Z_EXECUTE_SQL"), &error_info);
    if (func_desc == NULL) {
        error = "Cannot get function desc Z_EXECUTE_SQL: " + fromSapUc(error_info.message, 256);
        return false;
    }

    RFC_FUNCTION_HANDLE func = RfcCreateFunction(func_desc, &error_info);
    if (func == NULL) {
        error = "Cannot create RFC function Z_EXECUTE_SQL: " + fromSapUc(error_info.message, 256);
        RfcDestroyFunctionDesc(func_desc, NULL);
        return false;
    }

    // C-1 fix: func_desc is no longer needed once the function handle is created
    RfcDestroyFunctionDesc(func_desc, NULL);

    // Set IV_SQL
    std::wstring wsql = toSapUc(std::string(sql));
    RFC_RC rc = RfcSetString(func, TO_SAP_UC("IV_SQL"), wsql.c_str(), (unsigned)wsql.length(), &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_SQL";
        RfcDestroyFunction(func, NULL);
        return false;
    }

    // Set IV_MAX_ROWS
    rc = RfcSetInt(func, TO_SAP_UC("IV_MAX_ROWS"), conn->params.max_rows, &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_MAX_ROWS";
        RfcDestroyFunction(func, NULL);
        return false;
    }

    // Set IV_READONLY = 'X'
    rc = RfcSetString(func, TO_SAP_UC("IV_READONLY"), TO_SAP_UC("X"), 1, &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_READONLY";
        RfcDestroyFunction(func, NULL);
        return false;
    }

    // Execute
    rc = RfcInvoke(conn->rfc_conn, func, &error_info);
    if (rc != RFC_OK) {
        error = "RFC invoke failed: " + fromSapUc(error_info.message, 256);
        RfcDestroyFunction(func, NULL);
        return false;
    }

    // Get EV_ROW_COUNT
    int ev_row_count = 0;
    RfcGetInt(func, TO_SAP_UC("EV_ROW_COUNT"), &ev_row_count, &error_info);
    row_count = ev_row_count;

    // Check for errors
    std::vector<SAP_UC> ev_error(4096);
    unsigned ev_error_len = 0;
    rc = RfcGetString(func, TO_SAP_UC("EV_ERROR"), ev_error.data(), 4096, &ev_error_len, &error_info);
    if (rc == RFC_OK && ev_error_len > 0) {
        std::string err_str = fromSapUc(ev_error.data(), ev_error_len);
        if (!err_str.empty()) {
            error = "SAP error: " + err_str;
            RfcDestroyFunction(func, NULL);
            return false;
        }
    }

    // Read ET_FIELDS (table of ZSQL_FIELD)
    RFC_TABLE_HANDLE fields_table = NULL;
    rc = RfcGetTable(func, TO_SAP_UC("ET_FIELDS"), &fields_table, &error_info);
    if (rc != RFC_OK || fields_table == NULL) {
        error = "Cannot get ET_FIELDS";
        RfcDestroyFunction(func, NULL);
        return false;
    }

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
        // Trim trailing spaces (FIELDNAME is CHAR 30, blank-padded)
        while (!fname.empty() && fname.back() == ' ') fname.pop_back();
        strncpy_s(col.fieldname, sizeof(col.fieldname), fname.c_str(), sizeof(col.fieldname) - 1);

        RfcGetString(row, TO_SAP_UC("DATATYPE"), buf, 256, &len, &error_info);
        std::string dtype = fromSapUc(buf, len);
        // Trim trailing spaces
        while (!dtype.empty() && dtype.back() == ' ') dtype.pop_back();
        strncpy_s(col.datatype, sizeof(col.datatype), dtype.c_str(), sizeof(col.datatype) - 1);

        RfcGetInt(row, TO_SAP_UC("LENGTH"), &col.length, &error_info);
        RfcGetInt(row, TO_SAP_UC("DECIMALS"), &col.decimals, &error_info);
        RfcGetInt(row, TO_SAP_UC("COLPOS"), &col.colpos, &error_info);

        // Map ABAP type to ODBC SQL type
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

    // Read ET_DATA (table of ZSQL_ROW with field ROWDATA CHAR 10000)
    RFC_TABLE_HANDLE data_table = NULL;
    rc = RfcGetTable(func, TO_SAP_UC("ET_DATA"), &data_table, &error_info);
    if (rc != RFC_OK || data_table == NULL) {
        error = "Cannot get ET_DATA";
        RfcDestroyFunction(func, NULL);
        return false;
    }

    unsigned data_count = 0;
    RfcGetRowCount(data_table, &data_count, &error_info);
    rows.clear();
    rows.reserve(data_count);

    for (unsigned i = 0; i < data_count; i++) {
        RfcMoveTo(data_table, i, &error_info);
        RFC_STRUCTURE_HANDLE row = RfcGetCurrentRow(data_table, &error_info);

        SAP_UC rowdata[ROW_DATA_SIZE + 1];
        unsigned rowdata_len = 0;
        // Pass bufferLength = ROW_DATA_SIZE + 1 to accommodate full CHAR(10000) + null terminator
        RfcGetString(row, TO_SAP_UC("ROWDATA"), rowdata, ROW_DATA_SIZE + 1, &rowdata_len, &error_info);
        std::string rowStr = fromSapUc(rowdata, rowdata_len);
        // Remove trailing whitespace (spaces from CHAR field padding)
        while (!rowStr.empty() && (rowStr.back() == ' ' || rowStr.back() == '\t' || rowStr.back() == '\r' || rowStr.back() == '\n')) {
            rowStr.pop_back();
        }
        rows.push_back(rowStr);
    }

    RfcDestroyFunction(func, NULL);
    return true;
}