// TODO: Move to something more dedicated for rectangle packing

#include "types.h"

#include <raymath.h>
#include <raygui.h>

#include <string.h>
#include <stdlib.h>

#define WGT_TOOLBAR_IMPL
#include "widgets/toolbar.h"

#include "project/project.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define WINDOW_TITLE "Hello World"

#define ATLAS_SIZE 1024
#define ATLAS_ITEM_MIN_SIZE 16

static RSP_Sprite* sprites;
static FilePathList files;

static RenderTexture2D texture_atlas;

static Rectangle* currently_selected_rectangle;
static RSP_Sprite* currently_selected_sprite;

static WGT_ToolbarState toolbar_state;

static uint64_t hash (const char* key);
static int CompareTextureRectangles (const void* a, const void* b);
static void LoadAndFilterAssets (void);
static void RenderRectangleSizes (void);

static void CopyFile (const char* source, const char* destination);

int main (int argc, const char* argv[]) {
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    SetWindowState (FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize (WINDOW_WIDTH, WINDOW_HEIGHT);

    toolbar_state = WGT_InitToolbarState ();

    {   // Set framerate to max
        #define DEFAULT_TARGET_FPS 60
        int current_monitor = GetCurrentMonitor ();
        int fps = GetMonitorRefreshRate (current_monitor);

        // Fallback to 60 FPS target if error occurs
        if (fps <= 0) fps = DEFAULT_TARGET_FPS;

        SetTargetFPS (fps);
    }

    texture_atlas = LoadRenderTexture (ATLAS_SIZE, ATLAS_SIZE);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = CLITERAL(Vector2){ GetScreenWidth () / 2, GetScreenHeight () / 2 };
    camera.target = CLITERAL(Vector2){ ATLAS_SIZE / 2, ATLAS_SIZE / 2 };

    while (!WindowShouldClose ()) {
        // -----------------------------------------------------------------------------
        // Camera movement and zoom
        // -----------------------------------------------------------------------------
        if (IsMouseButtonDown (MOUSE_BUTTON_MIDDLE) || IsKeyDown (KEY_LEFT_ALT)) {
            Vector2 delta = GetMouseDelta ();

            delta = Vector2Scale (delta, -1.0f / camera.zoom);
            camera.target = Vector2Add (camera.target, delta);
        }

        float wheel = GetMouseWheelMove ();
        if (wheel != 0) {
            Vector2 mouse_world_position = GetScreenToWorld2D (GetMousePosition (), camera);

            camera.offset = GetMousePosition();
            camera.target = mouse_world_position;

            const float zoom_amount = 0.125f;

            camera.zoom += (wheel * zoom_amount);
            if (camera.zoom < zoom_amount) camera.zoom = zoom_amount;
        }

        // -----------------------------------------------------------------------------
        // Rectangle Collision
        // -----------------------------------------------------------------------------
        Vector2 mouse_world_position = GetScreenToWorld2D (GetMousePosition (), camera);

        for (size_t i = 0; i < files.count; i++) {
            currently_selected_rectangle = NULL;
            currently_selected_sprite = NULL;

            RSP_Sprite* current_sprite = &sprites[i];

            if (CheckCollisionPointRec (mouse_world_position, current_sprite->source)) {
                currently_selected_rectangle = &current_sprite->source;
                break;
            }
        }

        if (toolbar_state.button_new_project_pressed) {
            RSP_CreateEmptyProject ("new_project", NULL);
        }

        if (toolbar_state.button_load_project_pressed) {
            LoadAndFilterAssets ();
            RenderRectangleSizes ();
        }

        BeginDrawing ();
            ClearBackground (SKYBLUE);

            BeginMode2D (camera);
                DrawTexturePro (texture_atlas.texture, CLITERAL(Rectangle){ 0, 0, ATLAS_SIZE, -ATLAS_SIZE }, CLITERAL(Rectangle){ 0, 0, ATLAS_SIZE, ATLAS_SIZE }, Vector2Zero (), 0.0f, WHITE);
                DrawRectangleLinesEx (CLITERAL(Rectangle){ -1, -1, texture_atlas.texture.width + 2, texture_atlas.texture.height + 2 }, 1.0f, YELLOW);

                if (currently_selected_rectangle != NULL) {
                    DrawRectangleLinesEx (*currently_selected_rectangle, 1.0f, GREEN);

                    DrawLineV (
                        CLITERAL(Vector2){ .x = currently_selected_rectangle->x, .y = mouse_world_position.y },
                        CLITERAL(Vector2){ .x = currently_selected_rectangle->x + currently_selected_rectangle->width, .y = mouse_world_position.y },
                        WHITE
                    );

                    DrawLineV (
                        CLITERAL(Vector2){ .x = mouse_world_position.x, .y = currently_selected_rectangle->y },
                        CLITERAL(Vector2){ .x = mouse_world_position.x, .y = currently_selected_rectangle->y + currently_selected_rectangle->height },
                        WHITE
                    );

                    DrawCircleLinesV (mouse_world_position, 1, WHITE);
                }
            EndMode2D ();

            WGT_GuiToolbar (&toolbar_state);
        EndDrawing ();
    }

    for (size_t i = 0; i < files.count; i++) {
        UnloadTexture (sprites[i].texture);
    }

    free (sprites);

    UnloadRenderTexture (texture_atlas);

    CloseWindow ();

    return 0;
}

uint64_t hash (const char* key) {
    #define FNV_OFFSET 14695981039346656037UL
    #define FNV_PRIME 1099511628211UL

    uint64_t hash = FNV_OFFSET;

    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }

    return hash;
}

static int CompareTextureRectangles (const void* a, const void* b) {
    RSP_Sprite* sprite_a = (RSP_Sprite*)a;
    RSP_Sprite* sprite_b = (RSP_Sprite*)b;

    int mass_a = (int)(sprite_a->source.width * sprite_a->source.height);
    int mass_b = (int)(sprite_b->source.width * sprite_b->source.height);

    if (mass_a == mass_b) {
        if ((int)sprite_a->source.width == (int)sprite_b->source.width) {
            if (sprite_a->source.height > sprite_b->source.height) return -1;
            if (sprite_b->source.height > sprite_a->source.height) return 1;

            return 0;
        } else {
            if (sprite_a->source.width > sprite_b->source.width) return -1;
            if (sprite_b->source.width > sprite_a->source.width) return 1;

            return 0;
        }
    } else {
        if (mass_a > mass_b) return -1;
        if (mass_b > mass_a) return 1;
    }

    return 0;
}

void LoadAndFilterAssets (void) {
    files = LoadDirectoryFiles ("assets");

    if (files.count == 0) {
        TraceLog (LOG_ERROR, "No files found in [%s]", "assets");
        return;
    }

    sprites = calloc (files.count, sizeof (RSP_Sprite));

    for (size_t i = 0; i < files.count; i++) {
        Texture2D texture = LoadTexture (files.paths[i]);

        // CopyFile (files.paths[i], TextFormat ("projects/new_project/textures/%s", GetFileName (files.paths[i])));

        sprites[i] = CLITERAL(RSP_Sprite) {
            .texture = texture,

            .source = CLITERAL(Rectangle) { 0, 0, texture.width, texture.height },
            .origin = Vector2Zero ()
        };

        strncpy (sprites[i].name, GetFileNameWithoutExt (files.paths[i]), MAX_ASSET_NAME_LENGTH);
        sprites[i].hash = hash (sprites[i].name);
    }

    qsort (sprites, files.count, sizeof (RSP_Sprite), CompareTextureRectangles);
}

static void RenderRectangleSizes (void) {
    int textures_placed = 0;

    for (size_t i = 0; i < files.count; i++) {
        Rectangle* current_rectangle = &sprites[i].source;

        for (size_t j = 0; j < textures_placed; j++) {
            while (CheckCollisionRecs (*current_rectangle, sprites[j].source)) {
                current_rectangle->x += sprites[j].source.width;

                int within_x = (current_rectangle->x + current_rectangle->width) <= ATLAS_SIZE;

                if (!within_x) {
                    current_rectangle->x = 0;
                    current_rectangle->y += ATLAS_ITEM_MIN_SIZE;

                    j = 0;
                }
            }
        }

        textures_placed++;
    }

    BeginTextureMode (texture_atlas);
        ClearBackground (Fade (BLACK, 0));

        for (size_t i = 0; i < files.count; i++) {
            RSP_Sprite* current_sprite = &sprites[i];

            DrawTextureV (current_sprite->texture, CLITERAL(Vector2){ current_sprite->source.x, current_sprite->source.y }, WHITE);
        }
    EndTextureMode ();

    Image atlas_image = LoadImageFromTexture (texture_atlas.texture);
    ImageFlipVertical (&atlas_image);

    ExportImage (atlas_image, "output.png");

    UnloadImage (atlas_image);
}

void CopyFile (const char* source, const char* destination) {
    unsigned char* source_file;
    int file_length;

    source_file = LoadFileData (source, &file_length);

    SaveFileData (destination, source_file, file_length);

    MemFree (source_file);
}
