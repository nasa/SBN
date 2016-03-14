#include <string.h>
#include "sbn_app.h"
#include "sbn_main_events.h"
#include "sbn_loader.h"

/**
 * Reads a file describing the interface modules that
 * must be loaded.
 *
 * The format is the same as that for the peer file.
 */
int32 SBN_ReadModuleFile(void)
{
    /* A buffer for a line in a file */
    static char     SBN_ModuleData[SBN_MODULE_FILE_LINE_SIZE];
    int             BuffLen = 0, /* Length of the current buffer */
                    ModuleFile = 0;
    char            c = '\0';
    uint32          LineNum = 0;

    DEBUG_START();

    ModuleFile = OS_open(SBN_NONVOL_MODULE_FILENAME, OS_READ_ONLY, 0);

    if(ModuleFile == OS_ERROR)
    {
        return SBN_ERROR;
    }/* end if */

    memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SIZE);
    BuffLen = 0;

    /*
     ** Parse the lines from the file
     */
    while(1)
    {
        OS_read(ModuleFile, &c, 1);

        if (c == '!')
        {
            break;
        }/* end if */

        switch(c)
        {
            case '\n':
            case ' ':
            case '\t':
                /* ignore whitespace */
                break;

            case ',':
                /*
                 ** replace the field delimiter with a space
                 ** This is used for the sscanf string parsing
                 */
                SBN_ModuleData[BuffLen] = ' ';
                if (BuffLen < (SBN_MODULE_FILE_LINE_SIZE - 1))
                {
                    BuffLen++;
                }/* end if */
                break;
            case ';':
                /* Send the line to the file parser */
                if (SBN_ParseModuleEntry(SBN_ModuleData, LineNum) == -1)
                    return SBN_ERROR;
                LineNum++;
                memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SIZE);
                BuffLen = 0;
                break;
            default:
                /* Regular data gets copied in */
                SBN_ModuleData[BuffLen] = c;
                if (BuffLen < (SBN_MODULE_FILE_LINE_SIZE - 1))
                {
                    BuffLen++;
                }/* end if */
        }/* end switch */
    }/* end while */

    OS_close(ModuleFile);

    return SBN_OK;
}/* end SBN_ReadModuleFile */

/**
 * Parses a module file entry to obtain the protocol id, name,
 * path, and function structure for an interface type.
 *
 * @param FileEntry  Interface description line as read from file
 * @param LineNum    The line number in the module file

 * @return  SBN_OK on success, SBN_ERROR on error
 */
int32 SBN_ParseModuleEntry(char *FileEntry, uint32 LineNum)
{
    uint32  ProtocolId = 0;
    char    ModuleName[50];
    char    ModuleFile[50];
    char    StructName[50];
    int     ScanfStatus = 0;
    int ProtocolIdInt = 0;

    int32   ReturnCode = 0;
    uint32  ModuleId = 0;
    cpuaddr StructAddr = 0;

    DEBUG_START();

    /* switch on protocol ID */
    ScanfStatus = sscanf(FileEntry, "%d %s %s %s",
        &ProtocolIdInt, ModuleName, ModuleFile, StructName);
    ProtocolId = ProtocolIdInt;

    /*
    ** Check to see if the correct number of items were parsed
    */
    if (ScanfStatus != 4)
    {
        OS_printf("%s:Invalid SBN module file line,exp %d items,found %d",
            CFE_CPU_NAME, 4, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    ReturnCode = OS_ModuleLoad(&ModuleId, ModuleName, ModuleFile);

    if(ReturnCode != OS_SUCCESS)
    {
        return SBN_ERROR;
    }/* end if */

    ReturnCode = OS_SymbolLookup(&StructAddr, StructName);

    if(ReturnCode != OS_SUCCESS)
    {
        OS_printf("Failed to find symbol %s\n", StructName);
        return SBN_ERROR;
    }/* end if */

    SBN.IfOps[ProtocolId] = StructAddr;

    return SBN_OK;
}/* end SBN_ParseModuleEntry */
