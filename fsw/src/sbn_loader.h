#ifndef _sbn_loader_h_
#define _sbn_loader_h_

#include "cfe.h"

int32 SBN_ReadModuleFile(void);
int32 SBN_ParseModuleEntry(char *FileEntry, uint32 LineNum);
void SBN_UnloadModules(void);

#endif /* _sbn_loader_h_ */
