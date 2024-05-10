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
#define TOGGLE(x) x = !x

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
typedef struct RSP_WidgetWelcome {
    bool active;
    Vector2 anchor;

    int width;
    int height;

    int list_projects_scroll_index;
    int list_projects_active;

    bool button_load_project_pressed;
    bool button_delete_project_pressed;
    bool button_new_project_pressed;

    bool textbox_project_name_edit;
    char textbox_project_name_text[32];

    bool dropdown_atlas_size_edit;
    int dropdown_atlas_size_active;

    bool value_atlas_size_edit;
    int value_atlas_size;
} RSP_WidgetWelcome;


typedef struct RSP_WidgetToolbar {
    bool active;
    Vector2 anchor;
} RSP_WidgetToolbar;

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
        widget_welcome = RSP_InitWelcome ();
        widget_toolbar = RSP_InitToolbar ();
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

            #if BUILD_DEBUG
            DrawFPS (8, 8);
            #endif
        EndDrawing ();
    }

    CloseWindow ();

    return 0;
}

RSP_WidgetWelcome RSP_InitWelcome (void) {
    RSP_WidgetWelcome state = CLITERAL(RSP_WidgetWelcome) { 0 };

    state.anchor = Vector2Zero ();
    state.active = true;

    state.width = 640;
    state.height = 328;

    return state;
}

RSP_WidgetToolbar RSP_InitToolbar (void) {
    RSP_WidgetToolbar state = CLITERAL(RSP_WidgetToolbar) { 0 };

    state.anchor = Vector2Zero ();
    state.active = false;

    return state;
}

void RSP_UpdateWelcome (void) {
    widget_welcome.anchor = CLITERAL(Vector2){
        (GetScreenWidth () / 2) - (widget_welcome.width / 2),
        (GetScreenHeight () / 2) - (widget_welcome.height / 2)
    };
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

void RSP_RenderWelcome (void) {
    if (widget_welcome.dropdown_atlas_size_edit) GuiLock();

    if (widget_welcome.active) {
        const Rectangle layouts[] = { // NOTE: Yes I know this is not efficient
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 0,   widget_welcome.anchor.y + 0, widget_welcome.width, widget_welcome.height },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 32, 624, 128 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 168, 304, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 328, widget_welcome.anchor.y + 168, 304, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 208, 624, 112 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 472, widget_welcome.anchor.y + 288, 152, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 224, 96, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 224, 128, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 288, 96, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 288, 128, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 256, 128, 24 },
            CLITERAL(Rectangle){ widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 256, 96, 24 },
        };

        // Temp for now
        widget_welcome.active = !GuiWindowBox (layouts[0], "#015#Projects");
        GuiListView (layouts[1], "ONE;TWO;THREE;FOUR;FIVE", &widget_welcome.list_projects_scroll_index, &widget_welcome.list_projects_active);

        widget_welcome.button_load_project_pressed = GuiButton (layouts[2], "#005#Load Selected Project");
        widget_welcome.button_delete_project_pressed = GuiButton (layouts[3], "#143#Delete Selected Project");

        GuiGroupBox (layouts[4], "New Project");

        {
            widget_welcome.button_new_project_pressed = GuiButton (layouts[5], "#008#Create & Load Project");

            GuiLabel (layouts[6], "Project Name");
            if (GuiTextBox (layouts[7], widget_welcome.textbox_project_name_text, 128, widget_welcome.textbox_project_name_edit)) TOGGLE (widget_welcome.textbox_project_name_edit);

            GuiLabel(layouts[8], "Atlas Size");
            if (GuiValueBox (layouts[10], NULL, &widget_welcome.value_atlas_size, 0, 128, widget_welcome.value_atlas_size_edit)) TOGGLE (widget_welcome.value_atlas_size_edit);

            GuiLabel (layouts[11], "Alignment Value");
            if (GuiDropdownBox (layouts[9], "512;1024;2048;4096;8192", &widget_welcome.dropdown_atlas_size_active, widget_welcome.dropdown_atlas_size_edit)) TOGGLE (widget_welcome.dropdown_atlas_size_edit);
        }
    }

    GuiUnlock();
}

static void __RSP_Toolbar (void) {
    GuiStatusBar (CLITERAL(Rectangle){ widget_toolbar.anchor.x, widget_toolbar.anchor.y, GetRenderWidth (), 40 }, NULL);
}

void RSP_RenderEditor (void) {
    __RSP_Toolbar ();
}
