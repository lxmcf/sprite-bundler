#ifndef BUNDLE_H
#define BUNDLE_H

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

RSP_BundleError RSP_ExportBundle (const char* filename, RSP_Project* project_in);

#ifdef __cplusplus
}
#endif


#endif // BUNDKLE_H
