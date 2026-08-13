#pragma once

// Stub header for SAP NWRFC SDK — for CI compilation only
// Mimics the real sapnwrfc.h API surface

#include <cstring>
#include <cstdlib>

// SAP_UC is wchar_t on Windows
typedef wchar_t SAP_RAW;
typedef SAP_RAW SAP_UC;

// Helper macro to convert C string literals to SAP_UC
#define TO_SAP_UC(str) ((const SAP_UC*)(L##str))

// Compatibility for old code
#define CU8(str) TO_SAP_UC(str)

// Handle types — real SDK uses structs with void* handle
typedef struct { void* handle; } *RFC_CONNECTION_HANDLE;
typedef struct { void* handle; } *RFC_FUNCTION_DESC_HANDLE;
typedef struct RFC_DATA_CONTAINER { void* handle; } *DATA_CONTAINER_HANDLE;
typedef DATA_CONTAINER_HANDLE RFC_FUNCTION_HANDLE;
typedef DATA_CONTAINER_HANDLE RFC_STRUCTURE_HANDLE;
typedef DATA_CONTAINER_HANDLE RFC_TABLE_HANDLE;

typedef int RFC_RC;
typedef int RFC_INT;

#define RFC_OK 0

// Error info
typedef struct {
    RFC_RC code;
    SAP_UC message[256];
    RFC_RC group;
} RFC_ERROR_INFO;

// Connection parameter
typedef struct {
    const SAP_UC* name;
    const SAP_UC* value;
} RFC_CONNECTION_PARAMETER;

// Functions — stubs
static inline RFC_CONNECTION_HANDLE RfcOpenConnection(RFC_CONNECTION_PARAMETER const* params, unsigned count, RFC_ERROR_INFO* error) {
    error->code = 1;
    wcscpy_s(error->message, 256, L"Stub: no real NWRFC SDK");
    return NULL;
}
static inline RFC_RC RfcCloseConnection(RFC_CONNECTION_HANDLE conn, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_FUNCTION_DESC_HANDLE RfcGetFunctionDesc(RFC_CONNECTION_HANDLE conn, const SAP_UC* name, RFC_ERROR_INFO* error) { return NULL; }
static inline RFC_FUNCTION_HANDLE RfcCreateFunction(RFC_FUNCTION_DESC_HANDLE desc, RFC_ERROR_INFO* error) { return NULL; }
static inline RFC_RC RfcDestroyFunction(RFC_FUNCTION_HANDLE func, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcSetString(DATA_CONTAINER_HANDLE h, const SAP_UC* name, const SAP_UC* value, unsigned len, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcSetInt(DATA_CONTAINER_HANDLE h, const SAP_UC* name, RFC_INT value, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcGetInt(DATA_CONTAINER_HANDLE h, const SAP_UC* name, RFC_INT* value, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcGetString(DATA_CONTAINER_HANDLE h, const SAP_UC* name, SAP_UC* buf, unsigned buf_len, unsigned* result_len, RFC_ERROR_INFO* error) {
    if (buf && buf_len > 0) buf[0] = 0;
    if (result_len) *result_len = 0;
    return RFC_OK;
}
static inline RFC_RC RfcInvoke(RFC_CONNECTION_HANDLE conn, RFC_FUNCTION_HANDLE func, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_RC RfcGetTable(DATA_CONTAINER_HANDLE h, const SAP_UC* name, RFC_TABLE_HANDLE* table, RFC_ERROR_INFO* error) {
    if (table) *table = NULL;
    return RFC_OK;
}
static inline RFC_RC RfcGetRowCount(RFC_TABLE_HANDLE table, unsigned* count, RFC_ERROR_INFO* error) {
    if (count) *count = 0;
    return RFC_OK;
}
static inline RFC_RC RfcMoveTo(RFC_TABLE_HANDLE table, unsigned index, RFC_ERROR_INFO* error) { return RFC_OK; }
static inline RFC_STRUCTURE_HANDLE RfcGetCurrentRow(RFC_TABLE_HANDLE table, RFC_ERROR_INFO* error) { return NULL; }

// UTF-8 conversion — stub
static inline int RfcUTF8SFromSAPUC(char* dest, int dest_len, const SAP_UC* src, int src_len, int* result_len, void* reserved) {
    if (dest && dest_len > 0 && src && src_len > 0) {
        int copy_len = (src_len < dest_len - 1) ? src_len : dest_len - 1;
        for (int i = 0; i < copy_len; i++) dest[i] = (char)src[i];
        dest[copy_len] = 0;
        if (result_len) *result_len = copy_len;
    } else if (result_len) {
        *result_len = 0;
    }
    return 0;
}

// Lifecycle
static inline RFC_RC RfcInit() { return RFC_OK; }
static inline void RfcCleanup() {}