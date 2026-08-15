#include "sap_odbc.h"
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

// I-3 helper: read a string value from registry (HKLM\SOFTWARE\ODBC\ODBC.INI\<DSN>)
static std::string regReadStringDSN(const std::string& dsn, const char* valueName) {
    std::string regPath = "SOFTWARE\\ODBC\\ODBC.INI\\" + dsn;
    HKEY hKey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS) return "";
    char buf[1024];
    DWORD bufLen = sizeof(buf);
    DWORD type = REG_SZ;
    rc = RegQueryValueExA(hKey, valueName, NULL, &type, (LPBYTE)buf, &bufLen);
    RegCloseKey(hKey);
    if (rc != ERROR_SUCCESS) return "";
    return std::string(buf, bufLen > 0 ? bufLen - 1 : 0);
}

// I-3 helper: load DSN params from registry if not already set from connection string
static void loadDsnFromRegistry(ConnectionParams& params, const std::string& dsn) {
    if (params.host.empty())    params.host = regReadStringDSN(dsn, "Host");
    if (params.sysnr.empty())   params.sysnr = regReadStringDSN(dsn, "SysNr");
    if (params.client.empty())  params.client = regReadStringDSN(dsn, "Client");
    if (params.user.empty())    params.user = regReadStringDSN(dsn, "User");
    if (params.password.empty()) params.password = regReadStringDSN(dsn, "Password");
    std::string regLang = regReadStringDSN(dsn, "Lang");
    if (!regLang.empty() && params.lang == "EN") params.lang = regLang;
    std::string regMaxRows = regReadStringDSN(dsn, "MaxRows");
    if (!regMaxRows.empty() && params.max_rows == MAX_ROWS_DEFAULT)
        params.max_rows = atoi(regMaxRows.c_str());
}

// Parse connection string: "Host=sap-prod.firma.de;SysNr=10;Client=100;User=DEIN_USER;Password=********"
ConnectionParams parseConnectionString(const std::string& connstr) {
    ConnectionParams params;
    params.lang = "EN";
    params.max_rows = MAX_ROWS_DEFAULT;
    
    std::istringstream ss(connstr);
    std::string token;
    while (std::getline(ss, token, ';')) {
        size_t pos = token.find('=');
        if (pos == std::string::npos) continue;
        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);
        // trim whitespace from key
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        
        // I-4 fix: trim whitespace from value
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        
        if (key == "Host" || key == "host" || key == "HOST") params.host = value;
        else if (key == "SysNr" || key == "sysnr" || key == "SYSNR") params.sysnr = value;
        else if (key == "Client" || key == "client" || key == "CLIENT") params.client = value;
        else if (key == "User" || key == "user" || key == "USER") params.user = value;
        else if (key == "Password" || key == "password" || key == "PWD") params.password = value;
        else if (key == "Lang" || key == "lang" || key == "LANG") params.lang = value;
        else if (key == "MaxRows" || key == "maxrows") params.max_rows = std::atoi(value.c_str());
        else if (key == "DSN" || key == "dsn") {
            params.dsn_name = value;
            // I-3 fix: load DSN params from registry
            loadDsnFromRegistry(params, value);
        }
        // I-5 fix: ignore DRIVER= key (it's the ODBC driver name, not used by us)
        else if (key == "DRIVER" || key == "driver") { /* ignore */ }
    }
    return params;
}

SQLRETURN returnSqlError(SQLHANDLE handle, SQLSMALLINT handleType, 
                         const std::string& message, const char* state) {
    // Store the error so SQLGetDiagRec can retrieve it
    setError(handle, handleType, message, state);
    // Also log to debug output
    OutputDebugStringA("SAP ODBC: ");
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    return SQL_ERROR;
}