#ifndef PROJECT_H
#define PROJECT_H

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

RSP_ProjectError RSP_CreateEmptyProject (const char* project_name, RSP_Project* project);
RSP_ProjectError RSP_LoadProject (const char* project_name, RSP_Project* project);

void RSP_UnloadProject (RSP_Project* project);

#ifdef __cplusplus
}
#endif


#endif // PROJECT_H
