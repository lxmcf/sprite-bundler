// -----------------------------------------------------------------------------
// RaySprite
//
// A simple sprite atlas packer designed to comprss sprite and allow metadata
// editing for easy usage in any language.
// -----------------------------------------------------------------------------
#define RAYGUI_IMPLEMENTATION
#define RAYMATH_STATIC_INLINE
#include <raygui.h>
#include <raylib.h>
#include <raymath.h>

#include <stdint.h>
#include <sys/stat.h>

#include "vendor/parson.h"

// -----------------------------------------------------------------------------
// Config
// -----------------------------------------------------------------------------
#define MINIMUM_WINDOW_WIDTH  1280
#define MINIMUM_WINDOW_HEIGHT 720

#define WINDOW_TITLE "RaySprite"
#define WINDOW_FLAGS FLAG_WINDOW_RESIZABLE

#define MAX_ASSET_NAME_LENGTH 32
#define MAX_ASSET_FILE_LENGTH 64

#define MAX_PROJECT_NAME_LENGTH 32

#define DEFAULT_PROJECT_DIRECTORY "projects"
#define DEFAULT_PROJECT_EXTENSION ".rspp"
#define DEFAULT_PROJECT_VERSION   1

#define DEFAULT_BUNDLE_EXTENSION ".rspx"

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------
#define TOGGLE(x) x = !x

#define lengthof(x) (sizeof (x) / sizeof (x[0]))

#ifdef _WIN32
#define MakeDirectory(x) _mkdir (x)
#else
#define MakeDirectory(x) mkdir (x, 0777)
#endif

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------
typedef enum RSP_SpriteFlags {
    RSP_SPRITE_ANIMATED = 1 << 0,
    RSP_SPRITE_ORIGIN   = 1 << 1,
} RSP_SpriteFlags;

typedef enum RSP_ProjectError {
    RSP_PROJECT_ERROR_NONE,
    RSP_PROJECT_ERROR_EXISTS,
    RSP_PROJECT_ERROR_NOT_EXIST,
    RSP_PROJECT_ERROR_FAILED_WRITE,
    RSP_PROJECT_ERROR_FAILED_READ,
    RSP_PROJECT_ERROR_NULL
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
        uint16_t frames_count;
        uint16_t frames_speed;
    } animation; // Not used
} RSP_Sprite;

typedef struct RSP_Project {
    uint8_t version;
    uint8_t alignment;
    char name[MAX_PROJECT_NAME_LENGTH];

    bool should_embed_files;

    RSP_Sprite* sprites;
    uint16_t sprites_count;

    RenderTexture2D atlas;
    uint16_t atlas_size;
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
    char textbox_project_name_text[MAX_PROJECT_NAME_LENGTH];

    bool dropdown_atlas_size_edit;
    int dropdown_atlas_size_active;

    bool value_atlas_align_edit;
    int value_atlas_align;
} RSP_WidgetWelcome;

typedef struct RSP_WidgetToolbar {
    bool active;
    Vector2 anchor;

    bool button_save_project_pressed;
    bool button_export_png_pressed;
    bool button_export_bundle_pressed;
} RSP_WidgetToolbar;

// -----------------------------------------------------------------------------
// Global data
// -----------------------------------------------------------------------------
static RSP_WidgetWelcome widget_welcome;
static RSP_WidgetToolbar widget_toolbar;

static RSP_Project current_project;

static RSP_Mode current_application_mode;

static Camera2D camera;

static bool should_show_alert = false;
static const char* alert_text = NULL;

// -----------------------------------------------------------------------------
// State data
// -----------------------------------------------------------------------------
struct {
    FilePathList files;
} WELCOME_STATE;

struct {
    RSP_Sprite* current_hovered_sprite;

    Vector2 mouse;

    bool show_sprite_name_editor;
} EDITOR_STATE;

// -----------------------------------------------------------------------------
// Function decleration
// -----------------------------------------------------------------------------
static RSP_WidgetWelcome RSP_InitWelcome (void);
static RSP_WidgetToolbar RSP_InitToolbar (void);

static void RSP_UpdateWelcome (void);
static void RSP_UpdateEditor (void);

static void RSP_RenderWelcome (void);
static void RSP_RenderEditor (void);

// Projects
RSP_ProjectError RSP_CreateAndLoadProject (const char* project_name);
RSP_ProjectError RSP_LoadProject (const char* project_file);
RSP_ProjectError RSP_SaveProject (void);

RSP_ProjectError RSP_UnloadProject (void);

// Editor
static int CompareTextureSizes (const void* a, const void* b);

static void LoadSprites (FilePathList files);
static void SortSprites (void);
static void RenderAtlas (void);

// Bundles
static void RSP_ExportBundle (void);
static void RSP_LoadBundle (void);

// Utility
void CopyFile (const char* source, const char* destination);
void ShowAlert (const char* text);

// -----------------------------------------------------------------------------
// Core Application
// -----------------------------------------------------------------------------
int main (int argc, const char* argv[]) {
    InitWindow (MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT, WINDOW_TITLE);

    { // :window settings
        SetWindowMinSize (MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);
        SetWindowState (WINDOW_FLAGS);
    }

    { // :framerate
        int fps = GetMonitorRefreshRate (GetCurrentMonitor ());
        SetTargetFPS (fps <= 0 ? 60 : fps);
    }

    { // :widgets
        widget_welcome = RSP_InitWelcome ();
        widget_toolbar = RSP_InitToolbar ();
    }

    current_application_mode = RSP_MODE_WELCOME;

    camera      = CLITERAL (Camera2D){0};
    camera.zoom = 1.0f;

    WELCOME_STATE.files = LoadDirectoryFilesEx (DEFAULT_PROJECT_DIRECTORY, DEFAULT_PROJECT_EXTENSION, true);

    while (!WindowShouldClose () && widget_welcome.active) {
        if (should_show_alert)
            GuiLock ();

        switch (current_application_mode) {
            case RSP_MODE_WELCOME:
                RSP_UpdateWelcome ();
                break;
            case RSP_MODE_EDITOR:
                RSP_UpdateEditor ();
                break;

            default:
                break;
        }

        BeginDrawing ();
        ClearBackground (GetColor (GuiGetStyle (DEFAULT, BACKGROUND_COLOR)));
        switch (current_application_mode) {
            case RSP_MODE_WELCOME:
                RSP_RenderWelcome ();
                break;
            case RSP_MODE_EDITOR:
                RSP_RenderEditor ();
                break;

            default:
                break;
        }

        if (should_show_alert) {
            GuiUnlock ();
            DrawRectangle (0, 0, GetScreenWidth (), GetScreenHeight (), GuiFade (GetColor (GuiGetStyle (DEFAULT, BACKGROUND_COLOR)), 0.75f));

            Rectangle window_bounds = CLITERAL (Rectangle){(GetScreenWidth () / 2) - 180, (GetScreenHeight ()) / 2 - 90, 360, 180};
            should_show_alert       = !GuiWindowBox (window_bounds, "#205#Alert!");

            Rectangle text_bounds = CLITERAL (Rectangle){(GetScreenWidth () / 2) - (180 - 8), (GetScreenHeight ()) / 2 - (90 - 32), 360 - 16, 180 - 40};
            GuiDrawText (alert_text, text_bounds, TEXT_ALIGN_CENTER, GetColor (GuiGetStyle (DEFAULT, TEXT_COLOR_NORMAL)));
        }

        if (EDITOR_STATE.show_sprite_name_editor) {
            DrawRectangle (0, 0, GetScreenWidth (), GetScreenHeight (), GuiFade (GetColor (GuiGetStyle (DEFAULT, BACKGROUND_COLOR)), 0.75f));

            const float bounds_width  = 256.0f;
            const float bounds_height = 48.0f;

            Rectangle bounds = CLITERAL (Rectangle){
                .x = ((GetScreenWidth () / 2) - (bounds_width / 2)),
                .y = ((GetScreenHeight () / 2) - (bounds_height / 2)),

                .width  = bounds_width,
                .height = bounds_height,
            };

            if (GuiTextBox (bounds, EDITOR_STATE.current_hovered_sprite->name, MAX_ASSET_NAME_LENGTH, true)) {
                EDITOR_STATE.show_sprite_name_editor = false;
            }
        }

#if BUILD_DEBUG
        DrawFPS (8, 8);
#endif
        EndDrawing ();
    }

    UnloadDirectoryFiles (WELCOME_STATE.files);

    RSP_UnloadProject ();

    CloseWindow ();

    return 0;
}

// -----------------------------------------------------------------------------
// UI
// -----------------------------------------------------------------------------
RSP_WidgetWelcome RSP_InitWelcome (void) {
    if (!DirectoryExists (DEFAULT_PROJECT_DIRECTORY))
        MakeDirectory (DEFAULT_PROJECT_DIRECTORY);

    RSP_WidgetWelcome state = CLITERAL (RSP_WidgetWelcome){0};

    state.anchor = Vector2Zero ();
    state.active = true;

    state.value_atlas_align = 16;

    strncpy (state.textbox_project_name_text, "Hello World", 12);

    state.width  = 640;
    state.height = 328;

    return state;
}

RSP_WidgetToolbar RSP_InitToolbar (void) {
    RSP_WidgetToolbar state = CLITERAL (RSP_WidgetToolbar){0};

    state.anchor = Vector2Zero ();
    state.active = false;

    return state;
}

void RSP_UpdateWelcome (void) {
    widget_welcome.anchor = CLITERAL (Vector2){
        (GetScreenWidth () / 2) - (widget_welcome.width / 2),
        (GetScreenHeight () / 2) - (widget_welcome.height / 2)};

    RSP_ProjectError result = RSP_PROJECT_ERROR_NULL;

    if (widget_welcome.button_new_project_pressed)
        result = RSP_CreateAndLoadProject (widget_welcome.textbox_project_name_text);
    if (widget_welcome.button_load_project_pressed)
        result = RSP_LoadProject (WELCOME_STATE.files.paths[widget_welcome.list_projects_active]);

    if (widget_welcome.button_delete_project_pressed)
        ShowAlert ("NOT YET IMPLIMENTED");

    if (IsFileDropped ()) {
        FilePathList files = LoadDroppedFiles ();

        for (size_t i = 0; i < files.count; i++) {
            const char* extension = GetFileExtension (files.paths[i]);

            if (TextIsEqual (extension, DEFAULT_PROJECT_EXTENSION)) {
                result = RSP_LoadProject (files.paths[i]);
                break;
            }
        }

        UnloadDroppedFiles (files);
    }

    switch (result) {
        case RSP_PROJECT_ERROR_NONE:
            current_application_mode = RSP_MODE_EDITOR;
            break;

        case RSP_PROJECT_ERROR_EXISTS:
            ShowAlert (TextFormat ("Project with name %s already exists!", widget_welcome.textbox_project_name_text));
            break;

        case RSP_PROJECT_ERROR_FAILED_READ:
            ShowAlert ("Could not pass project file!");
            break;

        case RSP_PROJECT_ERROR_FAILED_WRITE:
            ShowAlert ("Could not write project file!");
            break;

        default:
            break;
    }
}

void RSP_UpdateEditor (void) {
    EDITOR_STATE.mouse = GetScreenToWorld2D (GetMousePosition (), camera);

    if (GuiIsLocked ())
        return;

    if (IsMouseButtonDown (MOUSE_BUTTON_MIDDLE) || IsKeyDown (KEY_LEFT_ALT)) {
        Vector2 delta = GetMouseDelta ();

        delta         = Vector2Scale (delta, -1.0f / camera.zoom);
        camera.target = Vector2Add (camera.target, delta);
    }

    float wheel = GetMouseWheelMove ();

    if (wheel != 0) {
        camera.offset = GetMousePosition ();
        camera.target = EDITOR_STATE.mouse;

        const float zoom_amount = 0.125f * (IsKeyDown (KEY_LEFT_CONTROL) + 1);

        camera.zoom += (wheel * zoom_amount);
        if (camera.zoom < zoom_amount)
            camera.zoom = zoom_amount;
    }

    if (IsFileDropped ()) {
        FilePathList files = LoadDroppedFiles ();

        LoadSprites (files);
        SortSprites ();
        RenderAtlas ();

        UnloadDroppedFiles (files);
    }

    if (widget_toolbar.button_save_project_pressed)
        RSP_SaveProject ();

    if (widget_toolbar.button_export_png_pressed) {
        const char* filename = TextFormat ("%s/%s/%s", DEFAULT_PROJECT_DIRECTORY, current_project.name, "atlas.png");

        if (FileExists (filename))
            remove (filename);

        Image atlas = LoadImageFromTexture (current_project.atlas.texture);
        ImageFlipVertical (&atlas);

        ExportImage (atlas, filename);
        UnloadImage (atlas);

        ShowAlert ("Atlas exported!");
    }

    if (widget_toolbar.button_export_bundle_pressed) {
        RSP_ExportBundle ();
    }

    if (!EDITOR_STATE.show_sprite_name_editor) {
        EDITOR_STATE.current_hovered_sprite = NULL;

        for (size_t i = 0; i < current_project.sprites_count; i++) {
            RSP_Sprite* sprite = &current_project.sprites[i];

            if (CheckCollisionPointRec (EDITOR_STATE.mouse, sprite->source)) {
                EDITOR_STATE.current_hovered_sprite = sprite;
                break;
            }
        }
    }

    if (GetMousePosition ().y > 48) {
        if (EDITOR_STATE.current_hovered_sprite != NULL) {
            if (IsMouseButtonReleased (MOUSE_BUTTON_LEFT)) {
                Vector2 origin_offset = CLITERAL (Vector2){
                    EDITOR_STATE.current_hovered_sprite->source.x,
                    EDITOR_STATE.current_hovered_sprite->source.y,
                };

                Vector2 new_origin = Vector2Subtract (EDITOR_STATE.mouse, origin_offset);

                EDITOR_STATE.current_hovered_sprite->origin = CLITERAL (Vector2){
                    roundf (new_origin.x),
                    roundf (new_origin.y),
                };
            }

            if (IsMouseButtonReleased (MOUSE_BUTTON_RIGHT)) {
                EDITOR_STATE.show_sprite_name_editor = true;
            }
        }
    }
}

void RSP_RenderWelcome (void) {
    if (widget_welcome.dropdown_atlas_size_edit)
        GuiLock ();

    if (widget_welcome.active) {
        const Rectangle layouts[] = {
            // NOTE: Yes I know this is not efficient
            CLITERAL (Rectangle){widget_welcome.anchor.x + 0,   widget_welcome.anchor.y + 0,   widget_welcome.width, widget_welcome.height},
            CLITERAL (Rectangle){widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 32,  624,                  128                  },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 168, 304,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 328, widget_welcome.anchor.y + 168, 304,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 8,   widget_welcome.anchor.y + 208, 624,                  112                  },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 472, widget_welcome.anchor.y + 288, 152,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 224, 96,                   24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 224, 128,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 288, 96,                   24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 288, 128,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 16,  widget_welcome.anchor.y + 256, 128,                  24                   },
            CLITERAL (Rectangle){widget_welcome.anchor.x + 152, widget_welcome.anchor.y + 256, 96,                   24                   },
        };

        // Temp for now
        widget_welcome.active = !GuiWindowBox (layouts[0], "#015#Projects");
        GuiSetStyle (LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiListViewEx (layouts[1], (const char**)WELCOME_STATE.files.paths, WELCOME_STATE.files.count, &widget_welcome.list_projects_scroll_index, &widget_welcome.list_projects_active, NULL);

        widget_welcome.button_load_project_pressed   = GuiButton (layouts[2], "#005#Load Selected Project");
        widget_welcome.button_delete_project_pressed = GuiButton (layouts[3], "#143#Delete Selected Project");

        GuiGroupBox (layouts[4], "New Project");

        {
            widget_welcome.button_new_project_pressed = GuiButton (layouts[5], "#008#Create & Load Project");

            GuiLabel (layouts[6], "Project Name");
            if (GuiTextBox (layouts[7], widget_welcome.textbox_project_name_text, 128, widget_welcome.textbox_project_name_edit))
                TOGGLE (widget_welcome.textbox_project_name_edit);

            GuiLabel (layouts[8], "Atlas Size");
            if (GuiDropdownBox (layouts[9], "512;1024;2048;4096;8192", &widget_welcome.dropdown_atlas_size_active, widget_welcome.dropdown_atlas_size_edit))
                TOGGLE (widget_welcome.dropdown_atlas_size_edit);

            GuiLabel (layouts[11], "Alignment Value");
            if (GuiValueBox (layouts[10], NULL, &widget_welcome.value_atlas_align, 0, 128, widget_welcome.value_atlas_align_edit))
                TOGGLE (widget_welcome.value_atlas_align_edit);
        }
    }

    GuiUnlock ();
}

static void __RSP_Toolbar (void) {
    GuiStatusBar (CLITERAL (Rectangle){widget_toolbar.anchor.x, widget_toolbar.anchor.y, GetRenderWidth (), 48}, NULL);

    GuiEnableTooltip ();

    GuiSetTooltip ("Save Project");
    widget_toolbar.button_save_project_pressed = GuiButton (CLITERAL (Rectangle){8, 8, 32, 32}, "#2#");

    GuiSetTooltip ("Export Atlas as PNG");
    widget_toolbar.button_export_png_pressed = GuiButton (CLITERAL (Rectangle){48, 8, 32, 32}, "#195#");

    GuiSetTooltip ("Export Bundle");
    widget_toolbar.button_export_bundle_pressed = GuiButton (CLITERAL (Rectangle){88, 8, 32, 32}, "#200#");

    GuiDisableTooltip ();
}

void RSP_RenderEditor (void) {
    BeginMode2D (camera);

    DrawRectangleLines (-1, -1, current_project.atlas_size + 2, current_project.atlas_size + 2, RED);
    DrawTexturePro (current_project.atlas.texture, CLITERAL (Rectangle){0, 0, current_project.atlas_size, -current_project.atlas_size}, CLITERAL (Rectangle){0, 0, current_project.atlas_size, current_project.atlas_size}, Vector2Zero (), 0.0f, WHITE);

    if (EDITOR_STATE.current_hovered_sprite != NULL) {
        DrawRectangleLinesEx (EDITOR_STATE.current_hovered_sprite->source, 0.1, GREEN);

        DrawLineV (
            CLITERAL (Vector2){.x = EDITOR_STATE.current_hovered_sprite->source.x, .y = EDITOR_STATE.mouse.y},
            CLITERAL (Vector2){.x = EDITOR_STATE.current_hovered_sprite->source.x + EDITOR_STATE.current_hovered_sprite->source.width, .y = EDITOR_STATE.mouse.y},
            WHITE);

        DrawLineV (
            CLITERAL (Vector2){.x = EDITOR_STATE.mouse.x, .y = EDITOR_STATE.current_hovered_sprite->source.y},
            CLITERAL (Vector2){.x = EDITOR_STATE.mouse.x, .y = EDITOR_STATE.current_hovered_sprite->source.y + EDITOR_STATE.current_hovered_sprite->source.height},
            WHITE);

        Vector2 origin = Vector2Add (
            EDITOR_STATE.current_hovered_sprite->origin,
            CLITERAL (Vector2){
                EDITOR_STATE.current_hovered_sprite->source.x,
                EDITOR_STATE.current_hovered_sprite->source.y});

        DrawCircleV (origin, 1.0f, Fade (RED, 0.5f));
        DrawCircleLinesV (origin, 1.0f, RED);
    }

    EndMode2D ();

    __RSP_Toolbar ();
}

// -----------------------------------------------------------------------------
// Projects
// -----------------------------------------------------------------------------
RSP_ProjectError RSP_CreateAndLoadProject (const char* project_name) {
    if (!DirectoryExists (DEFAULT_PROJECT_DIRECTORY))
        MakeDirectory (DEFAULT_PROJECT_DIRECTORY);

    const char* directory   = TextFormat ("%s/%s", DEFAULT_PROJECT_DIRECTORY, project_name);
    char* project_directory = MemAlloc (TextLength (directory));
    TextCopy (project_directory, directory);

    const char* file   = TextFormat ("%s/project%s", project_directory, DEFAULT_PROJECT_EXTENSION);
    char* project_file = MemAlloc (TextLength (file));
    TextCopy (project_file, file);

    if (FileExists (project_file))
        return RSP_PROJECT_ERROR_EXISTS;

    MakeDirectory (project_directory);
    MakeDirectory (TextFormat ("%s/textures", project_directory));

    const uint16_t atlas_sizes[5] = {512, 1024, 2048, 4096, 8192};

    {
        current_project.version = DEFAULT_PROJECT_VERSION,
        strncpy (current_project.name, widget_welcome.textbox_project_name_text, MAX_PROJECT_NAME_LENGTH);

        current_project.alignment  = widget_welcome.value_atlas_align;
        current_project.atlas_size = atlas_sizes[widget_welcome.dropdown_atlas_size_active];
    }

    RSP_ProjectError status = RSP_SaveProject ();

    if (status == RSP_PROJECT_ERROR_FAILED_WRITE)
        goto panic;

    status = RSP_LoadProject (project_file);

panic:
    MemFree (project_directory);
    MemFree (project_file);

    return status;
}

RSP_ProjectError RSP_LoadProject (const char* project_file) {
    if (!FileExists (project_file))
        return RSP_PROJECT_ERROR_NOT_EXIST;

    JSON_Value* root = json_parse_file (project_file);
    if (!root)
        return RSP_PROJECT_ERROR_FAILED_READ;

    JSON_Object* root_object = json_value_get_object (root);

    strncpy (current_project.name, json_object_get_string (root_object, "name"), MAX_PROJECT_NAME_LENGTH);
    current_project.version    = (uint8_t)json_object_get_number (root_object, "version");
    current_project.atlas_size = (uint16_t)json_object_get_number (root_object, "atlas_size");
    current_project.alignment  = (uint8_t)json_object_get_number (root_object, "alignment");

    current_project.atlas = LoadRenderTexture (current_project.atlas_size, current_project.atlas_size);

    JSON_Array* sprites_array     = json_object_get_array (root_object, "sprites");
    current_project.sprites_count = (uint16_t)json_array_get_count (sprites_array);

    current_project.sprites = MemAlloc (sizeof (RSP_Sprite) * current_project.sprites_count);

    for (size_t i = 0; i < current_project.sprites_count; i++) {
        JSON_Object* sprite_object = json_array_get_object (sprites_array, i);

        current_project.sprites[i] = CLITERAL (RSP_Sprite){0};
        RSP_Sprite* sprite         = &current_project.sprites[i];

        strncpy (sprite->name, json_object_get_string (sprite_object, "name"), MAX_ASSET_NAME_LENGTH);
        strncpy (sprite->file, json_object_get_string (sprite_object, "file"), MAX_ASSET_FILE_LENGTH);

        sprite->flags = (uint16_t)json_object_get_number (sprite_object, "flags");

        sprite->source = CLITERAL (Rectangle){
            (float)json_object_dotget_number (sprite_object, "source.x"),
            (float)json_object_dotget_number (sprite_object, "source.y"),
            (float)json_object_dotget_number (sprite_object, "source.width"),
            (float)json_object_dotget_number (sprite_object, "source.height"),
        };

        sprite->origin = CLITERAL (Vector2){
            (float)json_object_dotget_number (sprite_object, "origin.x"),
            (float)json_object_dotget_number (sprite_object, "origin.y"),
        };

        if (FileExists (sprite->file)) {
            sprite->texture = LoadTexture (sprite->file);
        } else
            TraceLog (LOG_ERROR, "Could not fine file %s!", sprite->file);
    }

    json_value_free (root);

    RenderAtlas ();

    return RSP_PROJECT_ERROR_NONE;
}

RSP_ProjectError RSP_SaveProject (void) {
    const char* directory   = TextFormat ("%s/%s", DEFAULT_PROJECT_DIRECTORY, current_project.name);
    char* project_directory = MemAlloc (TextLength (directory));
    TextCopy (project_directory, directory);

    const char* file   = TextFormat ("%s/project%s", project_directory, DEFAULT_PROJECT_EXTENSION);
    char* project_file = MemAlloc (TextLength (file));
    TextCopy (project_file, file);

    const char* backup_file = TextFormat ("%s.bkp", project_file);
    if (FileExists (backup_file))
        remove (backup_file);

    if (FileExists (project_file)) {
        CopyFile (project_file, backup_file);

        remove (project_file);
    }

    {
        JSON_Value* root         = json_value_init_object ();
        JSON_Object* root_object = json_value_get_object (root);

        json_object_set_string (root_object, "name", current_project.name);
        json_object_set_number (root_object, "version", current_project.version);
        json_object_set_number (root_object, "atlas_size", current_project.atlas_size);
        json_object_set_number (root_object, "alignment", current_project.alignment);

        json_object_set_boolean (root_object, "embed_files", current_project.should_embed_files);

        JSON_Value* sprites_value = json_value_init_array ();
        JSON_Array* sprites_array = json_value_get_array (sprites_value);

        for (size_t i = 0; i < current_project.sprites_count; i++) {
            RSP_Sprite* sprite = &current_project.sprites[i];

            JSON_Value* sprite_value   = json_value_init_object ();
            JSON_Object* sprite_object = json_value_get_object (sprite_value);

            // Generic data
            json_object_set_string (sprite_object, "name", sprite->name);
            json_object_set_string (sprite_object, "file", sprite->file);
            json_object_set_number (sprite_object, "flags", (double)sprite->flags);

            // Sprite source
            json_object_dotset_number (sprite_object, "source.x", sprite->source.x);
            json_object_dotset_number (sprite_object, "source.y", sprite->source.y);
            json_object_dotset_number (sprite_object, "source.width", sprite->source.width);
            json_object_dotset_number (sprite_object, "source.height", sprite->source.height);

            // Origin
            json_object_dotset_number (sprite_object, "origin.x", sprite->origin.x);
            json_object_dotset_number (sprite_object, "origin.y", sprite->origin.y);

            // Animation
            json_object_dotset_number (sprite_object, "animation.speed", sprite->animation.frames_speed);
            json_object_dotset_number (sprite_object, "animation.frame_count", sprite->animation.frames_count);

            JSON_Value* frames_value = json_value_init_array ();
            // JSON_Array* frames_array = json_value_get_array (sprites_value);

            // TODO: Actually add it... Maybe
            for (size_t j = 0; j < sprite->animation.frames_count; j++);

            json_object_dotset_value (sprite_object, "animation.frames", frames_value);

            json_array_append_value (sprites_array, sprite_value);
        }

        json_object_set_value (root_object, "sprites", sprites_value);

        JSON_Status status = json_serialize_to_file_pretty (root, project_file);

        json_value_free (root);

        if (status == JSONError)
            return RSP_PROJECT_ERROR_FAILED_WRITE;
    }

    MemFree (project_directory);
    MemFree (project_file);

    return RSP_PROJECT_ERROR_NONE;
}

RSP_ProjectError RSP_UnloadProject (void) {
    for (size_t i = 0; i < current_project.sprites_count; i++) {
        RSP_Sprite* sprite = &current_project.sprites[i];

        UnloadTexture (sprite->texture);

        if (sprite->animation.frames_count > 0)
            MemFree (sprite->animation.frames);
    }

    MemFree (current_project.sprites);

    UnloadDirectoryFiles (current_project.assets);

    current_project.assets.count  = 0;
    current_project.sprites_count = 0;

    return RSP_PROJECT_ERROR_NONE;
}

// -----------------------------------------------------------------------------
// Editor
// -----------------------------------------------------------------------------
static int CompareTextureSizes (const void* a, const void* b) {
    RSP_Sprite* sprite_a = (RSP_Sprite*)a;
    RSP_Sprite* sprite_b = (RSP_Sprite*)b;

    int mass_a = (int)(sprite_a->source.width * sprite_a->source.height);
    int mass_b = (int)(sprite_b->source.width * sprite_b->source.height);

    if (mass_a == mass_b) {
        if ((int)sprite_a->source.width == (int)sprite_b->source.width) {
            if (sprite_a->source.height > sprite_b->source.height)
                return -1;
            if (sprite_b->source.height > sprite_a->source.height)
                return 1;

            return 0;
        } else {
            if (sprite_a->source.width > sprite_b->source.width)
                return -1;
            if (sprite_b->source.width > sprite_a->source.width)
                return 1;

            return 0;
        }
    } else {
        if (mass_a > mass_b)
            return -1;
        if (mass_b > mass_a)
            return 1;
    }

    return 0;
}

static bool __is_image (const char* filename) {
    const char* accepted_formats[] = {".png", ".bmp", ".tga", ".jpg", ".jpeg", ".gif", ".qoi", ".psd;", ".dds", ".hdr", ".ktx", ".astc", ".pkm", ".pvr"};

    for (size_t i = 0; i < lengthof (accepted_formats); i++) {
        if (TextIsEqual (filename, accepted_formats[i]))
            return true;
    }

    return false;
}

void LoadSprites (FilePathList files) {
    if (files.count <= 0) {
        ShowAlert ("No files to load!");
        return;
    }

    uint16_t filtered_file_count = 0;
    const char* filtered_files[files.count];

    for (size_t i = 0; i < files.count; i++) {
        const char* extension = GetFileExtension (files.paths[i]);

        if (__is_image (extension)) {
            filtered_files[filtered_file_count] = files.paths[i];
            filtered_file_count++;
        }
    }

    current_project.sprites = MemRealloc (current_project.sprites, (current_project.sprites_count + filtered_file_count) * sizeof (RSP_Sprite));

    for (size_t i = 0; i < filtered_file_count; i++) {
        const char* new_filename = TextFormat ("projects/%s/textures/%d%s", current_project.name, current_project.sprites_count, GetFileExtension (filtered_files[i]));
        CopyFile (filtered_files[i], new_filename);

        current_project.sprites[current_project.sprites_count] = CLITERAL (RSP_Sprite){0};
        RSP_Sprite* sprite                                     = &current_project.sprites[current_project.sprites_count];

        strncpy (sprite->name, GetFileNameWithoutExt (filtered_files[i]), MAX_ASSET_NAME_LENGTH);
        strncpy (sprite->file, new_filename, MAX_ASSET_FILE_LENGTH);

        sprite->texture = LoadTexture (filtered_files[i]);
        if (sprite->texture.width > current_project.atlas_size || sprite->texture.height > current_project.atlas_size) {
            ShowAlert (TextFormat ("Texture [%s] too large! Aborting!", sprite->name));

            UnloadTexture (sprite->texture);
            remove (new_filename);

            break;
        }

        sprite->source = CLITERAL (Rectangle){0, 0, sprite->texture.width, sprite->texture.height};
        sprite->origin = CLITERAL (Vector2){0};

        current_project.sprites_count++;
    }
}

void SortSprites (void) {
    qsort (current_project.sprites, current_project.sprites_count, sizeof (RSP_Sprite), CompareTextureSizes);
    Rectangle atlas_bounds = CLITERAL (Rectangle){0, 0, current_project.atlas_size, current_project.atlas_size};

    int textures_placed = 0;
    for (size_t i = 0; i < current_project.sprites_count; i++) {
        Rectangle* current_rectangle = &current_project.sprites[i].source;

        for (size_t j = 0; j < textures_placed; j++) {
            while (CheckCollisionRecs (*current_rectangle, current_project.sprites[j].source)) {
                current_rectangle->x += current_project.sprites[j].source.width;

                int within_x = (current_rectangle->x + current_rectangle->width) <= current_project.atlas_size;

                if (!within_x) {
                    current_rectangle->x = 0;
                    current_rectangle->y += current_project.alignment;

                    j = 0;
                }
            }
        }

        bool inside_top_left     = (current_rectangle->x >= atlas_bounds.x && current_rectangle->y >= atlas_bounds.y);
        bool inside_bottom_right = ((current_rectangle->x + current_rectangle->width) <= (atlas_bounds.x + atlas_bounds.width) &&
                                    (current_rectangle->y + current_rectangle->height) <= (atlas_bounds.y + atlas_bounds.height));

        if (!inside_top_left || !inside_bottom_right) {
            for (size_t k = textures_placed; k < current_project.sprites_count; k++) {
                RSP_Sprite* sprite = &current_project.sprites[k];

                UnloadTexture (sprite->texture);

                if (sprite->animation.frames_count > 0)
                    MemFree (sprite->animation.frames);
            }

            current_project.sprites_count = textures_placed;

            break;
        }

        textures_placed++;
    }
}

void RenderAtlas (void) {
    BeginTextureMode (current_project.atlas);
    ClearBackground (Fade (BLACK, 0));

    for (size_t i = 0; i < current_project.sprites_count; i++) {
        RSP_Sprite* sprite = &current_project.sprites[i];

        DrawTextureV (sprite->texture, CLITERAL (Vector2){sprite->source.x, sprite->source.y}, WHITE);
    }
    EndTextureMode ();
}

// -----------------------------------------------------------------------------
// Bundles
// -----------------------------------------------------------------------------
void RSP_ExportBundle (void) {
    const char* file = TextFormat ("%s/%s/bundle%s", DEFAULT_PROJECT_DIRECTORY, current_project.name, DEFAULT_BUNDLE_EXTENSION);

    FILE* output = fopen (file, "wb");

    int32_t atlas_data_raw_size, atlas_data_compressed_size;

    Image atlas_image = LoadImageFromTexture (current_project.atlas.texture);
    ImageFlipVertical (&atlas_image);

    unsigned char* image_data_raw        = ExportImageToMemory (atlas_image, ".png", &atlas_data_raw_size);
    unsigned char* image_data_compressed = CompressData (image_data_raw, atlas_data_raw_size, &atlas_data_compressed_size);

    MemFree (image_data_raw);
    UnloadImage (atlas_image);

    const char* file_type = "RSPX";

    fwrite (file_type, sizeof (char), 4, output);

    fwrite (&current_project.sprites_count, sizeof (uint16_t), 1, output);
    fwrite (&atlas_data_compressed_size, sizeof (int32_t), 1, output);
    fwrite (image_data_compressed, sizeof (unsigned char), atlas_data_compressed_size, output);

    for (size_t i = 0; i < current_project.sprites_count; i++) {
        RSP_Sprite* sprite = &current_project.sprites[i];

        // Small header
        fwrite ("SPR", sizeof (char), 4, output);

        fwrite (sprite->name, sizeof (char), MAX_ASSET_NAME_LENGTH, output);
        fwrite (&sprite->flags, sizeof (uint16_t), 1, output);

        fwrite (&sprite->origin.x, sizeof (float), 1, output);
        fwrite (&sprite->origin.y, sizeof (float), 1, output);

        fwrite (&sprite->source.x, sizeof (float), 1, output);
        fwrite (&sprite->source.y, sizeof (float), 1, output);
        fwrite (&sprite->source.width, sizeof (float), 1, output);
        fwrite (&sprite->source.height, sizeof (float), 1, output);

        if (sprite->flags & RSP_SPRITE_ANIMATED) {
            fwrite (&sprite->animation.frames_count, sizeof (uint16_t), 1, output);
            fwrite (&sprite->animation.frames_speed, sizeof (uint16_t), 1, output);

            for (size_t j = 0; j < sprite->animation.frames_count; j++) {
                fwrite (&sprite->animation.frames[i].x, sizeof (float), 1, output);
                fwrite (&sprite->animation.frames[i].y, sizeof (float), 1, output);
                fwrite (&sprite->animation.frames[i].width, sizeof (float), 1, output);
                fwrite (&sprite->animation.frames[i].height, sizeof (float), 1, output);
            }
        }
    }

    MemFree (image_data_compressed);

    fclose (output);

    const char* header = TextFormat ("%s/%s/bundle%s", DEFAULT_PROJECT_DIRECTORY, current_project.name, ".h");

    FILE* header_output = fopen (header, "w");

    fprintf (header_output, "#ifndef RSP_SPRITE_NAMES\n");
    fprintf (header_output, "#define RSP_SPRITE_NAMES\n");
    fprintf (header_output, "\ntypedef enum RSP_SpriteName {\n");

    for (size_t i = 0; i < current_project.sprites_count; i++) {
        fprintf (header_output, "\tSPRITE_%s = %zu,\n", TextToUpper (current_project.sprites[i].name), i);
    }

    fprintf (header_output, "} RSP_SpriteName\n");
    fprintf (header_output, "\n#endif // RSP_SPRITE_NAMES;\n");

    fclose (header_output);

    ShowAlert ("Bundle exported!");
}

void RSP_LoadBundle (void) {
    const char* file = TextFormat ("%s/%s/bundle%s", DEFAULT_PROJECT_DIRECTORY, current_project.name, DEFAULT_BUNDLE_EXTENSION);

    FILE* import = fopen (file, "rb");

    if (!import)
        goto free;

    uint16_t sprite_count;
    int32_t atlas_data_raw_size, atlas_data_compressed_size;

    fread (&sprite_count, sizeof (uint16_t), 1, import);
    fread (&atlas_data_compressed_size, sizeof (int32_t), 1, import);

    unsigned char* atlas_data_compressed = calloc (atlas_data_compressed_size, sizeof (unsigned char));
    unsigned char* atlas_data_raw;

    fread (atlas_data_compressed, sizeof (unsigned char), atlas_data_compressed_size, import);
    atlas_data_raw = DecompressData (atlas_data_compressed, atlas_data_compressed_size, &atlas_data_raw_size);

    // Load atlas here

    MemFree (atlas_data_compressed);
    MemFree (atlas_data_raw);

    RSP_Sprite* sprites = calloc (sprite_count, sizeof (RSP_Sprite));
    char* header        = calloc (4, sizeof (char));

    for (size_t i = 0; i < sprite_count; i++) {
        fread (header, sizeof (char), 4, import);

        RSP_Sprite* sprite = &sprites[i];

        fread (sprite->name, sizeof (char), MAX_ASSET_NAME_LENGTH, import);
        fread (&sprite->flags, sizeof (uint16_t), 1, import);

        fread (&sprite->origin.x, sizeof (float), 1, import);
        fread (&sprite->origin.y, sizeof (float), 1, import);

        fread (&sprite->source.x, sizeof (float), 1, import);
        fread (&sprite->source.y, sizeof (float), 1, import);
        fread (&sprite->source.width, sizeof (float), 1, import);
        fread (&sprite->source.height, sizeof (float), 1, import);

        if (sprite->flags & RSP_SPRITE_ANIMATED) {
            fread (&sprite->animation.frames_count, sizeof (uint16_t), 1, import);
            fread (&sprite->animation.frames_speed, sizeof (uint16_t), 1, import);

            sprite->animation.frames = calloc (sprite->animation.frames_count, sizeof (Rectangle));

            for (size_t j = 0; j < sprite->animation.frames_count; j++) {
                fread (&sprite->animation.frames[j].x, sizeof (float), 1, import);
                fread (&sprite->animation.frames[j].y, sizeof (float), 1, import);
                fread (&sprite->animation.frames[j].width, sizeof (float), 1, import);
                fread (&sprite->animation.frames[j].height, sizeof (float), 1, import);
            }
        }
    }

    for (size_t i = 0; i < sprite_count; i++) {
        RSP_Sprite* sprite = &sprites[i];

        MemFree (sprite->animation.frames);
    }

    MemFree (header);
    MemFree (sprites);

free:
    fclose (import);
}

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------
void CopyFile (const char* source, const char* destination) {
    unsigned char* source_file;
    int file_length;

    source_file = LoadFileData (source, &file_length);

    SaveFileData (destination, source_file, file_length);

    MemFree (source_file);
}

void ShowAlert (const char* text) {
    should_show_alert = true;
    alert_text        = text;
}
