# Sprite Bundler

<p align="center">:construction: This ballooned massively and is really terribly written :construction:</p>

A basic sprite packer to combine multiple sprites into 1 texture atlas and export to a single bundle file.

## Application Usage
1. Run `$ make run BUILD=BUILD_RELEASE`
2. Select project properties and create new project
3. Drag image files over window
4. Click export

## API Usage (raylib)
```c
// main.c

#define RSP_IMPLEMENTATION
#include <rsp.h>

#define SPRITE_NAME "[INSERT SPRITE NAME]"

int main (int argc, const char* argv[]) {
    InitWindow (640, 360, "Hello World");

    SpriteBundle bundle = LoadBundle ("bundle.rspx");
    SetActiveBundle (&bundle);

    while (!WindowShouldClose ()) {
        ClearBackground (RAYWHITE);

        DrawSprite (GetSpriteId (SPRITE_NAME), CLITERAL(Vector2){ 32, 32 }, WHITE);
    }

    UnloadBundle (bundle);

    CloseWindow ();

    return 0;
}
```

> _**Note:** Sprite Bundler is not a 100% 'safe' program and does not do a lot of error checking, not intended for large scale usage, mostly game jams or quick prototypes_
