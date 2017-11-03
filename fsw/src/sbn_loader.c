#include <string.h>
#include "sbn_app.h"

#ifdef CFE_ES_CONFLOADER

#include "cfe_es_conf.h"

/**
 * Handles a row's worth of fields from a configuration file.
 * @param[in] Filename The filename of the configuration file currently being
 *            processed.
 * @param[in] LineNum The line number of the line being processed.
 * @param[in] Header The section header (if any) for the configuration file
 *            section.
 * @param[in] Row The entries from the row.
 * @param[in] FieldCount The number of fields in the row array.
 * @param[in] Opaque The Opaque data passed through the parser.
 * @return OS_SUCCESS on successful loading of the configuration file row.
 */
static int ModuleRowCallback(const char *Filename, int LineNum,
    const char *Header, const char *Row[], int FieldCount, void *Opaque)
{
    int32   Status = 0;
    uint32  ModuleID = 0;
    cpuaddr StructAddr = 0;
    int ProtocolID = 0;

    if (FieldCount != 4)
    {
        OS_printf("SBN invalid field count %d (expected 4)\n", FieldCount); /* TODO: event */
        return OS_SUCCESS;
    }

    ProtocolID = atoi(Row[0]);

    if (ProtocolID < 0 || ProtocolID > SBN_MAX_INTERFACE_TYPES)
    {   
        OS_printf("SBN invalid protocol id %d\n", ProtocolID);
        return OS_SUCCESS;
    }/* end if */

    Status = OS_ModuleLoad(&ModuleID, Row[1], Row[2]);
    if(Status != OS_SUCCESS)
    {
        OS_printf("SBN: Unable to load module %s (%s)\n", Row[1], Row[2]);
        return OS_SUCCESS;
    }/* end if */

    Status = OS_SymbolLookup(&StructAddr, Row[3]);
    if(Status != OS_SUCCESS)
    {
        OS_printf("SBN failed to find symbol %s\n", Row[3]);
        return OS_SUCCESS;
    }/* end if */

    OS_printf("SBN: Module initialized: %s in %s (%s)\n",
        Row[3], Row[1], Row[2]);
    SBN.IfOps[ProtocolID] = (SBN_IfOps_t *)StructAddr;
    SBN.ModuleIDs[ProtocolID] = ModuleID;

    return OS_SUCCESS;
}

/**
 * When the configuration file loader encounters an error, it will call this
 * function with details.
 * @param[in] Filename The filename of the configuration file currently being
 *            processed.
 * @param[in] LineNum The line number of the line being processed.
 * @param[in] ErrMsg The textual description of the error.
 * @param[in] Opaque The Opaque data passed through the parser.
 * @return OS_SUCCESS
 */
static int ModuleErrCallback(const char *Filename, int LineNum,
    const char *ErrMsg, void *Opaque)
{
    OS_printf("SBN %s\n", ErrMsg); /* TODO: event */
    return OS_SUCCESS;
}

/**
 * Function to read in, using the CFE_ES_Conf API, the configuration file
 * entries.
 * @return OS_SUCCESS on successful completion.
 */
int32 SBN_ReadModuleFile(void)
{
    return CFE_ES_ConfLoad(SBN_NONVOL_MODULE_FILENAME, NULL, ModuleRowCallback,
        ModuleErrCallback, NULL);
}

#else /* ! CFE_ES_CONFLOADER */

/**
 * \brief Reads a file describing the interface modules that must be loaded.
 *
 * \note The format is the same as that for the peer file.
 *
 * @return SBN_SUCCESS on success.
 */
int32 SBN_ReadModuleFile(void)
{
    /* A buffer for a line in a file */
    static char     SBN_ModuleData[SBN_MODULE_FILE_LINE_SZ];
    int             BuffLen = 0, /* Length of the current buffer */
                    ModuleFile = 0;
    char            CurrentChar = '\0';
    uint32          LineNum = 0;

    ModuleFile = OS_open(SBN_NONVOL_MODULE_FILENAME, OS_READ_ONLY, 0);

    if(ModuleFile == OS_ERROR)
    {
        return SBN_ERROR;
    }/* end if */

    memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SZ);
    BuffLen = 0;

    /* Parse the lines from the file */
    while(1)
    {
        OS_read(ModuleFile, &CurrentChar, 1);

        if (CurrentChar == '!')
        {
            break;
        }/* end if */

        switch(CurrentChar)
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
                if (BuffLen < (SBN_MODULE_FILE_LINE_SZ - 1))
                {
                    BuffLen++;
                }/* end if */
                break;
            case ';':
                /* Send the line to the file parser */
                if (SBN_ParseModuleEntry(SBN_ModuleData, LineNum) == -1)
                    return SBN_ERROR;
                LineNum++;
                memset(SBN_ModuleData, 0x0, SBN_MODULE_FILE_LINE_SZ);
                BuffLen = 0;
                break;
            default:
                /* Regular data gets copied in */
                SBN_ModuleData[BuffLen] = CurrentChar;
                if (BuffLen < (SBN_MODULE_FILE_LINE_SZ - 1))
                {
                    BuffLen++;
                }/* end if */
        }/* end switch */
    }/* end while */

    OS_close(ModuleFile);

    return SBN_SUCCESS;
}/* end SBN_ReadModuleFile */

/**
 * Parses a module file entry to obtain the protocol id, name,
 * path, and function structure for an interface type.
 *
 * @param[in] FileEntry  Interface description line as read from file
 * @param[in] LineNum    The line number in the module file

 * @return  SBN_SUCCESS on success, SBN_ERROR on error
 */
int32 SBN_ParseModuleEntry(char *FileEntry, uint32 LineNum)
{
    int  ProtocolID = 0;
    char    ModuleName[50];
    char    ModuleFile[50];
    char    StructName[50];
    int     ScanfStatus = 0;

    int32   ReturnCode = 0;
    uint32  ModuleID = 0;
    cpuaddr StructAddr = 0;

    /* switch on protocol ID */
    ScanfStatus = sscanf(FileEntry, "%d %49s %49s %49s",
        &ProtocolID, ModuleName, ModuleFile, StructName);

    /*
    ** Check to see if the correct number of items were parsed
    */
    if (ScanfStatus != 4)
    {
        OS_printf("SBN invalid module file line (exp %d items, found %d)",
            4, ScanfStatus);
        return SBN_ERROR;
    }/* end if */

    if (ProtocolID < 0 || ProtocolID > SBN_MAX_INTERFACE_TYPES)
    {
        OS_printf("SBN invalid protocol id %d", ProtocolID);
        return SBN_ERROR;
    }/* end if */

    ReturnCode = OS_ModuleLoad(&ModuleID, ModuleName, ModuleFile);

    if(ReturnCode != OS_SUCCESS)
    {
        return SBN_ERROR;
    }/* end if */

    ReturnCode = OS_SymbolLookup(&StructAddr, StructName);

    if(ReturnCode != OS_SUCCESS)
    {
        OS_printf("SBN failed to find symbol %s\n", StructName);
        return SBN_ERROR;
    }/* end if */

    OS_printf("SBN found symbol %s in %s (%s)\n", StructName, ModuleName,
        ModuleFile);
    SBN.IfOps[ProtocolID] = (SBN_IfOps_t *)StructAddr;
    SBN.ModuleIDs[ProtocolID] = ModuleID;

    return SBN_SUCCESS;
}/* end SBN_ParseModuleEntry */

void SBN_UnloadModules(void)
{
    int ProtocolID = 0;

    for(ProtocolID = 0; ProtocolID < SBN_MAX_INTERFACE_TYPES; ProtocolID++)
    {
        if(SBN.ModuleIDs[ProtocolID])
        {
            if(OS_ModuleUnload(SBN.ModuleIDs[ProtocolID]) != OS_SUCCESS)
            {
                /* TODO: send event? */
                OS_printf("Unable to unload module ID %d for Protocol ID %d\n",
                    SBN.ModuleIDs[ProtocolID], ProtocolID);
            }/* end if */
        }/* end if */
    }/* end for */
}/* end SBN_UnloadModules */

#endif /* CFE_ES_CONFLOADER */
