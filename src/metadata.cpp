#include "sap_odbc.h"
#include <algorithm>
#include <cctype>

// Helper: escape single quotes in SQL string literals to prevent SQL injection
static std::string escapeSql(const std::string& s) {
    std::string r;
    r.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '\'') r += "''";
        else r += c;
    }
    return r;
}

// Helper: convert connection language code (e.g. "DE", "EN") to SAP single-char code (e.g. "D", "E")
static std::string sapLangCode(const std::string& lang) {
    if (!lang.empty()) {
        // I-9 fix: uppercase the first character so lowercase "de" maps to "D"
        return std::string(1, (char)toupper((unsigned char)lang[0]));
    }
    return "E"; // default English
}

// Helper: build a ColumnMeta for metadata result sets
static ColumnMeta makeMetaCol(const char* name, SQLSMALLINT sql_type, int length) {
    ColumnMeta col;
    memset(&col, 0, sizeof(col));
    strncpy_s(col.fieldname, sizeof(col.fieldname), name, sizeof(col.fieldname) - 1);
    strncpy_s(col.datatype, sizeof(col.datatype), "C", 1);
    col.sql_type = sql_type;
    col.length = length;
    col.decimals = 0;
    col.colpos = 0;
    return col;
}

// Helper: convert ODBC pattern (% → *) — actually we pass through to SQL LIKE
// DBeaver sends "%" for all, or "MARA" for specific, or "MA%" for prefix
static std::string patternToSql(const std::string& pattern) {
    if (pattern.empty() || pattern == "%") return "";
    return pattern;
}

// SQLTables result columns (JDBC standard):
// TABLE_CAT, TABLE_SCHEM, TABLE_NAME, TABLE_TYPE, REMARKS
bool metaGetTables(SapConnection* conn, const std::string& tablePattern,
                   std::vector<ColumnMeta>& columns,
                   std::vector<std::vector<std::string>>& rows,
                   std::string& error) {

    if (!conn || !conn->connected) {
        error = "Not connected";
        return false;
    }

    // Build SQL — same as JDBC driver
    std::string sapLang = sapLangCode(conn->params.lang);
    std::string sql = "select TABNAME, DDTEXT from DD02V where DDLANGUAGE = '" + sapLang + "' and TABCLASS = 'TRANSP' and (CONTFLAG = 'A' or CONTFLAG = 'C')";

    // Add table name filter if specific pattern
    std::string pat = patternToSql(tablePattern);
    if (!pat.empty()) {
        // Replace * with % for SQL LIKE
        std::string likepat = pat;
        std::replace(likepat.begin(), likepat.end(), '*', '%');
        sql += " and TABNAME like '" + escapeSql(likepat) + "'";
    }

    // Execute via RFC
    std::vector<ColumnMeta> rawcols;
    std::vector<std::string> rawrows;
    int row_count = 0;

    if (!rfcExecuteSql(conn, sql.c_str(), rawcols, rawrows, row_count, error)) {
        return false;
    }

    // Build JDBC-compatible result columns
    columns.clear();
    columns.push_back(makeMetaCol("TABLE_CAT", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("TABLE_SCHEM", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("TABLE_NAME", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("TABLE_TYPE", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("REMARKS", SQL_VARCHAR, 255));

    // Parse pipe-delimited rows and build metadata rows
    rows.clear();
    for (const auto& rawrow : rawrows) {
        // Split by pipe — rawcols has 2 columns: TABNAME, DDTEXT
        std::string tabname, ddtext;
        size_t pipe = rawrow.find('|');
        if (pipe != std::string::npos) {
            tabname = rawrow.substr(0, pipe);
            ddtext = rawrow.substr(pipe + 1);
        } else {
            tabname = rawrow;
        }

        std::vector<std::string> row;
        row.push_back("SAPHANADB");           // TABLE_CAT
        row.push_back("SAPHANADB");           // TABLE_SCHEM
        row.push_back(tabname);               // TABLE_NAME
        row.push_back("TABLE");               // TABLE_TYPE
        row.push_back(ddtext);                // REMARKS
        rows.push_back(row);
    }

    return true;
}

// SQLColumns result columns (JDBC standard):
// TABLE_CAT, TABLE_SCHEM, TABLE_NAME, COLUMN_NAME, DATA_TYPE, TYPE_NAME,
// COLUMN_SIZE, BUFFER_LENGTH, DECIMAL_DIGITS, NUM_PREC_RADIX, NULLABLE,
// REMARKS, COLUMN_DEF, SQL_DATA_TYPE, SQL_DATETIME_SUB, CHAR_OCTET_LENGTH,
// ORDINAL_POSITION, IS_NULLABLE
bool metaGetColumns(SapConnection* conn, const std::string& tableName,
                    std::vector<ColumnMeta>& columns,
                    std::vector<std::vector<std::string>>& rows,
                    std::string& error) {

    if (!conn || !conn->connected) {
        error = "Not connected";
        return false;
    }

    // Build SQL — same as JDBC driver
    std::string sapLang = sapLangCode(conn->params.lang);
    std::string sql = "select TABNAME, FIELDNAME, INTTYPE, DOMNAME, INTLEN, POSITION, DDTEXT, NOTNULL from DD03VT where DDLANGUAGE = '" + sapLang + "' and TABNAME = '" + escapeSql(tableName) + "'";

    // Execute via RFC
    std::vector<ColumnMeta> rawcols;
    std::vector<std::string> rawrows;
    int row_count = 0;

    if (!rfcExecuteSql(conn, sql.c_str(), rawcols, rawrows, row_count, error)) {
        return false;
    }

    // Build JDBC-compatible result columns (18 columns)
    columns.clear();
    columns.push_back(makeMetaCol("TABLE_CAT", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("TABLE_SCHEM", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("TABLE_NAME", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("COLUMN_NAME", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("DATA_TYPE", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("TYPE_NAME", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("COLUMN_SIZE", SQL_INTEGER, 10));
    columns.push_back(makeMetaCol("BUFFER_LENGTH", SQL_INTEGER, 10));
    columns.push_back(makeMetaCol("DECIMAL_DIGITS", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("NUM_PREC_RADIX", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("NULLABLE", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("REMARKS", SQL_VARCHAR, 255));
    columns.push_back(makeMetaCol("COLUMN_DEF", SQL_VARCHAR, 128));
    columns.push_back(makeMetaCol("SQL_DATA_TYPE", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("SQL_DATETIME_SUB", SQL_SMALLINT, 5));
    columns.push_back(makeMetaCol("CHAR_OCTET_LENGTH", SQL_INTEGER, 10));
    columns.push_back(makeMetaCol("ORDINAL_POSITION", SQL_INTEGER, 10));
    columns.push_back(makeMetaCol("IS_NULLABLE", SQL_VARCHAR, 254));

    // Parse pipe-delimited rows (8 raw columns)
    rows.clear();
    for (const auto& rawrow : rawrows) {
        std::vector<std::string> fields;
        std::string current;
        for (size_t i = 0; i < rawrow.length(); i++) {
            if (rawrow[i] == '|') {
                fields.push_back(current);
                current.clear();
            } else {
                current += rawrow[i];
            }
        }
        fields.push_back(current);
        while (fields.size() < 8) fields.push_back("");

        // fields[0]=TABNAME, [1]=FIELDNAME, [2]=INTTYPE, [3]=DOMNAME,
        // [4]=INTLEN, [5]=POSITION, [6]=DDTEXT, [7]=NOTNULL

        std::string inttype = fields[2];
        std::string intlen = fields[4];
        std::string position = fields[5];
        std::string notnull = fields[7];

        // Map ABAP INTTYPE to ODBC DATA_TYPE
        std::string type_name;
        std::string data_type;
        std::string decimal_digits;
        if (inttype == "C" || inttype == "CHAR") {
            data_type = "12"; type_name = "VARCHAR"; // SQL_VARCHAR
        } else if (inttype == "N") {
            data_type = "12"; type_name = "NUMERIC";
        } else if (inttype == "I" || inttype == "INT4") {
            data_type = "4"; type_name = "INTEGER"; // SQL_INTEGER
        } else if (inttype == "INT2") {
            data_type = "5"; type_name = "SMALLINT";
        } else if (inttype == "INT1") {
            data_type = "-6"; type_name = "TINYINT";
        } else if (inttype == "P" || inttype == "PACKED") {
            data_type = "3"; type_name = "DECIMAL"; // SQL_DECIMAL
            decimal_digits = "0";
        } else if (inttype == "F" || inttype == "FLOAT") {
            data_type = "8"; type_name = "DOUBLE"; // SQL_DOUBLE
        } else if (inttype == "D" || inttype == "DATE") {
            data_type = "91"; type_name = "DATE"; // SQL_TYPE_DATE
        } else if (inttype == "T" || inttype == "TIME") {
            data_type = "92"; type_name = "TIME"; // SQL_TYPE_TIME
        } else if (inttype == "X" || inttype == "RAW") {
            data_type = "-3"; type_name = "VARBINARY";
        } else if (inttype == "STRING") {
            data_type = "-9"; type_name = "WVARCHAR";
        } else {
            data_type = "12"; type_name = "VARCHAR";
        }

        std::string nullable = (notnull == "X") ? "0" : "1"; // 0=NO NULLS, 1=NULLABLE
        std::string is_nullable = (notnull == "X") ? "NO" : "YES";

        std::vector<std::string> row;
        row.push_back("SAPHANADB");          // TABLE_CAT
        row.push_back("SAPHANADB");          // TABLE_SCHEM
        row.push_back(fields[0]);            // TABLE_NAME
        row.push_back(fields[1]);            // COLUMN_NAME
        row.push_back(data_type);            // DATA_TYPE
        row.push_back(type_name);            // TYPE_NAME
        row.push_back(intlen);               // COLUMN_SIZE
        row.push_back(intlen);               // BUFFER_LENGTH
        row.push_back(decimal_digits);       // DECIMAL_DIGITS
        row.push_back("10");                 // NUM_PREC_RADIX
        row.push_back(nullable);             // NULLABLE
        row.push_back(fields[6]);            // REMARKS
        row.push_back("");                   // COLUMN_DEF
        row.push_back(data_type);            // SQL_DATA_TYPE
        row.push_back("");                   // SQL_DATETIME_SUB
        row.push_back(intlen);               // CHAR_OCTET_LENGTH
        row.push_back(position);             // ORDINAL_POSITION
        row.push_back(is_nullable);          // IS_NULLABLE
        rows.push_back(row);
    }

    return true;
}