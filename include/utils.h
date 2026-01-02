#ifndef UTILS_H
#define UTILS_H

#include "tt_colors.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASSET_PATH
#define ASSET_PATH "A:../assets/"
#endif

#define ASSET(img) ((img == NULL) ? NULL : ASSET_PATH img)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UTILS_H */
