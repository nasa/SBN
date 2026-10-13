#include "cfe_tbl_filedef.h" /* Required to obtain the CFE_TBL_FILEDEF macro definition */
#include "listener_app_tbl.h"

/*
** The following is an example of the declaration statement that defines the desired
** contents of the table image.
*/
LISTENER_APP_ExampleTable_t ExampleTable = {1, 2};

/*
** The macro below identifies:
**    1) the data structure type to use as the table image format
**    2) the name of the table to be placed into the cFE Example Table File Header
**    3) a brief description of the contents of the file image
**    4) the desired name of the table image binary file that is cFE compatible
*/
CFE_TBL_FILEDEF(ExampleTable, LISTENER_APP.ExampleTable, Table Utility Test Table, listen_app_tbl.tbl)
