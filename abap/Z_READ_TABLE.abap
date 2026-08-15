*---------------------------------------------------------------------*
* Funktionbaustein: Z_READ_TABLE
* Zweck: Chunked Table Read für Massendaten-Extraktion
*        Unterstützt ROWSKIPS/ROWCOUNT für Paging
*        Nutzt pipe-delimited ROWDATA (CHAR 10000) wie Z_EXECUTE_SQL
*        Nur für einfache Table-Reads (keine Joins, keine Aggregation)
*        ORDER BY wird unterstützt für konsistentes Paging
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
*"     VALUE(EV_HAS_MORE) TYPE  CHAR1
*"     VALUE(EV_ERROR) TYPE  STRING
*"     VALUE(EV_ROW_COUNT) TYPE  I
*"  TABLES
*"      ET_FIELDS STRUCTURE  ZSQL_FIELD
*"      ET_DATA STRUCTURE  ZSQL_ROW
*"----------------------------------------------------------------------

  DATA: lv_where_clause TYPE string,
        lv_select       TYPE string,
        lv_orderby      TYPE string,
        lt_fields_cat   TYPE TABLE OF ZSQL_FIELD,
        ls_field_cat    TYPE ZSQL_FIELD,
        lt_data         TYPE TABLE OF ZSQL_ROW,
        ls_data         TYPE ZSQL_ROW,
        lo_table_descr  TYPE REF TO cl_abap_tabledescr,
        lo_struct_descr TYPE REF TO cl_abap_structdescr,
        lt_dynamic      TYPE REF TO data,
        ls_dynamic      TYPE REF TO data,
        lv_fieldname    TYPE string,
        lv_field_value  TYPE string,
        lv_char_val     TYPE c LENGTH 100,
        lv_rowdata      TYPE string,
        lv_total_rows   TYPE i,
        lv_check_table  TYPE string,
        lv_tabname      TYPE ddobjname.

  CLEAR: ev_error, ev_row_count, ev_has_more.
  CLEAR: et_fields[], et_data[].

*---------------------------------------------------------------------
* Validate inputs
*---------------------------------------------------------------------
  IF iv_table IS INITIAL.
    ev_error = 'IV_TABLE is empty'.
    RETURN.
  ENDIF.

  " Validate table name — only A-Z, 0-9, underscore and slash (for namespaces like /BIC/) allowed
  lv_check_table = iv_table.
  CONDENSE lv_check_table NO-GAPS.
  TRANSLATE lv_check_table TO UPPER CASE.
  IF NOT lv_check_table CO 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/'.
    ev_error = |Invalid table name (only A-Z, 0-9, _, / allowed): { lv_check_table }|.
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
  SELECT SINGLE tabname FROM dd02l INTO @DATA(lv_exists)
    WHERE tabname = @lv_check_table
      AND as4local = 'A'.
  IF sy-subrc <> 0.
    ev_error = |Table { lv_check_table } does not exist in DDIC|.
    RETURN.
  ENDIF.

  TRY.
      lo_struct_descr ?= cl_abap_structdescr=>describe_by_name( lv_check_table ).
      lo_table_descr = cl_abap_tabledescr=>create( lo_struct_descr ).
      CREATE DATA lt_dynamic TYPE HANDLE lo_table_descr.
      CREATE DATA ls_dynamic TYPE HANDLE lo_struct_descr.
    CATCH cx_root.
      ev_error = 'Cannot create dynamic structure for table'.
      RETURN.
  ENDTRY.

*---------------------------------------------------------------------
* Build and execute dynamic SELECT
*---------------------------------------------------------------------
  DATA: lv_fetch_limit TYPE i,
        lv_offset      TYPE i.

  lv_offset = iv_rowskips.

  IF iv_rowcount > 0.
    lv_fetch_limit = iv_rowcount.
  ELSE.
    lv_fetch_limit = 100000.  " safety limit
  ENDIF.

  " Absolute Obergrenze berücksichtigen
  IF iv_max_rows > 0.
    IF ( lv_offset + lv_fetch_limit ) > iv_max_rows.
      lv_fetch_limit = iv_max_rows - lv_offset.
      IF lv_fetch_limit < 0.
        lv_fetch_limit = 0.
      ENDIF.
    ENDIF.
  ENDIF.

  TRY.
    FIELD-SYMBOLS: <fs_table> TYPE STANDARD TABLE.
    ASSIGN lt_dynamic->* TO <fs_table>.

    IF iv_orderby IS NOT INITIAL.
      " OPTIMIERTES DB-PAGING (ABAP 7.50+): OFFSET erfordert zwingend ORDER BY
      IF lv_where_clause IS NOT INITIAL.
        SELECT (lv_select) FROM (lv_check_table)
          WHERE (iv_where)
          ORDER BY (iv_orderby)
          INTO CORRESPONDING FIELDS OF TABLE @<fs_table>
          UP TO @lv_fetch_limit ROWS
          OFFSET @lv_offset.
      ELSE.
        SELECT (lv_select) FROM (lv_check_table)
          ORDER BY (iv_orderby)
          INTO CORRESPONDING FIELDS OF TABLE @<fs_table>
          UP TO @lv_fetch_limit ROWS
          OFFSET @lv_offset.
      ENDIF.

    ELSE.
      " FALLBACK IN-MEMORY PAGING: Ohne ORDER BY ist kein OFFSET auf der DB zulässig
      DATA: lv_max_fetch TYPE i.
      lv_max_fetch = lv_offset + lv_fetch_limit.

      IF lv_where_clause IS NOT INITIAL.
        SELECT (lv_select) FROM (lv_check_table)
          WHERE (iv_where)
          INTO CORRESPONDING FIELDS OF TABLE @<fs_table>
          UP TO @lv_max_fetch ROWS.
      ELSE.
        SELECT (lv_select) FROM (lv_check_table)
          INTO CORRESPONDING FIELDS OF TABLE @<fs_table>
          UP TO @lv_max_fetch ROWS.
      ENDIF.
    ENDIF.

    CATCH cx_sy_dynamic_osql_error INTO DATA(lo_sql_error).
      ev_error = lo_sql_error->get_text( ).
      RETURN.
    CATCH cx_root INTO DATA(lo_cx_error).
      ev_error = lo_cx_error->get_text( ).
      RETURN.
  ENDTRY.

*---------------------------------------------------------------------*
* Get field metadata via DDIF_NAMETAB_GET 
*---------------------------------------------------------------------*
  DATA: lt_nametab TYPE TABLE OF dfies,
        ls_nametab TYPE dfies.

  lv_tabname = lv_check_table.

  CALL FUNCTION 'DDIF_NAMETAB_GET'
    EXPORTING
      tabname        = lv_tabname
    TABLES
      dfies_tab      = lt_nametab
    EXCEPTIONS
      not_found      = 1
      OTHERS         = 2.

  IF sy-subrc <> 0.
    ev_error = |DDIF_NAMETAB_GET failed for { lv_check_table }|.
    RETURN.
  ENDIF.

  " Remove .INCLUDE entries (field names starting with '.')
  DELETE lt_nametab WHERE fieldname(1) = '.'.

  " If specific fields were requested, filter
  DATA: lt_requested_fields TYPE TABLE OF string,
        lv_field_requested  TYPE string,
        lv_all_fields       TYPE char1.

  IF lv_select = '*'.
    lv_all_fields = 'X'.
  ELSE.
    SPLIT lv_select AT ',' INTO TABLE lt_requested_fields.

    LOOP AT lt_requested_fields INTO DATA(lv_req_field).
      CONDENSE lv_req_field NO-GAPS.
      TRANSLATE lv_req_field TO UPPER CASE.
      IF NOT lv_req_field CO 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/'.
        ev_error = |Invalid field name (only A-Z, 0-9, _, / allowed): { lv_req_field }|.
        RETURN.
      ENDIF.
    ENDLOOP.
  ENDIF.

  DATA: lv_colpos TYPE i.
  lv_colpos = 1.

  FIELD-SYMBOLS: <fs_dynamic> TYPE ANY,
                 <fs_field>   TYPE ANY.

  LOOP AT lt_nametab INTO ls_nametab.
    IF lv_all_fields = 'X'.
      lv_field_requested = 'X'.
    ELSE.
      lv_field_requested = ''.
      LOOP AT lt_requested_fields INTO lv_fieldname.
        IF ls_nametab-fieldname = lv_fieldname.
          lv_field_requested = 'X'.
          EXIT.
        ENDIF.
      ENDLOOP.
    ENDIF.

    IF lv_field_requested = 'X'.
      CLEAR ls_field_cat.
      ls_field_cat-fieldname = ls_nametab-fieldname.

      " Map ABAP INTTYPE to DATATYPE
      CASE ls_nametab-inttype.
        WHEN 'C' OR 'g'.
          ls_field_cat-datatype = 'C'.
        WHEN 'N'.
          ls_field_cat-datatype = 'N'.
        WHEN 'D'.
          ls_field_cat-datatype = 'D'.
        WHEN 'T'.
          ls_field_cat-datatype = 'T'.
        WHEN 'P'.
          ls_field_cat-datatype = 'P'.
        WHEN 'F'.
          ls_field_cat-datatype = 'F'.
        WHEN 'I'.
          ls_field_cat-datatype = 'I'.
        WHEN 's'.
          ls_field_cat-datatype = 'INT2'.
        WHEN 'b'.
          ls_field_cat-datatype = 'INT1'.
        WHEN 'X' OR 'y'.
          ls_field_cat-datatype = 'X'.
        WHEN OTHERS.
          ls_field_cat-datatype = 'C'.
      ENDCASE.

      ls_field_cat-length = ls_nametab-leng.
      ls_field_cat-decimals = ls_nametab-decimals.
      ls_field_cat-colpos = lv_colpos.
      lv_colpos = lv_colpos + 1.

      APPEND ls_field_cat TO et_fields.
    ENDIF.
  ENDLOOP.

*---------------------------------------------------------------------
* Fallback: Delete skipped rows (nur wenn kein ORDER BY übergeben wurde)
*---------------------------------------------------------------------
  IF iv_orderby IS INITIAL AND iv_rowskips > 0.
    lv_total_rows = lines( <fs_table> ).
    IF iv_rowskips >= lv_total_rows.
      IF lv_total_rows = lv_max_fetch.
        ev_has_more = 'X'.
      ENDIF.
      RETURN.
    ENDIF.
    DELETE <fs_table> FROM 1 TO iv_rowskips.
  ENDIF.

*---------------------------------------------------------------------
* Check if there are more rows
*---------------------------------------------------------------------
  IF iv_rowcount > 0.
    IF lines( <fs_table> ) = lv_fetch_limit.
      ev_has_more = 'X'.
    ENDIF.
  ENDIF.

*---------------------------------------------------------------------
* Convert rows to pipe-delimited ROWDATA
*---------------------------------------------------------------------
  LOOP AT <fs_table> ASSIGNING <fs_dynamic>.
    CLEAR lv_rowdata.

    LOOP AT et_fields INTO ls_field_cat.
      ASSIGN COMPONENT ls_field_cat-fieldname OF STRUCTURE <fs_dynamic> TO <fs_field>.
      IF sy-subrc = 0.
        CLEAR lv_field_value.
        CASE ls_field_cat-datatype.
          WHEN 'D'.
            IF <fs_field> IS NOT INITIAL.
              DATA(lv_d) = |{ <fs_field> }|.
              IF strlen( lv_d ) = 8.
                CONCATENATE lv_d(4) '-' lv_d+4(2) '-' lv_d+6(2) INTO lv_field_value.
              ELSE.
                lv_field_value = lv_d.
              ENDIF.
            ENDIF.
          WHEN 'T'.
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
            IF <fs_field> IS NOT INITIAL.
              WRITE <fs_field> TO lv_char_val NO-GROUPING NO-SIGN.
              IF <fs_field> < 0.
                CONCATENATE '-' lv_char_val INTO lv_char_val.
              ENDIF.
              lv_field_value = lv_char_val.
              REPLACE ALL OCCURRENCES OF ',' IN lv_field_value WITH '.'.
              CONDENSE lv_field_value.
              SHIFT lv_field_value LEFT DELETING LEADING SPACE.
            ENDIF.
          WHEN 'F'.
            IF <fs_field> IS NOT INITIAL.
              WRITE <fs_field> TO lv_char_val NO-GROUPING NO-SIGN.
              IF <fs_field> < 0.
                CONCATENATE '-' lv_char_val INTO lv_char_val.
              ENDIF.
              lv_field_value = lv_char_val.
              REPLACE ALL OCCURRENCES OF ',' IN lv_field_value WITH '.'.
              CONDENSE lv_field_value.
              SHIFT lv_field_value LEFT DELETING LEADING SPACE.
            ENDIF.
          WHEN 'X'.
            lv_field_value = |{ <fs_field> }|.
          WHEN OTHERS.
            lv_field_value = |{ <fs_field> }|.
            REPLACE ALL OCCURRENCES OF '|' IN lv_field_value WITH space.
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
