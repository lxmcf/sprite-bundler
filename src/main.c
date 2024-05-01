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

static Rectangle* texture_rectangles;
static FilePathList files;

static Texture2D test_texture;

static RenderTexture2D texture_atlas;

static int CompareTextureRectangles (const void* a, const void* b) {
    Rectangle* rect_a = (Rectangle*)a;
    Rectangle* rect_b = (Rectangle*)b;

    int mass_a = (int)(rect_a->width * rect_a->height);
    int mass_b = (int)(rect_b->width * rect_b->height);

    if (mass_a == mass_b) {
        if ((int)rect_a->width == (int)rect_b->width) {
            if (rect_a->height > rect_b->height) return -1;
            if (rect_b->height > rect_a->height) return 1;

            return 0;
        } else {
            if (rect_a->width > rect_b->width) return -1;
            if (rect_b->width > rect_a->width) return 1;

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

    texture_rectangles = calloc (files.count, sizeof (Rectangle));

    for (size_t i = 0; i < files.count; i++) {
        int width, height;
        stbi_info (files.paths[i], &width, &height, NULL);

        texture_rectangles[i] = CLITERAL(Rectangle) { 0, 0, width, height };
    }

    qsort (texture_rectangles, files.count, sizeof (Rectangle), CompareTextureRectangles);
}

static void RenderRectangleSizes (void) {
    int textures_placed = 0;

    // Skip first rectangle, will always be at 0|0
    for (size_t i = 0; i < files.count; i++) {
        Rectangle* current_rectangle = &texture_rectangles[i];

        for (size_t j = 0; j < textures_placed; j++) {
            while (CheckCollisionRecs (*current_rectangle, texture_rectangles[j])) {
                current_rectangle->x += texture_rectangles[j].width;

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

        // for (size_t i = 0; i < files.count; i++) {
        //     DrawRectangleLinesEx (texture_rectangles[i], 1.0f, RED);
        // }
    EndTextureMode ();

    Image atlas_image = LoadImageFromTexture (texture_atlas.texture);
    ImageFlipVertical (&atlas_image);

    ExportImage (atlas_image, "output.png");

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

    UnloadImage (atlas_image);
}

int main (int argc, const char* argv[]) {
    InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    texture_atlas = LoadRenderTexture (ATLAS_SIZE, ATLAS_SIZE);

    // Used to ensure we are displaying correctly
    test_texture = LoadTexture ("assets/atlas.png");

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    while (!WindowShouldClose ()) {
        if (IsMouseButtonDown (MOUSE_BUTTON_MIDDLE)) {
            Vector2 delta = GetMouseDelta ();

            delta = Vector2Scale (delta, -1.0f / camera.zoom);
            camera.target = Vector2Add (camera.target, delta);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouse_world_position = GetScreenToWorld2D (GetMousePosition (), camera);

            camera.offset = GetMousePosition();
            camera.target = mouse_world_position;

            const float zoom_amount = 0.125f;

            camera.zoom += (wheel * zoom_amount);
            if (camera.zoom < zoom_amount) camera.zoom = zoom_amount;
        }

        BeginDrawing ();
            ClearBackground (SKYBLUE);

            BeginMode2D (camera);
            {
                DrawTexturePro (texture_atlas.texture, CLITERAL(Rectangle){ 0, 0, ATLAS_SIZE, -ATLAS_SIZE }, CLITERAL(Rectangle){ 0, 0, ATLAS_SIZE, ATLAS_SIZE }, Vector2Zero (), 0.0f, WHITE);
                DrawRectangleLinesEx (CLITERAL(Rectangle){ -1, -1, texture_atlas.texture.width + 2, texture_atlas.texture.height + 2 }, 1.0f, YELLOW);

                Vector2 mouse_world_position = GetScreenToWorld2D (GetMousePosition (), camera);

                for (size_t i = 0; i < files.count; i++) {
                    Rectangle* current_rect = &texture_rectangles[i];

                    if (CheckCollisionPointRec (mouse_world_position, texture_rectangles[i])) {
                        DrawRectangleLinesEx (texture_rectangles[i], 1.0f, GREEN);

                        DrawLineV (
                            CLITERAL(Vector2){ .x = current_rect->x, .y = mouse_world_position.y },
                            CLITERAL(Vector2){ .x = current_rect->x + current_rect->width, .y = mouse_world_position.y },
                            WHITE
                        );

                        DrawLineV (
                            CLITERAL(Vector2){ .x = mouse_world_position.x, .y = current_rect->y },
                            CLITERAL(Vector2){ .x = mouse_world_position.x, .y = current_rect->y + current_rect->height },
                            WHITE
                        );

                        DrawCircleLinesV (mouse_world_position, 1, WHITE);

                        break;
                    }
                }
            }
            EndMode2D ();

            if (GuiButton (CLITERAL(Rectangle){ 32, 32, 128, 48 }, "Load Assets")) {
                if (!texture_rectangles) {
                    LoadAndFilterAssets ();
                    RenderRectangleSizes ();
                }
            }

            DrawFPS (8, 8);
        EndDrawing ();
    }

    UnloadRenderTexture (texture_atlas);
    UnloadTexture (test_texture);

    CloseWindow ();

    return 0;
}
