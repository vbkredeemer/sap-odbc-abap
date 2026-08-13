#pragma once

// Stub header for SAP NWRFC SDK — for CI compilation only
// On real builds, use the actual sapnwrfc.h from the NWRFC SDK

// Basic types
typedef unsigned short SAP_RAW;
typedef SAP_RAW SAP_UC;
typedef SAP_RAW* SAP_UTF8;
typedef const SAP_UC* CU8;
typedef void* RFC_CONNECTION_HANDLE;
typedef void* RFC_FUNCTION_HANDLE;
typedef void* RFC_TABLE_HANDLE;
typedef void* RFC_STRUCTURE_HANDLE;

typedef int RFC_RC;
typedef int RFC_INT;

#define RFC_OK 0
#define RFC_NOT_FOUND 1

// Error info
typedef struct {
    RFC_RC code;
    SAP_UC message[256];
    int key[8];
} RFC_ERROR_INFO;

// Connection parameter
typedef struct {
    const SAP_UC* name;
    const SAP_UC* value;
} RFC_CONNECTION_PARAMETER_V3;

// Functions — stubs that return NULL/0
static inline RFC_CONNECTION_HANDLE RfcOpenConnectionV3(RFC_CONNECTION_PARAMETER_V3* params, unsigned count, RFC_ERROR_INFO* error) { return NULL; }
static inline RFC_RC RfcCloseConnection(RFC_CONNECTION_HANDLE conn, RFC_ERROR_INFO* error, void* reserved) { return RFC_OK; }
static inline RFC_FUNCTION_HANDLE RfcCreateFunction(RFC_CONNECTION_HANDLE conn, const SAP_UC* name, RFC_ERROR_INFO* error) { return NULL; }
static inline RFC_RC RfcDestroyFunction(RFC_FUNCTION_HANDLE func, RFC_ERROR_INFO* error, void* reserved) { return RFC_OK; }
static inline RFC_RC RfcSetString(RFC_FUNCTION_HANDLE func, const SAP_UC* name, const SAP_UC* value, unsigned len, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcSetInt(RFC_FUNCTION_HANDLE func, const SAP_UC* name, RFC_INT value, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcGetInt(RFC_FUNCTION_HANDLE func, const SAP_UC* name, RFC_INT* value, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcGetString(RFC_FUNCTION_HANDLE func, const SAP_UC* name, SAP_UC* buffer, unsigned buffer_len, unsigned* result_len, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcInvoke(RFC_CONNECTION_HANDLE conn, RFC_FUNCTION_HANDLE func, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_TABLE_HANDLE RfcGetTable(RFC_FUNCTION_HANDLE func, const SAP_UC* name, RFC_ERROR_INFO* error) { return NULL; }
static inline int RfcGetRowCount(RFC_TABLE_HANDLE table, RFC_ERROR_INFO* error, void* reserved) { return 0; }
static inline RFC_RC RfcMoveTo(RFC_TABLE_HANDLE table, int index, RFC_ERROR_INFO* error, void* reserved) { return RFC_OK; }
static inline RFC_STRUCTURE_HANDLE RfcGetCurrentRow(RFC_TABLE_HANDLE table, RFC_ERROR_INFO* error, void* reserved) { return NULL; }

// UTF-8 conversion — stub
static inline int RfcUTF8SFromSAPUC(char* dest, int dest_len, const SAP_UC* src, int src_len, int* result_len, void* reserved) {
    if (dest && dest_len > 0 && src && src_len > 0) {
        int copy_len = (src_len < dest_len - 1) ? src_len : dest_len - 1;
        for (int i = 0; i < copy_len; i++) dest[i] = (char)src[i];
        dest[copy_len] = 0;
        if (result_len) *result_len = copy_len;
    }
    return 0;
}

// Lifecycle
static inline void RfcStartup() {}
static inline void RfcCleanup() {}