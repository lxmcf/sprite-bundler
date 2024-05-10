// TODO: Move to something more dedicated for rectangle packing

#include "types.h"

#include <raymath.h>
#include <raygui.h>

#include <string.h>
#include <stdlib.h>

#define WGT_TOOLBAR_IMPL
#include "widgets/toolbar.h"

#define WGT_WELCOME_IMPL
#include "widgets/welcome.h"

#include "project/project.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define WINDOW_TITLE "Hello World"

#define ATLAS_ITEM_MIN_SIZE 16

RSP_Project main_project;

static Rectangle* currently_selected_rectangle;
static RSP_Sprite* currently_selected_sprite;

static WGT_ToolbarState toolbar_state;

static bool welcome_active;
static WGT_WelcomeState welcome_state;

static int CompareTextureRectangles (const void* a, const void* b);
static void LoadAndFilterAssets (FilePathList* files);
static void LoadNewAsset (const char* filename);
static void RenderRectangleSizes (void);

void CopyFile (const char* source, const char* destination);

int main (int argc, const char* argv[]) {
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    SetWindowState (FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize (WINDOW_WIDTH, WINDOW_HEIGHT);

    toolbar_state = WGT_InitToolbarState ();

    welcome_active = true;
    welcome_state = WGT_InitWelcomeState ();

    {   // Set framerate to max supported
        #define DEFAULT_TARGET_FPS 60
        int current_monitor = GetCurrentMonitor ();
        int fps = GetMonitorRefreshRate (current_monitor);

        // Fallback to 60 FPS target if error occurs
        if (fps <= 0) fps = DEFAULT_TARGET_FPS;

        SetTargetFPS (fps);
    }

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = CLITERAL(Vector2){ GetScreenWidth () / 2, GetScreenHeight () / 2 };
    camera.target = CLITERAL(Vector2){ 0, 0 };

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

        for (size_t i = 0; i < main_project.atlas.sprite_count; i++) {
            currently_selected_rectangle = NULL;
            currently_selected_sprite = NULL;

            RSP_Sprite* current_sprite = &main_project.sprites[i];

            if (CheckCollisionPointRec (mouse_world_position, current_sprite->source)) {
                currently_selected_rectangle = &current_sprite->source;
                break;
            }
        }

        if (IsFileDropped () ) {
            FilePathList files = LoadDroppedFiles ();

            for (size_t i = 0; i < files.count; i++) {
                LoadNewAsset (files.paths[i]);
            }

            qsort (main_project.sprites, main_project.atlas.sprite_count, sizeof (RSP_Sprite), CompareTextureRectangles);

            RenderRectangleSizes ();
            UnloadDroppedFiles (files);
        }

        if (toolbar_state.button_save_project) {
            RSP_SaveProject (&main_project);
        }

        if (toolbar_state.button_load_project_pressed) {
            RenderRectangleSizes ();
        }

        if (welcome_state.button_create_project) {
            main_project.atlas.size = 1024;
            RSP_CreateEmptyProject (welcome_state.textbox_create_project, &main_project);
        }

        if (welcome_state.load_selected_project && welcome_state.activeSelection != -1) {
            RSP_LoadProject (welcome_state.files.paths[welcome_state.activeSelection], &main_project);
            RenderRectangleSizes ();

            welcome_state.activeSelection = -1;
            welcome_active = false;
        }

        BeginDrawing ();
            ClearBackground (GetColor(GuiGetStyle (DEFAULT, BASE_COLOR_NORMAL)));

            if (welcome_active) {
                WGT_GuiWelcome (&welcome_state);
            } else {
                BeginMode2D (camera);
                    DrawTexturePro (main_project.atlas.texture.texture, CLITERAL(Rectangle){ 0, 0, main_project.atlas.size, -main_project.atlas.size }, CLITERAL(Rectangle){ 0, 0, main_project.atlas.size, main_project.atlas.size }, Vector2Zero (), 0.0f, WHITE);
                    DrawRectangleLinesEx (CLITERAL(Rectangle){ -1, -1, main_project.atlas.texture.texture.width + 2, main_project.atlas.texture.texture.height + 2 }, 1.0f, YELLOW);

                    if (currently_selected_rectangle != NULL) {
                        DrawRectangleLinesEx (*currently_selected_rectangle, 1.0f, GREEN);

                        mouse_world_position = CLITERAL(Vector2) {
                            roundf (mouse_world_position.x),
                            roundf (mouse_world_position.y),
                        };

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
            }

            DrawFPS (8, 8);
        EndDrawing ();
    }

    RSP_UnloadProject (&main_project);

    CloseWindow ();

    return 0;
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

void LoadAndFilterAssets (FilePathList* files) {
    if (files->count == 0) {
        TraceLog (LOG_ERROR, "No files->found in [%s]", "assets");
        return;
    }

    main_project.sprites = calloc (files->count, sizeof (RSP_Sprite));

    for (size_t i = 0; i < files->count; i++) {
        Texture2D texture = LoadTexture (files->paths[i]);
        const char* new_filename = TextFormat ("projects/%s/textures/%s", main_project.name, GetFileName (files->paths[i]));

        CopyFile (files->paths[i], new_filename);

        main_project.sprites[i] = CLITERAL(RSP_Sprite) {
            .texture = texture,

            .source = CLITERAL(Rectangle) { 0, 0, texture.width, texture.height },
            .origin = Vector2Zero ()
        };

        strncpy (main_project.sprites[i].name, GetFileNameWithoutExt (files->paths[i]), MAX_ASSET_NAME_LENGTH);
        strncpy (main_project.sprites[i].file, new_filename, MAX_ASSET_FILENAME_LENGTH);
    }

    main_project.atlas.sprite_count = files->count;

    qsort (main_project.sprites, files->count, sizeof (RSP_Sprite), CompareTextureRectangles);
}

static void LoadNewAsset (const char* filename) {
    if (!FileExists (filename)) {
        TraceLog (LOG_ERROR, "File [%s] does not exists!", filename);

        return;
    }

    Texture2D texture = LoadTexture (filename);
    const char* new_filename = TextFormat ("projects/%s/textures/%d%s", main_project.name, main_project.atlas.sprite_count, GetFileExtension (filename));

    CopyFile (filename, new_filename);

    main_project.sprites = realloc (main_project.sprites, (main_project.atlas.sprite_count + 1) * sizeof (RSP_Sprite));

    main_project.sprites[main_project.atlas.sprite_count] = CLITERAL(RSP_Sprite) {
        .texture = texture,

        .source = CLITERAL(Rectangle) { 0, 0, texture.width, texture.height },
        .origin = Vector2Zero (),
    };

    strncpy (main_project.sprites[main_project.atlas.sprite_count].name, GetFileNameWithoutExt (filename), MAX_ASSET_NAME_LENGTH);
    strncpy (main_project.sprites[main_project.atlas.sprite_count].file, new_filename, MAX_ASSET_FILENAME_LENGTH);

    main_project.atlas.sprite_count++;
}

static void RenderRectangleSizes (void) {
    int textures_placed = 0;

    for (size_t i = 0; i < main_project.atlas.sprite_count; i++) {
        Rectangle* current_rectangle = &main_project.sprites[i].source;

        for (size_t j = 0; j < textures_placed; j++) {
            while (CheckCollisionRecs (*current_rectangle, main_project.sprites[j].source)) {
                current_rectangle->x += main_project.sprites[j].source.width;

                int within_x = (current_rectangle->x + current_rectangle->width) <= main_project.atlas.size;

                if (!within_x) {
                    current_rectangle->x = 0;
                    current_rectangle->y += ATLAS_ITEM_MIN_SIZE;

                    j = 0;
                }
            }
        }

        textures_placed++;
    }

    BeginTextureMode (main_project.atlas.texture);
        ClearBackground (Fade (BLACK, 0));

        for (size_t i = 0; i < main_project.atlas.sprite_count; i++) {
            RSP_Sprite* current_sprite = &main_project.sprites[i];

            DrawTextureV (current_sprite->texture, CLITERAL(Vector2){ current_sprite->source.x, current_sprite->source.y }, WHITE);
        }
    EndTextureMode ();

    Image atlas_image = LoadImageFromTexture (main_project.atlas.texture.texture);
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
