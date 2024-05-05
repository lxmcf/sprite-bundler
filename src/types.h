#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>

#include <stdint.h>
#include <stddef.h>

#define MAX_ASSET_NAME_LENGTH 32
typedef struct RSP_Sprite {
    uint64_t hash;
    char name[MAX_ASSET_NAME_LENGTH];

    // TODO: Move to Image to avoid GPU memory usage
    Texture2D texture;

    Rectangle source;

    Rectangle* animation_frames;
    int animation_frame_count;

    Vector2 origin;
} RSP_Sprite;

#define MAX_PROJECT_NAME_LENGTH 32
typedef struct RSP_Project {
    char name[MAX_PROJECT_NAME_LENGTH];
    int version;

    FilePathList files;

    RSP_Sprite* sprites;
    size_t sprite_count;

    int texture_atlas_size;
    RenderTexture2D texture_atlas;
} RSP_Project;

typedef enum RSP_ProjectError {
    RSP_PROJECT_ERROR_NONE,
    RSP_PROJECT_ERROR_EXISTS,
    RSP_PROJECT_ERROR_NOT_EXIST
} RSP_ProjectError;

typedef enum RSP_BundleError {
    RSP_BUNDLE_ERROR_NONE
} RSP_BundleError;

#endif // TYPES_H
