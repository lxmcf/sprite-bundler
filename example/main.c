#define RSP_IMPLEMENTATION
#include <math.h>
#include <rsp.h>

// NOTE: Will return sinf but between 0 and 1
#define SINE(x) ((sinf ((float)x) + 1.0f) / 2.0f)

int main (int argc, const char* argv[]) {
    InitWindow (640, 360, "Hello World");

    SetTargetFPS (60);

    const char* sprite_names[5] = {"blue_ship", "pink_ship", "green_ship", "beige_ship", "yellow_ship"};

    int current_sprite = 0;
    float rotation     = 0;

    SpriteBundle bundle = LoadBundle ("bundle.rspx");
    SetActiveBundle (&bundle);

    while (!WindowShouldClose ()) {
        if (IsMouseButtonReleased (MOUSE_BUTTON_LEFT))
            current_sprite++;

        rotation += GetFrameTime () * 90;
        const float time = GetTime () * 2;
        Vector2 scale    = CLITERAL (Vector2){1 + SINE (time), 1 + SINE (time)};

        BeginDrawing ();
        ClearBackground (RAYWHITE);

        DrawSpriteEx (current_sprite % bundle.sprites_count, CLITERAL (Vector2){GetScreenWidth () / 2, GetScreenHeight () / 2}, scale, rotation, WHITE);

        DrawText ("Click to change sprite!", 8, 8, 20, LIGHTGRAY);
        DrawText (TextFormat ("Current sprite: %s", sprite_names[current_sprite % bundle.sprites_count]), 8, 32, 20, LIGHTGRAY);
        EndDrawing ();
    }

    UnloadBundle (bundle);

    CloseWindow ();

    return 0;
}
