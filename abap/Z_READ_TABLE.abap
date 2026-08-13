*---------------------------------------------------------------------*
* Funktionbaustein: Z_READ_TABLE
* Zweck: Chunked Table Read für Massendaten-Extraktion
*        Unterstützt ROWSKIPS/ROWCOUNT für Paging
*        Nutzt pipe-delimited ROWDATA (CHAR 10000) wie Z_EXECUTE_SQL
*        Nur für einfache Table-Reads (keine Joins, keine Aggregation)
*        ORDER BY wird unterstützt für konsistentes Paging
*---------------------------------------------------------------------*
* Installation:
* 1. SE37 → Funktionsgruppe Z_SQL (dieselbe wie Z_EXECUTE_SQL)
* 2. Neuer Funktionsbaustein Z_READ_TABLE
* 3. Attribute: Remote-Enabled Module ✓
* 4. Interface wie unten dokumentiert
* 5. Code einfügen
*---------------------------------------------------------------------*
* Interface:
*
* IMPORTING:
*   IV_TABLE     TYPE TABNAME       - Tabellenname (z.B. 'MARA')
*   IV_WHERE     TYPE STRING        - WHERE-Klausel ohne 'WHERE' (z.B. "MTART = 'FERT'")
*   IV_FIELDS    TYPE STRING        - Feldliste komma-separiert (z.B. 'MATNR,MTART') oder '*' für alle
*   IV_ORDERBY   TYPE STRING        - ORDER BY-Klausel (z.B. 'MATNR') — empfohlen für konsistentes Paging
*   IV_ROWSKIPS  TYPE I             - Anzahl zu überspringender Zeilen (Default 0)
*   IV_ROWCOUNT  TYPE I             - Maximale Zeilen pro Aufruf (z.B. 10000, 0 = alle)
*   IV_MAX_ROWS  TYPE I             - Absolute Obergrenze (0 = keine)
*
* EXPORTING:
*   EV_ROW_COUNT TYPE I             - Tatsächlich zurückgegebene Zeilen
*   EV_HAS_MORE  TYPE CHAR1         - 'X' = weitere Zeilen verfügbar, ' ' = komplett
*   EV_ERROR     TYPE STRING        - Fehlermeldung (leer = OK)
*
* TABLES:
*   ET_FIELDS    STRUCTURE ZSQL_FIELD  - Spaltenmetadaten (wie Z_EXECUTE_SQL)
*   ET_DATA      STRUCTURE ZSQL_ROW    - Pipe-delimited Zeilendaten (wie Z_EXECUTE_SQL)
*---------------------------------------------------------------------*

FUNCTION Z_READ_TABLE.
*"----------------------------------------------------------------------
*"*"Lokale Schnittstelle:
*"  IMPORTING
*"     VALUE(IV_TABLE) TYPE  TABNAME
*"     VALUE(IV_WHERE) TYPE  STRING OPTIONAL
*"     VALUE(IV_FIELDS) TYPE  STRING OPTIONAL
*"     VALUE(IV_ORDERBY) TYPE  STRING OPTIONAL
*"     VALUE(IV_ROWSKIPS) TYPE  I DEFAULT 0
*"     VALUE(IV_ROWCOUNT) TYPE  I DEFAULT 0
*"     VALUE(IV_MAX_ROWS) TYPE  I DEFAULT 0
*"  EXPORTING
*"     VALUE(EV_ROW_COUNT) TYPE  I
*"     VALUE(EV_HAS_MORE) TYPE  CHAR1
*"     VALUE(EV_ERROR) TYPE  STRING
*"  TABLES
*"     ET_FIELDS STRUCTURE ZSQL_FIELD
*"     ET_DATA STRUCTURE ZSQL_ROW
*"----------------------------------------------------------------------

  DATA: lv_where_clause TYPE string,
        lv_select        TYPE string,
        lv_orderby       TYPE string,
        lt_fields_cat    TYPE TABLE OF ZSQL_FIELD,
        ls_field_cat     TYPE ZSQL_FIELD,
        lt_data          TYPE TABLE OF ZSQL_ROW,
        ls_data          TYPE ZSQL_ROW,
        lo_table_descr   TYPE REF TO cl_abap_tabledescr,
        lo_struct_descr  TYPE REF TO cl_abap_structdescr,
        lo_data_ref      TYPE REF TO data,
        lt_dynamic       TYPE REF TO data,
        ls_dynamic       TYPE REF TO data,
        lv_fieldname     TYPE string,
        lv_field_value   TYPE string,
        lv_rowdata       TYPE string,
        lv_total_rows    TYPE i,
        lv_fetch_count   TYPE i,
        lv_skip          TYPE i.

  CLEAR: ev_error, ev_row_count, ev_has_more.
  CLEAR: et_fields[], et_data[].

  *---------------------------------------------------------------------
  * Validate inputs
  *---------------------------------------------------------------------
  IF iv_table IS INITIAL.
    ev_error = 'IV_TABLE is empty'.
    RETURN.
  ENDIF.

  *---------------------------------------------------------------------
  * Build field list
  *---------------------------------------------------------------------
  IF iv_fields IS INITIAL OR iv_fields = '*' OR iv_fields = ''.
    lv_select = '*'.
  ELSE.
    lv_select = iv_fields.
  ENDIF.

  *---------------------------------------------------------------------
  * Build WHERE clause
  *---------------------------------------------------------------------
  IF iv_where IS NOT INITIAL.
    CONCATENATE 'WHERE' iv_where INTO lv_where_clause SEPARATED BY space.
  ENDIF.

  *---------------------------------------------------------------------
  * Build ORDER BY
  *---------------------------------------------------------------------
  IF iv_orderby IS NOT INITIAL.
    CONCATENATE 'ORDER BY' iv_orderby INTO lv_orderby SEPARATED BY space.
  ENDIF.

  *---------------------------------------------------------------------
  * Create dynamic structure for the table
  *---------------------------------------------------------------------
  TRY.
      lo_struct_descr ?= cl_abap_structdescr=>describe_by_name( iv_table ).
      lo_table_descr = cl_abap_tabledescr=>create( lo_struct_descr ).
      CREATE DATA lt_dynamic TYPE HANDLE lo_table_descr.
      CREATE DATA ls_dynamic TYPE HANDLE lo_struct_descr.
    CATCH cx_root.
      ev_error = 'Cannot create dynamic structure for table'.
      RETURN.
  ENDTRY.

  *---------------------------------------------------------------------
  * Build and execute dynamic SELECT
  * Use UP TO ROWS to limit, then delete skipped rows
  *---------------------------------------------------------------------
  DATA: lv_max_fetch TYPE i.
  lv_max_fetch = iv_rowcount + iv_rowskips.
  IF iv_max_rows > 0 AND lv_max_fetch > iv_max_rows.
    lv_max_fetch = iv_max_rows.
  ENDIF.
  IF lv_max_fetch = 0.
    lv_max_fetch = 100000.  " safety limit
  ENDIF.

  TRY.
      * Build the full dynamic SQL statement
      DATA: lv_sql TYPE string.

      IF lv_orderby IS NOT INITIAL.
        CONCATENATE 'SELECT' lv_select 'FROM' iv_table
                    lv_where_clause lv_orderby
                    'INTO TABLE' 'lt_dynamic'
                    'UP TO' lv_max_fetch 'ROWS'
                    INTO lv_sql SEPARATED BY space.
      ELSE.
        CONCATENATE 'SELECT' lv_select 'FROM' iv_table
                    lv_where_clause
                    'INTO TABLE' 'lt_dynamic'
                    'UP TO' lv_max_fetch 'ROWS'
                    INTO lv_sql SEPARATED BY space.
      ENDIF.

      * Execute — use dynamic SELECT with subrc check
      * NOTE: ABAP dynamic SELECT needs Open SQL syntax
      * We use SELECT (fields) FROM (table) WHERE (where) ORDER BY (orderby)
      * This works on ABAP 7.00+

      IF lv_where_clause IS NOT INITIAL AND lv_orderby IS NOT INITIAL.
        SELECT (lv_select) FROM (iv_table)
          WHERE (iv_where)
          ORDER BY (iv_orderby)
          INTO TABLE lt_dynamic UP TO lv_max_fetch ROWS.
      ELSEIF lv_where_clause IS NOT INITIAL.
        SELECT (lv_select) FROM (iv_table)
          WHERE (iv_where)
          INTO TABLE lt_dynamic UP TO lv_max_fetch ROWS.
      ELSEIF lv_orderby IS NOT INITIAL.
        SELECT (lv_select) FROM (iv_table)
          ORDER BY (iv_orderby)
          INTO TABLE lt_dynamic UP TO lv_max_fetch ROWS.
      ELSE.
        SELECT (lv_select) FROM (iv_table)
          INTO TABLE lt_dynamic UP TO lv_max_fetch ROWS.
      ENDIF.

    CATCH cx_sy_dynamic_osql_error INTO DATA(lo_sql_error).
      ev_error = lo_sql_error->get_text( ).
      RETURN.
    CATCH cx_root INTO DATA(lo_cx_error).
      ev_error = lo_cx_error->get_text( ).
      RETURN.
  ENDTRY.

  *---------------------------------------------------------------------
  * Get field metadata from the dynamic structure
  *---------------------------------------------------------------------
  DATA: lt_components TYPE cl_abap_structdescr=>component_table,
        ls_component  TYPE abap_componentdescr.

  lt_components = lo_struct_descr->get_components( ).

  * If specific fields were requested, filter
  DATA: lt_requested_fields TYPE TABLE OF string,
        lv_field_requested  TYPE string,
        lv_all_fields       TYPE char1.

  IF lv_select = '*'.
    lv_all_fields = 'X'.
  ELSE.
    SPLIT lv_select AT ',' INTO TABLE lt_requested_fields.
  ENDIF.

  DATA: lv_colpos TYPE i.
  lv_colpos = 1.

  FIELD-SYMBOLS: <fs_dynamic> TYPE ANY,
                 <fs_field>   TYPE ANY,
                 <fs_table>   TYPE STANDARD TABLE.

  ASSIGN lt_dynamic->* TO <fs_table>.

  LOOP AT lt_components INTO ls_component.
    * Check if field is requested
    IF lv_all_fields = 'X'.
      lv_field_requested = 'X'.
    ELSE.
      lv_field_requested = ''.
      LOOP AT lt_requested_fields INTO lv_fieldname.
        IF ls_component-name = lv_fieldname.
          lv_field_requested = 'X'.
          EXIT.
        ENDIF.
      ENDLOOP.
    ENDIF.

    IF lv_field_requested = 'X'.
      CLEAR ls_field_cat.
      ls_field_cat-fieldname = ls_component-name.

      * Map ABAP type to DATATYPE
      CASE ls_component-type_kind.
        WHEN cl_abap_structdescr=>typekind_char
          OR cl_abap_structdescr=>typekind_string.
          ls_field_cat-datatype = 'C'.
        WHEN cl_abap_structdescr=>typekind_int.
          ls_field_cat-datatype = 'I'.
        WHEN cl_abap_structdescr=>typekind_int2.
          ls_field_cat-datatype = 'INT2'.
        WHEN cl_abap_structdescr=>typekind_int1.
          ls_field_cat-datatype = 'INT1'.
        WHEN cl_abap_structdescr=>typekind_packed.
          ls_field_cat-datatype = 'P'.
        WHEN cl_abap_structdescr=>typekind_float.
          ls_field_cat-datatype = 'F'.
        WHEN cl_abap_structdescr=>typekind_date.
          ls_field_cat-datatype = 'D'.
        WHEN cl_abap_structdescr=>typekind_time.
          ls_field_cat-datatype = 'T'.
        WHEN cl_abap_structdescr=>typekind_hex.
          ls_field_cat-datatype = 'X'.
        WHEN OTHERS.
          ls_field_cat-datatype = 'C'.
      ENDCASE.

      ls_field_cat-length = ls_component-length.
      ls_field_cat-decimals = ls_component-decimals.
      ls_field_cat-colpos = lv_colpos.
      lv_colpos = lv_colpos + 1.

      APPEND ls_field_cat TO et_fields.
    ENDIF.
  ENDLOOP.

  *---------------------------------------------------------------------
  * Delete skipped rows (ROWSKIPS)
  *---------------------------------------------------------------------
  lv_total_rows = lines( <fs_table> ).

  IF iv_rowskips > 0.
    IF iv_rowskips >= lv_total_rows.
      * All rows were skipped — no data to return
      IF lv_total_rows = lv_max_fetch.
        ev_has_more = 'X'.
      ENDIF.
      RETURN.
    ENDIF.
    * Delete the first ROWSKIPS rows
    DELETE <fs_table> FROM 1 TO iv_rowskips.
    lv_total_rows = lines( <fs_table> ).
  ENDIF.

  *---------------------------------------------------------------------
  * Check if there are more rows
  *---------------------------------------------------------------------
  * If we fetched exactly lv_max_fetch rows, there might be more
  DATA: lv_fetched_total TYPE i.
  lv_fetched_total = lines( <fs_table> ) + iv_rowskips.

  IF lv_fetched_total >= lv_max_fetch AND iv_rowcount > 0.
    ev_has_more = 'X'.
  ENDIF.

  *---------------------------------------------------------------------
  * Convert rows to pipe-delimited ROWDATA
  *---------------------------------------------------------------------
  LOOP AT <fs_table> ASSIGNING <fs_dynamic>.
    CLEAR lv_rowdata.

    LOOP AT et_fields INTO ls_field_cat.
      * Get field value dynamically
      ASSIGN COMPONENT ls_field_cat-fieldname OF STRUCTURE <fs_dynamic> TO <fs_field>.
      IF sy-subrc = 0.
        * Type-aware conversion to MSSQL-compatible formats
        CLEAR lv_field_value.
        CASE ls_field_cat-datatype.
          WHEN 'D'.
            * SAP DATE YYYYMMDD → MSSQL YYYY-MM-DD
            IF <fs_field> IS NOT INITIAL.
              DATA(lv_d) = |{ <fs_field> }|.
              IF strlen( lv_d ) = 8.
                CONCATENATE lv_d(4) '-' lv_d+4(2) '-' lv_d+6(2) INTO lv_field_value.
              ELSE.
                lv_field_value = lv_d.
              ENDIF.
            ENDIF.
          WHEN 'T'.
            * SAP TIME HHMMSS → MSSQL HH:MM:SS
            IF <fs_field> IS NOT INITIAL.
              DATA(lv_t) = |{ <fs_field> }|.
              IF strlen( lv_t ) = 6.
                CONCATENATE lv_t(2) ':' lv_t+2(2) ':' lv_t+4(2) INTO lv_field_value.
              ELSE.
                lv_field_value = lv_t.
              ENDIF.
            ENDIF.
          WHEN 'I' OR 'INT1' OR 'INT2'.
            lv_field_value = |{ <fs_field> }|.
            CONDENSE lv_field_value.
          WHEN 'P'.
            * Packed decimal with dot, no thousand separators
            IF <fs_field> IS NOT INITIAL.
              WRITE <fs_field> TO lv_field_value NO-GROUPING.
              REPLACE ALL OCCURRENCES OF ',' IN lv_field_value WITH '.'.
              CONDENSE lv_field_value.
              SHIFT lv_field_value LEFT DELETING LEADING SPACE.
            ENDIF.
          WHEN 'F'.
            * Float with dot notation
            IF <fs_field> IS NOT INITIAL.
              WRITE <fs_field> TO lv_field_value NO-GROUPING.
              REPLACE ALL OCCURRENCES OF ',' IN lv_field_value WITH '.'.
              CONDENSE lv_field_value.
              SHIFT lv_field_value LEFT DELETING LEADING SPACE.
            ENDIF.
          WHEN 'X'.
            * RAW → hex string with 0x prefix
            DATA(lv_x) = |{ <fs_field> }|.
            CONCATENATE '0x' lv_x INTO lv_field_value.
          WHEN OTHERS.
            * CHAR/STRING — remove leading/trailing spaces
            lv_field_value = |{ <fs_field> }|.
            SHIFT lv_field_value RIGHT DELETING TRAILING space.
            SHIFT lv_field_value LEFT DELETING LEADING space.
        ENDCASE.

        IF lv_rowdata IS INITIAL.
          lv_rowdata = lv_field_value.
        ELSE.
          CONCATENATE lv_rowdata lv_field_value INTO lv_rowdata SEPARATED BY '|'.
        ENDIF.
      ENDIF.
    ENDLOOP.

    ls_data-rowdata = lv_rowdata.
    APPEND ls_data TO et_data.
  ENDLOOP.

  ev_row_count = lines( et_data ).

ENDFUNCTION.