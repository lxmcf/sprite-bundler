// -----------------------------------------------------------------------------
// RaySprite
//
// A simple sprite atlas packer designed to comprss sprite and allow metadata
// editing for easy usage in any language.
// -----------------------------------------------------------------------------
#define RAYGUI_IMPLEMENTATION
#define RAYMATH_STATIC_INLINE
#include <raylib.h>
#include <raymath.h>
#include <raygui.h>

#include <string.h>
#include <stdint.h>
#include <stddef.h>

// -----------------------------------------------------------------------------
// Config
// -----------------------------------------------------------------------------
#define MINIMUM_WINDOW_WIDTH 1280
#define MINIMUM_WINDOW_HEIGHT 720

#define WINDOW_TITLE "RaySprite"
#define WINDOW_FLAGS FLAG_WINDOW_RESIZABLE

#define MAX_ASSET_NAME_LENGTH 32
#define MAX_ASSET_FILE_LENGTH 64

#define MAX_PROJECT_NAME_LENGTH 32

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SINE(x) ((sinf ((float)x) + 1.0f) / 2.0f)

#define lengthof(x) (sizeof (x) / sizeof (x[0]))

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------
typedef enum RSP_SpriteFlags {
    RSP_SPRITE_ANIMATED     = 1 << 0,
    RSP_SPRITE_ORIGIN       = 1 << 1,
} RSP_SpriteFlags;

typedef enum RSP_ProjectError {
    RSP_PROJECT_ERROR_NONE,
    RSP_PROJECT_ERROR_EXISTS,
    RSP_PROJECT_ERROR_NOT_EXIST,
    RSP_PROJECT_ERROR_INVALID,
} RSP_ProjectError;

typedef enum RSP_Mode {
    RSP_MODE_WELCOME,
    RSP_MODE_EDITOR,
} RSP_Mode;

// -----------------------------------------------------------------------------
// Type definitions
// -----------------------------------------------------------------------------
typedef struct RSP_Sprite {
    char name[MAX_ASSET_NAME_LENGTH];
    char file[MAX_ASSET_FILE_LENGTH];
    uint16_t flags;

    Texture2D texture;

    Rectangle source;
    Vector2 origin;

    struct {
        Rectangle* frames;
        int16_t frames_count;
    } animation;
} RSP_Sprite;

typedef struct RSP_Project {
    uint8_t version;
    char name[MAX_PROJECT_NAME_LENGTH];

    RSP_Sprite* sprites;
    uint16_t sprites_count;

    RenderTexture2D atlas;
    FilePathList assets;
} RSP_Project;

// Widgets
typedef struct RSP_WidgetToolbar {
    bool active;
    Vector2 anchor;
} RSP_WidgetToolbar;

typedef struct RSP_WidgetWelcome {
    bool active;
    Vector2 anchor;
} RSP_WidgetWelcome;

// -----------------------------------------------------------------------------
// Global data
// -----------------------------------------------------------------------------
static RSP_WidgetWelcome widget_welcome;
static RSP_WidgetToolbar widget_toolbar;

static RSP_Mode current_application_mode;

static Camera2D camera;

// -----------------------------------------------------------------------------
// Function decleration
// -----------------------------------------------------------------------------
static RSP_WidgetWelcome RSP_InitWelcome (void);
static RSP_WidgetToolbar RSP_InitToolbar (void);

static void RSP_UpdateWelcome (void);
static void RSP_UpdateEditor (void);

static void RSP_RenderWelcome (void);
static void RSP_RenderEditor (void);

// -----------------------------------------------------------------------------
// Core Application
// -----------------------------------------------------------------------------
int main (int argc, const char* argv[]) {
    InitWindow (MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT, WINDOW_TITLE);

    {   // :window settings
        SetWindowMinSize (MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);
        SetWindowState (WINDOW_FLAGS);
    }

    {   // :framerate
        int fps = GetMonitorRefreshRate (GetCurrentMonitor ());
        SetTargetFPS (fps <= 0 ? fps : 60);
    }

    {   // :widgets

    }

    current_application_mode = RSP_MODE_WELCOME;

    camera = CLITERAL(Camera2D){ 0 };
    camera.offset = CLITERAL(Vector2) { GetScreenWidth () / 2, GetScreenHeight () / 2 };

    while (!WindowShouldClose ()) {
        switch (current_application_mode) {
            case RSP_MODE_WELCOME:  RSP_UpdateWelcome (); break;
            case RSP_MODE_EDITOR:   RSP_UpdateEditor (); break;

            default: break;
        }

        BeginDrawing ();
            ClearBackground (GetColor (GuiGetStyle (DEFAULT, BACKGROUND_COLOR)));

            switch (current_application_mode) {
                case RSP_MODE_WELCOME:  RSP_RenderWelcome (); break;
                case RSP_MODE_EDITOR:   RSP_RenderEditor (); break;

                default: break;
            }
        EndDrawing ();
    }

    CloseWindow ();

    return 0;
}

RSP_WidgetWelcome RSP_InitWelcome (void) {
    RSP_WidgetWelcome state = CLITERAL(RSP_WidgetWelcome) { 0 };

    return state;
}

RSP_WidgetToolbar RSP_InitToolbar (void) {
    RSP_WidgetToolbar state = CLITERAL(RSP_WidgetToolbar) { 0 };

    return state;
}

void RSP_UpdateWelcome (void) {
    widget_welcome.anchor = CLITERAL(Vector2){ GetScreenWidth () / 2, GetScreenHeight () / 2 };
}

void RSP_UpdateEditor (void) {
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
}

void RSP_RenderWelcome (void) { }

void RSP_RenderEditor (void) { }
