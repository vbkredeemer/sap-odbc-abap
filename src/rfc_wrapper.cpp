#include "sap_odbc.h"

// RFC connection parameters
static RFC_CONNECTION_PARAMETER_V3 createConnParams(const ConnectionParams& p) {
    RFC_CONNECTION_PARAMETER_V3 params[6];
    params[0].name = CU8("ASHOST");   params[0].value = CU8(p.host.c_str());
    params[1].name = CU8("SYSNR");    params[1].value = CU8(p.sysnr.c_str());
    params[2].name = CU8("CLIENT");   params[2].value = CU8(p.client.c_str());
    params[3].name = CU8("USER");     params[3].value = CU8(p.user.c_str());
    params[4].name = CU8("PASSWD");   params[4].value = CU8(p.password.c_str());
    params[5].name = CU8("LANG");     params[5].value = CU8(p.lang.c_str());
    return params;
}

bool rfcConnect(SapConnection* conn) {
    RFC_ERROR_INFO error_info;
    RFC_CONNECTION_PARAMETER_V3 params = createConnParams(conn->params);
    
    conn->rfc_conn = RfcOpenConnectionV3(&params, 6, &error_info);
    if (conn->rfc_conn == NULL) {
        conn->error_msg = "RFC connection failed: ";
        conn->error_msg += error_info.message;
        conn->connected = false;
        return false;
    }
    conn->connected = true;
    return true;
}

void rfcDisconnect(SapConnection* conn) {
    if (conn->connected && conn->rfc_conn != NULL) {
        RfcCloseConnection(conn->rfc_conn, NULL, NULL);
        conn->rfc_conn = NULL;
        conn->connected = false;
    }
}

bool rfcExecuteSql(SapConnection* conn, const char* sql,
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::string>& rows,
                   int& row_count,
                   std::string& error) {
    if (!conn->connected) {
        error = "Not connected";
        return false;
    }

    RFC_ERROR_INFO error_info;
    RFC_FUNCTION_HANDLE func = RfcCreateFunction(conn->rfc_conn, CU8("Z_EXECUTE_SQL"), &error_info);
    if (func == NULL) {
        error = "Cannot create RFC function Z_EXECUTE_SQL: ";
        error += error_info.message;
        return false;
    }

    // Set IV_SQL
    RFC_RC rc = RfcSetString(func, CU8("IV_SQL"), CU8(sql), strlen(sql), &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_SQL: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    // Set IV_MAX_ROWS
    int max_rows = conn->params.max_rows;
    rc = RfcSetInt(func, CU8("IV_MAX_ROWS"), max_rows, &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_MAX_ROWS: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    // Set IV_READONLY = 'X'
    rc = RfcSetString(func, CU8("IV_READONLY"), CU8("X"), 1, &error_info);
    if (rc != RFC_OK) {
        error = "Cannot set IV_READONLY: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    // Execute
    rc = RfcInvoke(conn->rfc_conn, func, &error_info);
    if (rc != RFC_OK) {
        error = "RFC invoke failed: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    // Get EV_ROW_COUNT
    int ev_row_count = 0;
    RfcGetInt(func, CU8("EV_ROW_COUNT"), &ev_row_count, &error_info);
    row_count = ev_row_count;

    // Check for errors
    SAP_UC ev_error[4096];
    unsigned ev_error_len = 0;
    rc = RfcGetString(func, CU8("EV_ERROR"), ev_error, 4096, &ev_error_len, &error_info);
    if (rc == RFC_OK && ev_error_len > 0) {
        // Convert to char
        char error_str[4096];
        RfcUTF8SFromSAPUC(error_str, 4096, ev_error, ev_error_len, NULL, NULL);
        if (strlen(error_str) > 0) {
            error = "SAP error: ";
            error += error_str;
            RfcDestroyFunction(func, NULL, NULL);
            return false;
        }
    }

    // Read ET_FIELDS (table of ZSQL_FIELD)
    RFC_TABLE_HANDLE fields_table = RfcGetTable(func, CU8("ET_FIELDS"), &error_info);
    if (fields_table == NULL) {
        error = "Cannot get ET_FIELDS: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    int field_count = RfcGetRowCount(fields_table, NULL, NULL);
    columns.clear();
    columns.reserve(field_count);

    for (int i = 0; i < field_count; i++) {
        RfcMoveTo(fields_table, i, NULL, NULL);
        RFC_STRUCTURE_HANDLE row = RfcGetCurrentRow(fields_table, NULL, NULL);

        ColumnMeta col;
        memset(&col, 0, sizeof(col));

        SAP_UC buf[256];
        unsigned len = 0;

        RfcGetString(row, CU8("FIELDNAME"), buf, 256, &len, &error_info);
        RfcUTF8SFromSAPUC(col.fieldname, sizeof(col.fieldname), buf, len, NULL, NULL);

        RfcGetString(row, CU8("DATATYPE"), buf, 256, &len, &error_info);
        RfcUTF8SFromSAPUC(col.datatype, sizeof(col.datatype), buf, len, NULL, NULL);

        RfcGetInt(row, CU8("LENGTH"), &col.length, &error_info);
        RfcGetInt(row, CU8("DECIMALS"), &col.decimals, &error_info);
        RfcGetInt(row, CU8("COLPOS"), &col.colpos, &error_info);

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
    RFC_TABLE_HANDLE data_table = RfcGetTable(func, CU8("ET_DATA"), &error_info);
    if (data_table == NULL) {
        error = "Cannot get ET_DATA: ";
        error += error_info.message;
        RfcDestroyFunction(func, NULL, NULL);
        return false;
    }

    int data_count = RfcGetRowCount(data_table, NULL, NULL);
    rows.clear();
    rows.reserve(data_count);

    char row_buf[ROW_DATA_SIZE + 1];
    for (int i = 0; i < data_count; i++) {
        RfcMoveTo(data_table, i, NULL, NULL);
        RFC_STRUCTURE_HANDLE row = RfcGetCurrentRow(data_table, NULL, NULL);

        SAP_UC rowdata[ROW_DATA_SIZE + 1];
        unsigned rowdata_len = 0;
        RfcGetString(row, CU8("ROWDATA"), rowdata, ROW_DATA_SIZE, &rowdata_len, &error_info);
        RfcUTF8SFromSAPUC(row_buf, sizeof(row_buf), rowdata, rowdata_len, NULL, NULL);
        rows.push_back(std::string(row_buf));
    }

    RfcDestroyFunction(func, NULL, NULL);
    return true;
}