#include <string.h>
#include "sbn_loader.h"
#include "sbn_app.h"

/**
 * Reads a file describing the interface modules that
 * must be loaded.
 *
 * The format is the same as that for the peer file.
 */
int32 SBN_ReadModuleFile(void) {
    static char SBN_ModuleData[SBN_MODULE_FILE_LINE_SIZE];  /* A buffer for a line in a file */
    int BuffLen; /* Length of the current buffer */
    int ModuleFile = 0;
    char c;
    uint32 LineNum = 0;

    ModuleFile = OS_open(SBN_NONVOL_MODULE_FILENAME, OS_READ_ONLY, 0);

    if(ModuleFile == OS_ERROR) {
        return SBN_ERROR;
    }

    memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SIZE);
    BuffLen = 0;

    /*
     ** Parse the lines from the file
     */
    while(1) {
        OS_read(ModuleFile, &c, 1);

        if (c == '!')
            break;

        if (c == '\n' || c == ' ' || c == '\t')
        {
            /*
             ** Skip all white space in the file
             */
            ;
        }
        else if (c == ',')
        {
            /*
             ** replace the field delimiter with a space
             ** This is used for the sscanf string parsing
             */
            SBN_ModuleData[BuffLen] = ' ';
            if (BuffLen < (SBN_MODULE_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else if (c != ';')
        {
            /*
             ** Regular data gets copied in
             */
            SBN_ModuleData[BuffLen] = c;
            if (BuffLen < (SBN_MODULE_FILE_LINE_SIZE - 1))
                BuffLen++;
        }
        else
        {
            /*
             ** Send the line to the file parser
             */
            if (SBN_ParseModuleEntry(SBN_ModuleData, LineNum) == -1)
                return SBN_ERROR;
            LineNum++;
            memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SIZE);
            BuffLen = 0;
        }

    }/* end while */

    OS_close(ModuleFile);

    return SBN_OK;
}

/**
 * Parses a module file entry to obtain the protocol id, name,
 * path, and function structure for an interface type.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the module file

 * @return  SBN_OK on success, SBN_ERROR on error
 */
int32 SBN_ParseModuleEntry(char *FileEntry, uint32 LineNum) {
    uint32  ProtocolId;
    char    ModuleName[50];
    char    ModuleFile[50];
    char    StructName[50];
    int     ScanfStatus;

    int32   ReturnCode;
    uint32  ModuleId;
    uint32  StructAddr;

    /* switch on protocol ID */
    ScanfStatus = sscanf(FileEntry, "%lu %s %s %s", &ProtocolId, ModuleName, ModuleFile, StructName);

    /*
    ** Check to see if the correct number of items were parsed
    */
    if (ScanfStatus != 4) {

        /*CFE_EVS_SendEvent(SBN_INV_LINE_EID,CFE_EVS_ERROR,
                          "%s:Invalid SBN module file line,exp %d items,found %d",
                          CFE_CPU_NAME, 4, ScanfStatus); */
        printf("%s:Invalid SBN module file line,exp %d items,found %d",
                          CFE_CPU_NAME, 4, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    ReturnCode = OS_ModuleLoad(&ModuleId, ModuleName, ModuleFile);

    if(ReturnCode != OS_SUCCESS) {
        return SBN_ERROR;
    }

    ReturnCode = OS_SymbolLookup(&StructAddr, StructName);

    SBN.IfOps[ProtocolId] = (SBN_InterfaceOperations *)StructAddr;

    if(ReturnCode != OS_SUCCESS) {
        printf("Failed to find symbol %s\n", StructName);
        return SBN_ERROR;
    }

    return SBN_OK;
}
