#include "../include/ibm_db.h"

/*Initialize ODBC Handler*/
ODBC *init_handlers(){
    ODBC *odbc = malloc(sizeof(ODBC));
    if (!odbc) return NULL;
    odbc->env = NULL;
    odbc->dbc = NULL;
    odbc->error_head = NULL;
    return odbc;
}

/*Setup ODBC Connection to IBM Database*/
ODBC * db_connect(ODBC *odbc, char *connStr){

    SQLRETURN ret;
    SQLCHAR outStr[1024];
    SQLSMALLINT outStrlen;

    odbc = init_handlers();

    if (odbc == NULL) return NULL;

    /* Allocate environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(odbc->env));

    /* ODBC 3 support */
    SQLSetEnvAttr(odbc->env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

    /* Allocate connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, odbc->env, &(odbc->dbc));

    /* Connect to the Database*/
    ret = SQLDriverConnect(odbc->dbc, NULL, (SQLCHAR *) connStr, SQL_NTS,
                           outStr, sizeof(outStr), &outStrlen,
                           SQL_DRIVER_COMPLETE);

    /*Run Result*/
    if (SQL_SUCCEEDED(ret)) {
        if (ret == SQL_SUCCESS_WITH_INFO) {

            /*TODO: Return feedback message */
        }
    } else {
        /*Generate error message*/
        odbc->error_head = get_error_connection("Database Connect", odbc->dbc, SQL_HANDLE_DBC);
        return odbc;
    }
    return odbc;
}

/*Generate Error Message*/
CONNECTION_ERROR * get_error_connection(char *fn, SQLHANDLE handle, SQLSMALLINT type) {
    SQLINTEGER i = 0;
    SQLINTEGER native;
    SQLCHAR state[7];
    SQLCHAR text[256];
    SQLSMALLINT len;
    SQLRETURN ret;

    CONNECTION_ERROR *head = NULL;
    CONNECTION_ERROR *tail = NULL;
    CONNECTION_ERROR *temp = NULL;
    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len );

        if (SQL_SUCCEEDED(ret)){

            if(!head){
                head = (CONNECTION_ERROR *) malloc(sizeof(CONNECTION_ERROR));
                head->next = NULL;
                head->native = native;
                head->state = (char *) malloc(7 * sizeof(char));
                head->description = (char *) malloc(256 * sizeof(char));
                strcpy(head->state, ((char *) state));
                strcpy(head->description, ((char *) text));
                tail = head;
            }else{
                temp = (CONNECTION_ERROR *) malloc(sizeof(CONNECTION_ERROR));
                temp->next = NULL;
                temp->native = native;
                temp->state = (char *) malloc(7 * sizeof(char));
                temp->description = (char *) malloc(256 * sizeof(char));
                strcpy(temp->state, ((char *) state));
                strcpy(temp->description, ((char *) text));
                tail->next = temp;
                tail = tail->next;
                tail->next = NULL;
                temp = NULL;
            }
        }
    }
    while(ret == SQL_SUCCESS);
    return head;
}

/*Check whether the ODBC Handler has error*/
bool connection_error_found (ODBC * head){
    return head && head->error_head ? true : false;
}

char * state_connection_error (ODBC * head) {
    return !head || !head->error_head ? "99999" : head->error_head->state;
}

char * description_connection_error (ODBC * head){
    return !head || !head->error_head ? "UnKnown Error.\n" : head->error_head->description;
}

int native_connection_error (ODBC * head){
    return !head || !head->error_head ? -99999 : head->error_head->native;
}

ODBC * free_connection_error_node(ODBC * head){

    if(!head) return NULL;
    if(!head->error_head) return head;

    CONNECTION_ERROR * temp = head->error_head;
    head->error_head = head->error_head->next;
    temp->next = NULL;
    free(temp->description);
    free(temp->state);
    free(temp);
    temp = NULL;
    return head;
}

/*Get SQL Database info*/
DB_INFO * db_info (ODBC *odbc) {

    SQLUINTEGER getdata_support;
    SQLUSMALLINT max_concur_act;

    if (!odbc) return NULL;

    DB_INFO * info = (DB_INFO *)malloc(sizeof(DB_INFO));

    if(!info) return NULL;

    SQLGetInfo(odbc->dbc, SQL_DBMS_NAME, (SQLPOINTER)info->dbms_name, sizeof(info->dbms_name), NULL);
    SQLGetInfo(odbc->dbc, SQL_DBMS_VER, (SQLPOINTER)info->dbms_ver, sizeof(info->dbms_ver), NULL);
    SQLGetInfo(odbc->dbc, SQL_GETDATA_EXTENSIONS, (SQLPOINTER)&getdata_support, 0, 0);
    SQLGetInfo(odbc->dbc, SQL_MAX_CONCURRENT_ACTIVITIES, &max_concur_act, 0, 0);

    if (max_concur_act == 0) strcpy(((char *) info->max_concur_act), "SQL_MAX_CONCURRENT_ACTIVITIES - no limit or undefined");
    else sprintf(((char *) info->max_concur_act), "SQL_MAX_CONCURRENT_ACTIVITIES = %u", max_concur_act);

    if (getdata_support & SQL_GD_ANY_ORDER) strcpy(((char *) info->getdata_support), "SQLGetData - columns can be retrieved in any order\n");
    else strcpy(((char *) info->getdata_support), "SQLGetData - columns must be retrieved in order\n");

    if (getdata_support & SQL_GD_ANY_COLUMN) strcat(((char *) info->getdata_support), "SQLGetData - can retrieve columns before last bound one");
    else strcat(((char *) info->getdata_support), "SQLGetData - columns must be retrieved after last bound one");

    return info;
}

/*Return Database Name*/
char * return_db_name (DB_INFO * info){
    return info && strlen((char *) info->dbms_name) > 0 ? ((char *) info->dbms_name) : NULL;
}

/*Return Database Version*/
char * return_db_version (DB_INFO * info){
    return info && strlen((char *) info->dbms_ver) > 0 ? ((char *) info->dbms_ver) : NULL;
}

/*Return SQL_MAX_CONCURRENT_ACTIVITIES*/
char * return_max_concur_act (DB_INFO * info){
    return info && strlen((char *) info->max_concur_act) > 0 ? ((char *) info->max_concur_act) : NULL;
}

/*Return SQL_GETDATA_EXTENSIONS*/
char * return_getdata_support (DB_INFO * info){
    return info && strlen((char *) info->getdata_support) > 0 ? ((char *) info->getdata_support) : NULL;
}

/*Query handler fro UPDATE, DELETE, INSERT*/
TABLE_RESULT * table_update (ODBC *odbc, char *queryStr){

    /*Initialize the pointers */
    TABLE_RESULT * result = NULL;

    result = (TABLE_RESULT * )malloc(sizeof(TABLE_RESULT));
    result->head = NULL;
    result->size = NULL;
    result->error_head = NULL;

    SQLHSTMT stmt;
    SQLRETURN ret; /* ODBC API return status */

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &stmt);

    /* Loop through the rows in the result-set */
    ret = SQLExecDirect(stmt, (SQLCHAR*) queryStr, SQL_NTS);

    if (!SQL_SUCCEEDED(ret))  result->error_head = get_error_fetching("Run SQL", stmt, SQL_HANDLE_STMT);
    if (stmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

/*Initialize Table row*/
TABLE_ROW * row_init (){
    TABLE_ROW * row = (TABLE_ROW *)malloc(sizeof(TABLE_ROW));
    if (!row) return NULL;
    row->col_head = NULL;
    row->next = NULL;

    return row;
}

/*Initialize table col*/
TABLE_COL * col_init (){
    TABLE_COL * col = (TABLE_COL *)malloc(sizeof(TABLE_COL));
    if(!col) return NULL;
    col->next = NULL;
    col->col_content = NULL;

    return col;
}

/*Query Handler for SELECT and data fetching*/
TABLE_RESULT * table_fetch (ODBC *odbc, char *queryStr){


    /*Initialize the pointers */
    TABLE_RESULT * result = NULL;

    /*Table size temp storage*/
    TABLE_SIZE * table_size = NULL;

    TABLE_ROW * row_head = NULL;
    TABLE_ROW * row_tail = NULL;
    TABLE_ROW * row_temp = NULL;
    TABLE_COL * col_head = NULL;
    TABLE_COL * col_tail = NULL;
    TABLE_COL * col_temp = NULL;
    COL_INFO * info_head = NULL;
    COL_INFO * info_tail = NULL;
    COL_INFO * info_temp = NULL;

    SQLHSTMT stmt;
    SQLRETURN ret; /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */
    int row = 1;
    SQLUSMALLINT i;

    result = (TABLE_RESULT * )malloc(sizeof(TABLE_RESULT)); if (!result) return NULL;
    result->head = NULL;
    result->size = NULL;
    result->col_info = NULL;
    result->error_head = NULL;

    /* Allocate statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &stmt);

    // Prepare Statement (my change to free format input by user)
    ret = SQLPrepare (stmt, (SQLCHAR*) queryStr, SQL_NTS);

    if (SQL_SUCCEEDED(ret))
        ret = SQLNumResultCols(stmt, &columns);
    else {
        result->error_head = get_error_fetching("Prepare SQL", stmt, SQL_HANDLE_STMT);
        if (stmt != SQL_NULL_HSTMT)
              SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return result;
    }

    /*Fetch detailed infomation for each column*/
    for (i=1;i<=columns;i++) {
        SQLCHAR *      ColumnName = NULL;
        SQLSMALLINT    ColumnNameLen;
        SQLSMALLINT    ColumnDataType;
        SQLULEN        ColumnDataSize;
        SQLSMALLINT    ColumnDataDigits;
        SQLSMALLINT    ColumnDataNullable;
        SQLCHAR *      ColumnData = NULL;
        SQLLEN         ColumnDataLen;

        ColumnName = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);
        ret = SQLDescribeCol (
                              stmt,                  // Select Statement (Prepared)
                              i,                     // Columnn Number
                              ColumnName,            // Column Name (returned)
                              MAX_COL_NAME_LEN,      // size of Column Name buffer
                              &ColumnNameLen,        // Actual size of column name
                              &ColumnDataType,       // SQL Data type of column
                              &ColumnDataSize,       // Data size of column in table
                              &ColumnDataDigits,     // Number of decimal digits
                              &ColumnDataNullable    // Whether column nullable
        );


        /*TODO: Fix "Program type out of range" error*/

        // ColumnData = (SQLCHAR *) malloc (ColumnDataSize+1);
        // ret = SQLBindCol (
        //                   stmt,                        // Statement handle
        //                   i,                           // Column number
        //                   (double)ColumnDataType,      // C Data Type
        //                   ColumnData,                  // Data buffer
        //                   ColumnDataSize,              // Size of Data Buffer
        //                   ColumnDataLen                // Size of data returned
        // );

        // if (!SQL_SUCCEEDED(ret)){
        //     result->error_head = get_error_fetching("Bind Column", stmt, SQL_HANDLE_STMT);
        //     if (stmt != SQL_NULL_HSTMT)
        //         SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        //     return result;
        // }

        /*Store the column infomation*/
        info_temp = (COL_INFO * )malloc(sizeof(COL_INFO)); if (!info_temp) return NULL;
        info_temp->id = i;
        info_temp->name = (char *)malloc(ColumnNameLen * sizeof(char)); if (!info_temp->name) return NULL;

        /*TODO: Replace strcpy() with memcpy()*/

        strcpy (info_temp->name, ((char *) ColumnName));
        //printf("%s\n", ColumnName);
        info_temp->type = (int)ColumnDataType;
        info_temp->size = (int)ColumnDataSize;
        info_temp->decimal = (int)ColumnDataDigits;
        info_temp->nullable = (int)ColumnDataNullable;
        info_temp->next = NULL;

        if(!result->col_info){
            result->col_info = (COL_INFO * )malloc(sizeof(COL_INFO)); if (!result->col_info) return NULL;
            result->col_info  = info_temp;
            result->col_info->next = NULL;
            info_tail = info_head = result->col_info;
        }else{
            info_tail->next = info_temp;
            info_tail = info_tail->next;
            info_tail->next = NULL;
        }
    }//for


    /* Executing*/
    ret = SQLExecDirect(stmt, (SQLCHAR*) queryStr, SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {

        //ret = SQLNumResultCols(stmt, &columns);

        while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {

            //check if it's the first node
            if(row_head == NULL){
                row_head = row_init();
                row_head->col_head = NULL;
                row_head->row_num = row;
                row_head->next = NULL;
                row_tail = row_head;
                row_tail->row_num = row;
                row_tail->col_head = NULL;
                row_tail->next = NULL;
            }else{
                row_temp = row_init();
                row_temp ->row_num = row;
                row_temp->col_head = NULL;
                row_temp ->next = NULL;
                row_tail->next = row_temp;
                row_tail = row_tail->next;
                row_tail->next = NULL;
            }
            row++;

            /* Loop through the columns */
            for (i = 1; i <= columns; i++) {
                SQLLEN indicator;
                char buf[512];
                /* retrieve column data as a string */
                ret = SQLGetData(stmt, i, SQL_C_CHAR, buf, sizeof(buf), &indicator);
                if (SQL_SUCCEEDED(ret)) {
                    /* Handle null columns */
                    if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");

                    /*Set up a node that contain the current content*/
                    col_temp  = col_init();
                    col_temp->col_content = (char *)malloc(520 * sizeof(char));

                    /*TODO: Replace strcpy() with memcpy()*/

                    strcpy (col_temp ->col_content, buf);
                    col_temp->col_num = i;
                    col_temp->next = NULL;

                    /*first check the head*/
                    if(!row_tail->col_head){
                        col_head = col_temp;
                        col_head->next = NULL;
                        row_tail->col_head = col_head;
                        col_tail = col_head;
                        col_tail-> next = NULL;
                    }else{
                        col_tail->next = col_temp;
                        col_tail = col_tail->next;
                        col_tail->next = NULL;
                    }
                }else{
                    result->error_head = get_error_fetching("Fetch Data", stmt, SQL_HANDLE_STMT);
                    if (stmt != SQL_NULL_HSTMT)
                            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
                    return result;
                }
            }//end of for
            col_head = NULL;
            col_tail = NULL;
            row_temp = NULL;
            col_temp = NULL;
        }
    }else{
        result->error_head = get_error_fetching("Run SQL", stmt, SQL_HANDLE_STMT);
        if (stmt != SQL_NULL_HSTMT)
                SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return result;
    }

    table_size = (TABLE_SIZE *)malloc(sizeof(TABLE_SIZE));
    table_size->row = row;
    table_size->col = columns;

    result->head = row_head;
    result->size = table_size;

    /*Free Statement handler*/
    if (stmt != SQL_NULL_HSTMT)
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

/*Get total number for rows for the table*/
int total_row (TABLE_RESULT * result){
    return result->size ? result->size->row : 0;
}

/*Get total number of columns for the table*/
int total_col(TABLE_RESULT * result){
    return result->size ? result->size->col : 0;
}

/*function that can return the string for a specific item*/
char * item_fetch(int row, int column, TABLE_RESULT * result){
    TABLE_COL * item = item_fetch_helper(row,column,result);
    return item ? item->col_content : NULL;
}

TABLE_COL * item_fetch_helper(int row, int column, TABLE_RESULT * result){
    if(!result || !result->head) return NULL;
    TABLE_ROW * head = result->head;
    TABLE_ROW * rtemp = head;
    while(rtemp){
        if(rtemp->row_num == row){
            TABLE_COL * ctemp = rtemp->col_head;
            if(!ctemp) return NULL;
            while(ctemp){
                if(ctemp->col_num == column) return ctemp;
                ctemp = ctemp->next;
            }
        }
        rtemp = rtemp->next;
    }
    return NULL;
}

int col_data_type_fetch(int column, TABLE_RESULT * result){
    COL_INFO * info = col_info_fetch_helper(column, result);
    return info ? (int)info->type : -999999;

}

char * col_name_fetch (int column, TABLE_RESULT * result){
    COL_INFO * info = col_info_fetch_helper(column, result);
    return info ? info->name : "Unknown";
}

COL_INFO * col_info_fetch_helper(int column, TABLE_RESULT * result){
    if(!result || !result->col_info) return NULL;
    COL_INFO * info = result->col_info;
    while(info){
        if(info->id == column) return info;
        info = info->next;
    }
    return NULL;
}

/*Generate fetching error*/
FETCHING_ERROR * get_error_fetching(char *fn, SQLHANDLE handle, SQLSMALLINT type)
{

    SQLINTEGER	 i = 0;
    SQLINTEGER	 native;
    SQLCHAR	 state[7];
    SQLCHAR	 text[256];
    SQLSMALLINT	 len;
    SQLRETURN	 ret;

    FETCHING_ERROR * head = NULL;
    FETCHING_ERROR * tail = NULL;
    FETCHING_ERROR * temp = NULL;

    /*Fetching Error*/
    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text,
                            sizeof(text), &len );
        if (SQL_SUCCEEDED(ret)){

            if(!head){
                head = (FETCHING_ERROR *)malloc(sizeof(FETCHING_ERROR));
                head->next = NULL;
                head->native = native;
                head->state = (char *)malloc(7 * sizeof(char));
                head->description = (char *)malloc(256 * sizeof(char));
                strcpy(head->state, ((char *) state));
                strcpy(head->description, ((char *) text));
                tail = head;
            }else{
                temp = (FETCHING_ERROR *)malloc(sizeof(FETCHING_ERROR));
                temp->next = NULL;
                temp->native = native;
                temp->state = (char *)malloc(7 * sizeof(char));
                temp->description = (char *)malloc(256 * sizeof(char));
                strcpy(temp->state, ((char *) state));
                strcpy(temp->description, ((char *) text));
                tail->next = temp;
                tail = tail->next;
                tail->next = NULL;
                temp = NULL;
            }
        }
    }
    while( ret == SQL_SUCCESS );
    return head;
}

/*Check Query Error*/
bool sql_error_found (TABLE_RESULT * head){
    return head && head->error_head ? true : false;
}

char * state_sql_error (TABLE_RESULT * head) {
    return !head || !head->error_head ? "99999" : head->error_head->state;
}

char * description_sql_error (TABLE_RESULT * head){
    return !head || !head->error_head ? "UnKnown Error.\n" : head->error_head->description;
}

int native_sql_error (TABLE_RESULT * head){
    return !head || !head->error_head ? -99999 : head->error_head->native;
}

TABLE_RESULT * free_sql_error_node(TABLE_RESULT * head){

    if(!head) return NULL;
    if(!head->error_head) return head;

    FETCHING_ERROR * temp = head->error_head;
    head->error_head = head->error_head->next;
    temp->next = NULL;
    free(temp->description);
    free(temp->state);
    free(temp);
    temp = NULL;
    return head;
}

/*Clear the table data*/
void table_clear (TABLE_RESULT * result) {
    if(!result) return;

    /*Free column info*/
    COL_INFO * info = result->col_info;
    while(info){
        COL_INFO * temp = info;
        info = info->next;
        free(temp->name); temp->name = NULL;
        free(temp); temp = NULL;
    }

    /*Free column content*/
    TABLE_ROW *row = result->head;
    while(row){
        TABLE_COL *col = row->col_head;
        while (col) {
            TABLE_COL * ctemp = col;
            col = col->next;
            free(ctemp->col_content); ctemp->col_content = NULL;
            free(ctemp);
        }
        TABLE_ROW * rtemp = row;
        row = row->next;
        rtemp->col_head = NULL;
        free(rtemp);
    }
    free(result->size); result->size = NULL;
    result->head = NULL;
    free(result); result = NULL;
}

/*Disconnect Database*/
void db_disconnect(ODBC *odbc){

    if(odbc){
        /* free up allocated handles */
        SQLDisconnect(odbc->dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
        odbc->dbc = NULL;
        SQLFreeHandle(SQL_HANDLE_ENV, odbc->env);
        odbc->env = NULL;
        free(odbc);
        odbc = NULL;
    }
}
