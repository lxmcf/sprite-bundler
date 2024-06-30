# Sprite Bundler

<p align="center">:construction: This ballooned massively and is really terribly written, you have been warned! :construction:</p>

![Welcome Screen](/data/welcome_screen.png)

A basic sprite packer to combine multiple sprites into 1 texture atlas and export to a single bundle file.

## Application Usage
1. Run `$ make run BUILD=BUILD_RELEASE`
1. Select project properties and create new project
1. Drag image files over window
1. Click export (This will also generate a header file of named enums)

## Limitations & Warnings
1. Only 1 texture atlas
1. Only 1 bundle can be used at once (Multiple can be loaded into memory)
1. My handling of strings is... Yeah
1. Editor and raylib implementation has not been tested on Windows at all
1. Animation's are 'kind of' ready to be added
1. I may not update this any further

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
        BeginDrawing ();
            ClearBackground (RAYWHITE);
            DrawSprite (GetSpriteId (SPRITE_NAME), CLITERAL(Vector2){ 32, 32 }, WHITE);
        EndDrawing ();
    }

    UnloadBundle (bundle);

    CloseWindow ();

    return 0;
}
```

## Working Example
A working example including prebuilt bundle can be found in the 'example' directory, simply run the following...
```shell
./build.sh
./demo
```

> _**Note:** Sprite Bundler is not a 100% 'safe' program and does not do a lot of error checking, not intended for medium-large scale usage, mostly designed for game jams, quick prototypes and small scale games_
