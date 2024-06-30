#ifndef RSP_H
#define RSP_H

#include <raylib.h>
#include <stdint.h>

#define MAX_SPRITE_NAME_LENGTH 32

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
    int id;

    Texture2D atlas;

    Sprite* sprites;
    uint16_t sprites_count;
} SpriteBundle;

#ifdef __cplusplus
extern "C" {
#endif

int GetSpriteId (const char* name);
void DrawSprite (int id, Vector2 position, Color colour);
void DrawSpriteEx (int id, Vector2 position, Vector2 scale, float rotation, Color colour);

Vector2 GetSpriteOrigin (int id);
void SetSpriteOrigin (int id, Vector2 origin);

Vector2 GetSpriteSize (int id);
const char* GetSpriteName (int id);

SpriteBundle LoadBundle (const char* filename);
void SetActiveBundle (SpriteBundle* bundle);
int IsBundleReady (SpriteBundle bundle);

void UnloadBundle (SpriteBundle bundle);

// #define RSP_IMPLEMENTATION // Used for debugging
#ifdef RSP_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#define HEADER_SIZE 4

#define RSP_SPRITE_ANIMATED 1 << 0
#define RSP_SPRITE_ORIGIN 1 << 1

// -----------------------------------------------------------------------------
// PRIVATE DATA
// -----------------------------------------------------------------------------
static SpriteBundle* rsp__current_bundle = NULL;
static int rsp__bundles_loaded = 0;

// Taken from https://benhoyt.com/writings/hash-table-in-c/
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

// -----------------------------------------------------------------------------
// USER METHODS
// -----------------------------------------------------------------------------
int GetSpriteId (const char* name) {
    if (rsp__current_bundle == NULL) return -1;

    uint64_t hash = rsp__hash (name);

    for (size_t i = 0; i < rsp__current_bundle->sprites_count; i++) {
        if (hash == rsp__current_bundle->sprites[i].hash) return i;
    }

    return -1;
}

void DrawSprite (int id, Vector2 position, Color colour) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return;
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    DrawTexturePro (rsp__current_bundle->atlas, sprite->source, CLITERAL(Rectangle){ position.x, position.y, sprite->source.width, sprite->source.height }, sprite->origin, 0.0f, colour);
}

void DrawSpriteEx (int id, Vector2 position, Vector2 scale, float rotation, Color colour) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return;
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    Rectangle output = CLITERAL(Rectangle){
        .x = position.x,
        .y = position.y,
        .width = sprite->source.width * scale.x,
        .height = sprite->source.height * scale.y
    };

    Vector2 adjusted_origin = CLITERAL(Vector2) {
        sprite->origin.x * scale.x,
        sprite->origin.y * scale.y
    };

    DrawTexturePro (rsp__current_bundle->atlas, sprite->source, output, adjusted_origin, rotation, colour);
}

Vector2 GetSpriteOrigin (int id) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return CLITERAL(Vector2){ 0, 0 };;
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    return sprite->origin;
}

void SetSpriteOrigin (int id, Vector2 origin) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return;
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    sprite->origin = origin;
}

Vector2 GetSpriteSize (int id) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return CLITERAL(Vector2){ 0, 0 };
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    return CLITERAL(Vector2) { sprite->source.width, sprite->source.height };
}

const char* GetSpriteName (int id) {
    if (rsp__current_bundle == NULL || id > rsp__current_bundle->sprites_count) return "null";
    Sprite* sprite = &rsp__current_bundle->sprites[id];

    return sprite->name;
}

// -----------------------------------------------------------------------------
// CORE METHODS
// -----------------------------------------------------------------------------
SpriteBundle LoadBundle (const char* filename) {
    SpriteBundle bundle = { 0 };

    FILE* bundle_info = fopen (filename, "rb");
    if (!bundle_info) {
        TraceLog (LOG_ERROR, "BUNDLE: [%s] Failed to open file", filename);
        goto bundle_close;
    }

    char* file_type = RL_MALLOC (sizeof (char) * 4);
    fread (file_type, sizeof (char), 4, bundle_info);

    if (!TextIsEqual (file_type, "RSPX")) {
        TraceLog (LOG_ERROR, "BUNDLE: [%s] File was not a sprite bundle", file_type);
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
            TraceLog (LOG_ERROR, "BUNDLE: [%s] Expected [RSP] header", header);
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

    bundle.id = ++rsp__bundles_loaded;

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

    UnloadTexture (bundle.atlas);

    TraceLog (LOG_INFO, "BUNDLE: [ID %d] Sprite bundle unloaded successfully", bundle.id);

    rsp__bundles_loaded--;
}

#undef HEADER_SIZE

#undef RSP_SPRITE_ANIMATED
#undef RSP_SPRITE_ORIGIN

#endif // RSP_IMPLEMENTATION

#ifdef __cplusplus
}
#endif // __cplusplus

#undef MAX_SPRITE_NAME_LENGTH

#endif // RSP_H
