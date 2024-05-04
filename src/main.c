// TODO: Move to something more dedicated for rectangle packing

#include <raylib.h>
#include <raymath.h>

#include <raygui.h>
#include <parson.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#define WGT_TOOLBAR_IMPL
#include "widgets/toolbar.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define WINDOW_TITLE "Hello World"

#define ATLAS_SIZE 1024
#define ATLAS_ITEM_MIN_SIZE 16

#define MAX_ASSET_NAME_LENGTH 32

#define ASSET_DIR "assets"
#define PROJECT_DIR "projects"

typedef struct RTX_Sprite {
    size_t id;

    uint64_t hash;
    char name[MAX_ASSET_NAME_LENGTH];

    Texture2D texture;

    Rectangle source;
    Vector2 origin;
} RTX_Sprite;

struct {
    int mode;
} STATE;

static RTX_Sprite* sprites;
static FilePathList files;

static RenderTexture2D texture_atlas;

static Rectangle* currently_selected_rectangle;
static RTX_Sprite* currently_selected_sprite;

static WGT_ToolbarState toolbar_state;

static uint64_t hash (const char* key);
static int CompareTextureRectangles (const void* a, const void* b);
static void LoadAndFilterAssets (void);
static void RenderRectangleSizes (void);
static void ExportData (void);
static void ImportData (void);

static void NewProject (const char* name);
static void SaveProject (const char* name);
static void LoadProject (const char* name);

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

    {
        if (!DirectoryExists (PROJECT_DIR)) mkdir (PROJECT_DIR, 0777);
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

            RTX_Sprite* current_sprite = &sprites[i];

            if (CheckCollisionPointRec (mouse_world_position, current_sprite->source)) {
                currently_selected_rectangle = &current_sprite->source;
                break;
            }
        }

        if (toolbar_state.button_new_project_pressed) {
            NewProject ("test");
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
    RTX_Sprite* sprite_a = (RTX_Sprite*)a;
    RTX_Sprite* sprite_b = (RTX_Sprite*)b;

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
    files = LoadDirectoryFiles (ASSET_DIR);

    if (files.count == 0) {
        TraceLog (LOG_INFO, "No files found in [%s]", ASSET_DIR);
        return;
    }

    sprites = calloc (files.count, sizeof (RTX_Sprite));

    for (size_t i = 0; i < files.count; i++) {
        Texture2D texture = LoadTexture (files.paths[i]);

        sprites[i] = CLITERAL(RTX_Sprite) {
            .texture = texture,

            .source = CLITERAL(Rectangle) { 0, 0, texture.width, texture.height },
            .origin = Vector2Zero ()
        };

        strncpy (sprites[i].name, GetFileNameWithoutExt (files.paths[i]), MAX_ASSET_NAME_LENGTH);
        sprites[i].hash = hash (sprites[i].name);
    }

    qsort (sprites, files.count, sizeof (RTX_Sprite), CompareTextureRectangles);

    for (size_t i = 0; i < files.count; i++) sprites[i].id = i;
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
            RTX_Sprite* current_sprite = &sprites[i];

            DrawTextureV (current_sprite->texture, CLITERAL(Vector2){ current_sprite->source.x, current_sprite->source.y }, WHITE);
        }
    EndTextureMode ();

    Image atlas_image = LoadImageFromTexture (texture_atlas.texture);
    ImageFlipVertical (&atlas_image);

    ExportImage (atlas_image, "output.png");

    UnloadImage (atlas_image);
}

void ExportData (void) {
    FILE* output_file = fopen ("data.dat", "wb");

    if (output_file) {
        fwrite (&files.count, sizeof (unsigned int), 1, output_file);

        int atlas_size;
        int atlas_compressed_size;

        Image atlas_image = LoadImageFromTexture (texture_atlas.texture);

        unsigned char* data = ExportImageToMemory (atlas_image, ".png", &atlas_size);
        unsigned char* compressed_data = CompressData (data, atlas_size, &atlas_compressed_size);

        free (data);

        fwrite (&atlas_compressed_size, sizeof (int), 1, output_file);
        fwrite (compressed_data, sizeof (unsigned char), atlas_compressed_size, output_file);

        free (compressed_data);

        UnloadImage (atlas_image);

        for (int i = 0; i < files.count; i++) {
            RTX_Sprite* sprite = &sprites[i];

            fwrite (&sprite->hash, sizeof (uint64_t), 1, output_file);
            fwrite (sprite->name, sizeof (char), MAX_ASSET_NAME_LENGTH, output_file);

            fwrite (&sprite->source.x, sizeof (float), 1, output_file);
            fwrite (&sprite->source.y, sizeof (float), 1, output_file);
            fwrite (&sprite->source.width, sizeof (float), 1, output_file);
            fwrite (&sprite->source.height, sizeof (float), 1, output_file);

            fwrite (&sprite->origin.x, sizeof (float), 1, output_file);
            fwrite (&sprite->origin.y, sizeof (float), 1, output_file);
        }

        fclose (output_file);
    } else {
        TraceLog (LOG_ERROR, "Failed to open file!");
    }
}

void ImportData (void) {
    FILE* import_file = fopen ("data.dat", "rb");

    int file_count;
    int data_compressed_size, data_decompressed_size;

    if (import_file) {
        fread (&file_count, sizeof(unsigned int), 1, import_file);
        fread (&data_compressed_size, sizeof (int), 1, import_file);
        TraceLog (LOG_INFO, "%d", data_compressed_size);

        unsigned char* compressed_data = calloc (data_compressed_size, sizeof (unsigned char));
        unsigned char* decompressed_data;

        fread (compressed_data, sizeof (unsigned char), data_compressed_size, import_file);

        decompressed_data = DecompressData (compressed_data, data_compressed_size, &data_decompressed_size);

        Image atlas_image = LoadImageFromMemory (".png", decompressed_data, data_decompressed_size);
        ImageFlipVertical (&atlas_image);

        Texture2D atlas_texture = LoadTextureFromImage (atlas_image);

        free (compressed_data);
        free (decompressed_data);

        BeginTextureMode (texture_atlas);
            ClearBackground (Fade (BLACK, 0));

            DrawTexture (atlas_texture, 0, 0, WHITE);
        EndTextureMode ();

        UnloadImage (atlas_image);
        UnloadTexture (atlas_texture);

        if (sprites != NULL) {
            free (sprites);
        }

        sprites = calloc(file_count, sizeof(RTX_Sprite));

        for (int i = 0; i < file_count; i++) {
            RTX_Sprite* current_sprite = &sprites[i];

            fread (&current_sprite->hash, sizeof (uint64_t), 1, import_file);
            fread (current_sprite->name, sizeof (char), MAX_ASSET_NAME_LENGTH, import_file);

            fread (&current_sprite->source.x, sizeof (float), 1, import_file);
            fread (&current_sprite->source.y, sizeof (float), 1, import_file);
            fread (&current_sprite->source.width, sizeof (float), 1, import_file);
            fread (&current_sprite->source.height, sizeof (float), 1, import_file);

            fread (&current_sprite->origin.x, sizeof (float), 1, import_file);
            fread (&current_sprite->origin.y, sizeof (float), 1, import_file);
        }

        files.count = file_count;

        fclose (import_file);
    } else {
        TraceLog (LOG_ERROR, "Failed to open file!");
    }
}

void NewProject (const char* name) {
    if (DirectoryExists (name)) {
        TraceLog (LOG_ERROR, "Project [%s] already exists!", name);

        return;
    }

    mkdir (TextFormat ("%s/%s", PROJECT_DIR, name), 0777);
    mkdir (TextFormat ("%s/%s/assets", PROJECT_DIR, name), 0777);

    ChangeDirectory (TextFormat ("%s/%s", PROJECT_DIR, name));

    JSON_Value* root_value = json_value_init_object ();
    JSON_Object* root_object = json_value_get_object (root_value);

    // Array
    JSON_Value* rectangle_array_value = json_value_init_array ();
    JSON_Array* rectangle_array = json_value_get_array (rectangle_array_value);

    for (size_t i = 0; i < files.count; i++) {
        JSON_Value* rectangle_value = json_value_init_object ();
        JSON_Object* rectangle_object = json_value_get_object (rectangle_value);

        json_object_set_number (rectangle_object, "x", sprites[i].source.x);
        json_object_set_number (rectangle_object, "y", sprites[i].source.y);
        json_object_set_number (rectangle_object, "width", sprites[i].source.width);
        json_object_set_number (rectangle_object, "height", sprites[i].source.height);

        json_array_append_value (rectangle_array, rectangle_value);
    }

    json_object_set_value (root_object, "sprites", rectangle_array_value);

    json_serialize_to_file_pretty (root_value, "test.json");
    json_value_free (root_value);
}
