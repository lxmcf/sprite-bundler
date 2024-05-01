// TODO: Move to something more dedicated for rectangle packing

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Hello World"

#define ATLAS_SIZE 1024
#define ATLAS_ITEM_MIN_SIZE 16

#define ASSET_DIR "assets"

typedef struct RTX_Sprite {
    size_t id;
    Texture2D texture;

    Rectangle source;
    Vector2 origin;
} RTX_Sprite;

static RTX_Sprite* sprites;
static FilePathList files;

static RenderTexture2D texture_atlas;

static Rectangle* currently_selected_rectangle;
static RTX_Sprite* currently_selected_sprite;

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

static void LoadAndFilterAssets (void) {
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
    }

    qsort (sprites, files.count, sizeof (RTX_Sprite), CompareTextureRectangles);

    for (size_t i = 0; i < files.count; i++) sprites[i].id = i;
}

static void RenderRectangleSizes (void) {
    int textures_placed = 0;

    // Skip first rectangle, will always be at 0|0
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

            DrawTextureV ((*current_sprite).texture, CLITERAL(Vector2){ current_sprite->source.x, current_sprite->source.y }, WHITE);
        }
    EndTextureMode ();

    // Image atlas_image = LoadImageFromTexture (texture_atlas.texture);
    // ImageFlipVertical (&atlas_image);

    // ExportImage (atlas_image, "output.png");

    // int filesize;
    // int compsize;
    // int decompsize;

    // unsigned char* data = ExportImageToMemory (atlas_image, ".png", &filesize);
    // unsigned char* compdata = CompressData (data, filesize, &compsize);
    // unsigned char* decompdata = DecompressData (compdata, compsize, &decompsize);

    // TraceLog (LOG_INFO, " RAW [%d] | COMP [%d] | DECOMP [%d]", filesize, compsize, decompsize);

    // SaveFileData ("output_raw.dat", data, filesize);
    // SaveFileData ("output_comp.dat", compdata, compsize);
    // SaveFileData ("output_decomp.dat", decompdata, decompsize);

    // free (data);
    // free (compdata);
    // free (decompdata);

    // UnloadImage (atlas_image);
}

int main (int argc, const char* argv[]) {
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    SetWindowState (FLAG_WINDOW_RESIZABLE);

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

            if (GuiButton (CLITERAL(Rectangle){ 32, 32, 128, 48 }, "Load Assets")) {
                if (!sprites) {
                    LoadAndFilterAssets ();
                    RenderRectangleSizes ();
                }
            }

            if (currently_selected_rectangle != NULL) {
                int x = (int)(mouse_world_position.x - currently_selected_rectangle->x);
                int y = (int)(mouse_world_position.y - currently_selected_rectangle->y);

                const char* coordinates = TextFormat ("%d, %d", x, y);

                Vector2 text_size = MeasureTextEx (GetFontDefault (), coordinates, 40, 1.0f);
                Vector2 render_position = Vector2Subtract (GetMousePosition (), Vector2SubtractValue (text_size, 2));

                Rectangle tooltip = CLITERAL(Rectangle) {
                    .x = render_position.x - 2,
                    .y = render_position.y - 2,
                    .width = text_size.x + 2,
                    .height = text_size.y + 2
                };

                DrawRectangleRec (tooltip, Fade (BLACK, 0.5f));
                DrawTextEx (GetFontDefault (), coordinates, render_position, 40, 1.0f, RAYWHITE);
            }

            DrawFPS (8, 8);
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
