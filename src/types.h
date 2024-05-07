#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>

#include <stdint.h>
#include <stddef.h>

#define MAX_ASSET_NAME_LENGTH 32
#define MAX_ASSET_FILENAME_LENGTH 1028
typedef struct RSP_Sprite {
    char name[MAX_ASSET_NAME_LENGTH];
    char file[MAX_ASSET_FILENAME_LENGTH];
    // TODO: Add flags to only save 'altered' data
    // TODO: Move to Image to avoid GPU memory usage
    Texture2D texture;

    Rectangle source;

    Rectangle* animation_frames;
    int animation_frame_count;

    Vector2 origin;
} RSP_Sprite;

#define MAX_PROJECT_NAME_LENGTH 32
typedef struct RSP_Project {
    int version;
    char name[MAX_PROJECT_NAME_LENGTH];

    RSP_Sprite* sprites;

    struct {
        int size;
        size_t sprite_count;

        RenderTexture2D texture;
    } atlas;

    FilePathList files;
} RSP_Project;

typedef enum RSP_ProjectError {
    RSP_PROJECT_ERROR_NONE,
    RSP_PROJECT_ERROR_EXISTS,
    RSP_PROJECT_ERROR_NOT_EXIST,
    RSP_PROJECT_ERROR_INVALID,
} RSP_ProjectError;

typedef enum RSP_BundleError {
    RSP_BUNDLE_ERROR_NONE
} RSP_BundleError;

#endif // TYPES_H
