#include "sap_odbc.h"
#include <sstream>
#include <algorithm>

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
        // trim whitespace
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        
        if (key == "Host" || key == "host" || key == "HOST") params.host = value;
        else if (key == "SysNr" || key == "sysnr" || key == "SYSNR") params.sysnr = value;
        else if (key == "Client" || key == "client" || key == "CLIENT") params.client = value;
        else if (key == "User" || key == "user" || key == "USER") params.user = value;
        else if (key == "Password" || key == "password" || key == "PWD") params.password = value;
        else if (key == "Lang" || key == "lang" || key == "LANG") params.lang = value;
        else if (key == "MaxRows" || key == "maxrows") params.max_rows = std::atoi(value.c_str());
    }
    return params;
}

SQLRETURN returnSqlError(SQLHANDLE handle, SQLSMALLINT handleType, 
                         const std::string& message, const char* state) {
    // In a real driver, we'd store this in the handle's error buffer
    // For now, log to debug output
    OutputDebugStringA("SAP ODBC: ");
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    return SQL_ERROR;
}