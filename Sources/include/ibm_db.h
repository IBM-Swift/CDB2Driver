#ifndef odbc_connect_h
#define odbc_connect_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#include <sql.h>
#include <sqlext.h>
#elif __APPLE__
#include "/usr/local/include/sql.h"
#include "/usr/local/include/sqlext.h"
#endif

#include <string.h>

#define MAX_COL_NAME_LEN  512

/*Define Connection Error Struct*/
typedef struct conn_error {
    char * state;
    int native;
    char * description;
    struct conn_error * next;
}CONNECTION_ERROR;

/*Define Connection Struct*/
typedef struct odbc_handler {
    SQLHDBC dbc;
    SQLHENV env;
    CONNECTION_ERROR * error_head;
} ODBC;

typedef struct db_info {
    SQLCHAR dbms_name[256];
    SQLCHAR dbms_ver[256];
    SQLCHAR getdata_support[256];
    SQLCHAR max_concur_act[256];
}DB_INFO;

/*Define Table Column Struct*/
typedef struct table_col {
    int col_num;
    char * col_content;
    struct table_col * next;
} TABLE_COL;

/*Define Table Row Struct*/
typedef struct table_row {
    int row_num;
    struct table_col * col_head;
    struct table_row * next;
} TABLE_ROW;

/*Define Table Size Struct*/
typedef struct table_size {
    int row;
    int col;
} TABLE_SIZE;

/*Define Query Error Struct*/
typedef struct fetching_error{
    char * state;
    int native;
    char * description;
    struct fetching_error * next;
}FETCHING_ERROR;

typedef struct col_info{
    int id;
    char * name;
    int type;
    int size;
    int decimal;
    int nullable;
    struct col_info * next;
}COL_INFO;

/*Desine Query Result Struct*/
typedef struct table_result{
    TABLE_SIZE * size;
    COL_INFO * col_info;
    TABLE_ROW * head;
    FETCHING_ERROR * error_head;
}TABLE_RESULT;

/*Initialize the functions*/

/*Initialize ODBC Handler*/
ODBC * init_handlers();
/*Setup ODBC Connection*/
ODBC * db_connect(ODBC *odbc, char *connStr);

/*Disconnect the Database*/
void db_disconnect(ODBC *odbc);

/*ODBC Connection Error Handle*/
CONNECTION_ERROR * get_error_connection(char *fn, SQLHANDLE handle, SQLSMALLINT type);
bool connection_error_found (ODBC * head);
char * state_connection_error (ODBC * head);
char * description_connection_error (ODBC * head);
int native_connection_error (ODBC * head);
ODBC * free_connection_error_node(ODBC * head);

/*Get SQL Database Info */
DB_INFO * db_info (ODBC *odbc);
char * return_db_name (DB_INFO * info);
char * return_db_version (DB_INFO * info);
char * return_max_concur_act (DB_INFO * info);
char * return_getdata_support (DB_INFO * info);

/*Query Handler for "UPDATE, DELETE, INSERT etc."*/
TABLE_RESULT * table_update (ODBC *odbc, char *queryStr);

/*Query Handler for "SELECT"*/
TABLE_RESULT * table_fetch (ODBC *odbc, char *queryStr);
TABLE_ROW * row_init ();
TABLE_COL * col_init ();
char * item_fetch(int row, int column, TABLE_RESULT * result);
TABLE_COL * item_fetch_helper(int row, int column, TABLE_RESULT * result);
int col_data_type_fetch(int column, TABLE_RESULT * result);
COL_INFO * col_info_fetch_helper(int column, TABLE_RESULT * result);
char * col_name_fetch (int column, TABLE_RESULT * result);
int total_row (TABLE_RESULT * result);
int total_col(TABLE_RESULT * result);

/*Clear All Temp Data*/
void table_clear (TABLE_RESULT * head);

/*SQL Error Handle*/
FETCHING_ERROR * get_error_fetching(char *fn, SQLHANDLE handle, SQLSMALLINT type);
bool sql_error_found (TABLE_RESULT * head);
char * state_sql_error (TABLE_RESULT * head);
char * description_sql_error (TABLE_RESULT * head);
int native_sql_error (TABLE_RESULT * head);
TABLE_RESULT * free_sql_error_node(TABLE_RESULT * head);


#endif /* odbc_connect_h */
