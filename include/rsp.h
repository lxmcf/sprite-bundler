#ifndef RSP_H
#define RSP_H

#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

int GetSpriteId (const char* name);
void DrawSprite (int id, Vector2 position, Color color);

// #define RSP_IMPLEMENTATION
#ifdef RSP_IMPLEMENTATION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SPRITE_NAME_LENGTH 32
#define HEADER_SIZE 4

#define RSP_SPRITE_ANIMATED 1 << 0
#define RSP_SPRITE_ORIGIN 1 << 1

typedef struct Sprite {
    uint64_t hash;

    char name[MAX_SPRITE_NAME_LENGTH];

    uint16_t flags;
    Rectangle source;
    Vector2 origin;

    struct {
        Rectangle* frames;
        uint16_t frames_count;
    } animation; // Not used
} Sprite;

typedef struct SpriteBundle {
    Texture2D atlas;

    Sprite* sprites;
    uint16_t sprites_count;
} SpriteBundle;

static SpriteBundle* rsp__current_bundle = NULL;

static uint64_t rsp__hash (const char* key) {
    #define FNV_OFFSET 14695981039346656037UL
    #define FNV_PRIME 1099511628211UL

    uint64_t hash = FNV_OFFSET;

    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }

    return hash;
}

int GetSpriteId (const char* name) {
    if (rsp__current_bundle == NULL) return -1;

    uint64_t hash = rsp__hash (name);

    for (size_t i = 0; i < rsp__current_bundle->sprites_count; i++) {
        if (hash == rsp__current_bundle->sprites[i].hash) return i;
    }

    return -1;
}

void DrawSprite (int id, Vector2 position, Color color) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return;

    Sprite* sprite = &rsp__current_bundle->sprites[id];
    DrawTexturePro (rsp__current_bundle->atlas, sprite->source, CLITERAL(Rectangle){ position.x, position.y, sprite->source.width, sprite->source.height }, sprite->origin, 0.0f, color);
}

SpriteBundle LoadBundle (const char* filename);
void SetActiveBundle (SpriteBundle* bundle);
int IsBundleReady (SpriteBundle bundle);

void UnloadBundle (SpriteBundle bundle);

SpriteBundle LoadBundle (const char* filename) {
    SpriteBundle bundle = { 0 };

    FILE* bundle_info = fopen (filename, "rb");
    if (!bundle_info) {
        TraceLog (LOG_ERROR, "RSP: [%s] Failed to open file", filename);
        goto bundle_close;
    }

    int atlas_data_size_raw, atlas_data_size_compressed;

    fread (&bundle.sprites_count, sizeof (uint16_t), 1, bundle_info);
    fread (&atlas_data_size_compressed, sizeof (int32_t), 1, bundle_info);

    unsigned char* atlas_data_compressed = RL_CALLOC (atlas_data_size_compressed, sizeof (unsigned char));
    unsigned char* atlas_data_raw;

    fread (atlas_data_compressed, sizeof (unsigned char), atlas_data_size_compressed, bundle_info);
    atlas_data_raw = DecompressData (atlas_data_compressed, atlas_data_size_compressed, &atlas_data_size_raw);

    Image atlas_image = LoadImageFromMemory (".png", atlas_data_raw, atlas_data_size_raw);
    bundle.atlas = LoadTextureFromImage (atlas_image);

    UnloadImage (atlas_image);

    RL_FREE (atlas_data_compressed);
    RL_FREE (atlas_data_raw);

    bundle.sprites = RL_CALLOC (bundle.sprites_count, sizeof (Sprite));
    char* header = RL_CALLOC (HEADER_SIZE, sizeof (unsigned char));

    for (size_t i = 0; i < bundle.sprites_count; i++) {
        fread (header, sizeof (char), HEADER_SIZE, bundle_info);

        if (!TextIsEqual ("SPR", header)) {
            TraceLog (LOG_ERROR, "RSP: [%s] Expected [RSP] header", header);
            bundle.sprites_count = i;

            UnloadBundle (bundle);

            goto bundle_free;
        }

        Sprite* sprite = &bundle.sprites[i];

        fread (sprite->name, sizeof (char), MAX_SPRITE_NAME_LENGTH, bundle_info);
        fread (&sprite->flags, sizeof (uint16_t), 1, bundle_info);

        sprite->hash = rsp__hash (sprite->name);

        // Origin
        fread (&sprite->origin.x, sizeof (float), 1, bundle_info);
        fread (&sprite->origin.y, sizeof (float), 1, bundle_info);

        // Source
        fread (&sprite->source.x, sizeof (float), 1, bundle_info);
        fread (&sprite->source.y, sizeof (float), 1, bundle_info);
        fread (&sprite->source.width, sizeof (float), 1, bundle_info);
        fread (&sprite->source.height, sizeof (float), 1, bundle_info);

        if (sprite->flags & RSP_SPRITE_ANIMATED) {
            fread (&sprite->animation.frames_count, sizeof (uint16_t), 1, bundle_info);
            sprite->animation.frames = RL_CALLOC (sprite->animation.frames_count, sizeof (Rectangle));

            for (size_t j = 0; j < sprite->animation.frames_count; j++) {
                fread (&sprite->animation.frames[j].x, sizeof (float), 1, bundle_info);
                fread (&sprite->animation.frames[j].y, sizeof (float), 1, bundle_info);
                fread (&sprite->animation.frames[j].width, sizeof (float), 1, bundle_info);
                fread (&sprite->animation.frames[j].height, sizeof (float), 1, bundle_info);
            }
        }
    }

bundle_free:
    RL_FREE (header);

bundle_close:
    fclose (bundle_info);

    return bundle;
}

void SetActiveBundle (SpriteBundle* bundle) {
    rsp__current_bundle = bundle;
}

int IsBundleReady (SpriteBundle bundle) {
    int result = 0;

    if ((bundle.sprites_count != 0) && (IsTextureReady (bundle.atlas))) result = 1;

    return result;
}


void UnloadBundle (SpriteBundle bundle) {
    for (size_t i = 0; i < bundle.sprites_count; i++) {
        Sprite* sprite = &bundle.sprites[i];

        if (sprite->flags & RSP_SPRITE_ANIMATED) {
            RL_FREE (sprite->animation.frames);
        }
    }

    RL_FREE (bundle.sprites);
}

#undef MAX_SPRITE_NAME_LENGTH
#undef HEADER_SIZE

#undef RSP_SPRITE_ANIMATED
#undef RSP_SPRITE_ORIGIN

#endif // RSP_IMPLEMENTATION

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RSP_H
