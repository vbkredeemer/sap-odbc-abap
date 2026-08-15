/*
 * test_exports.c — Check if GetProcAddress can find each ODBC function
 * Build: x86_64-w64-mingw32-gcc -o test_exports.exe test_exports.c -lkernel32
 */
#include <windows.h>
#include <stdio.h>

int main() {
    FILE* f = fopen("C:\\Scripts\\SAP_ODBC\\test_exports.log", "w");
    if (!f) f = fopen("test_exports.log", "w");
    if (!f) { MessageBoxA(NULL, "Cannot open log", "Test", MB_OK); return 1; }

    fprintf(f, "=== GetProcAddress Test ===\n\n");

    /* Try to load the DLL from System32 */
    HMODULE hDll = LoadLibraryA("C:\\Windows\\System32\\sapodbcabap.dll");
    if (!hDll) {
        fprintf(f, "FATAL: Cannot load C:\\Windows\\System32\\sapodbcabap.dll\n");
        fprintf(f, "GetLastError = %lu\n", GetLastError());
        fclose(f);
        MessageBoxA(NULL, "Cannot load DLL", "Test Exports", MB_OK);
        return 1;
    }
    fprintf(f, "DLL loaded at %p\n\n", (void*)hDll);

    /* Check each ODBC function */
    const char* funcs[] = {
        "ConfigDSN",
        "SQLAllocHandle", "SQLFreeHandle",
        "SQLDriverConnect", "SQLConnect", "SQLDisconnect",
        "SQLExecDirect", "SQLPrepare", "SQLExecute",
        "SQLFetch", "SQLGetData", "SQLNumResultCols",
        "SQLDescribeCol", "SQLColAttribute", "SQLRowCount",
        "SQLMoreResults", "SQLCancel", "SQLCloseCursor",
        "SQLBindParameter", "SQLBindCol", "SQLEndTran",
        "SQLSetConnectAttr", "SQLSetStmtAttr", "SQLSetEnvAttr",
        "SQLGetInfo", "SQLGetFunctions", "SQLGetTypeInfo",
        "SQLTables", "SQLColumns", "SQLError",
        "SQLGetDiagRec", "SQLGetDiagField",
        "SQLSetPos", "SQLBulkOperations",
        "SQLGetConnectAttr", "SQLGetStmtAttr", "SQLGetEnvAttr",
        "SQLBrowseConnect", "SQLGetCursorName", "SQLSetCursorName",
        "SQLNativeSql",
        /* Also check for possible decorated/mangled names */
        "_Z13SQLExecDirectPvPhs", "_Z10SQLPreparePvPhs",
        "SQLExecDirect@24", "SQLPrepare@24",
        NULL
    };

    for (int i = 0; funcs[i]; i++) {
        FARPROC proc = GetProcAddress(hDll, funcs[i]);
        fprintf(f, "  %-40s : %s\n", funcs[i], proc ? "FOUND" : "NOT FOUND");
    }

    /* Also try enumerating the export table directly */
    fprintf(f, "\n=== Export Table Enumeration ===\n");
    /* GetProcAddress only works for named exports. Let's check
       if SQLExecDirect is really there by trying a few variants. */
    const char* variants[] = {
        "SQLExecDirect", "SQLExecDirectA", "SQLExecDirectW",
        "_SQLExecDirect", "SQLExecDirect@0", NULL
    };
    fprintf(f, "\nSQLExecDirect variants:\n");
    for (int i = 0; variants[i]; i++) {
        FARPROC proc = GetProcAddress(hDll, variants[i]);
        fprintf(f, "  %-30s : %s\n", variants[i], proc ? "FOUND" : "NOT FOUND");
    }

    FreeLibrary(hDll);
    fprintf(f, "\n=== Done ===\n");
    fclose(f);

    MessageBoxA(NULL,
        "Export test complete. Check C:\\Scripts\\SAP_ODBC\\test_exports.log",
        "Test Exports", MB_OK | MB_SETFOREGROUND);
    return 0;
}