#ifndef WELCOME_H
#define WELCOME_H

#include <raygui.h>

typedef struct WGT_WelcomeState {
    Vector2 anchor;

    FilePathList files;

    int width;
    int height;

    int scrollIndex;
    int activeSelection;
    int focusSelection;
    bool load_selected_project;

    bool textbox_create_project_selected;

    char textbox_create_project[128];
    bool button_create_project;
} WGT_WelcomeState;

#ifdef __cplusplus
extern "C" {
#endif

WGT_WelcomeState WGT_InitWelcomeState (void);
void WGT_GuiWelcome (WGT_WelcomeState* state);

#ifdef WGT_WELCOME_IMPL

WGT_WelcomeState WGT_InitWelcomeState (void) {
    WGT_WelcomeState state = { 0 };

    state.anchor = CLITERAL(Vector2){ 0, 0 };

    state.files = LoadDirectoryFilesEx ("projects", ".rspp", true);
    state.activeSelection = -1;

    state.width = 640;
    state.height = 360;

    state.button_create_project = false;

    return state;
}

void WGT_GuiWelcome (WGT_WelcomeState* state) {
    Rectangle bounds = { 0 };

    Vector2 window_middle = CLITERAL(Vector2) { GetScreenWidth () / 2, GetScreenHeight () / 2, };
    state->anchor = CLITERAL(Vector2) { .x = window_middle.x - (state->width / 2), .y = window_middle.y - (state->height / 2), };

    bounds = CLITERAL(Rectangle) { .x = state->anchor.x, .y = state->anchor.y, .width = state->width, .height = state->height, };
    GuiWindowBox (bounds, "#15# Prjects");

    // -----------------------------------------------------------------------------
    // Projects
    // -----------------------------------------------------------------------------
    GuiSetStyle (LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    bounds = CLITERAL(Rectangle) { .x = state->anchor.x + 8, .y = state->anchor.y + 32, .width = state->width - 16, .height = 124, };
    GuiListViewEx (bounds, (const char**)state->files.paths, state->files.count, &state->scrollIndex, &state->activeSelection, &state->focusSelection);

    // -----------------------------------------------------------------------------
    // Load Project
    // -----------------------------------------------------------------------------
    bounds = CLITERAL(Rectangle) { .x = state->anchor.x + 8, .y = state->anchor.y + 164, .width = state->width - 16, .height = 32, };
    state->load_selected_project = GuiButton (bounds, "#5#Load Selected Project");

    // -----------------------------------------------------------------------------
    // Textbox
    // -----------------------------------------------------------------------------
    GuiSetStyle (TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    bounds = CLITERAL(Rectangle) { .x = state->anchor.x + 8, .y = (state->anchor.y + state->height) - 40, .width = 304, .height = 32, };
    if (GuiTextBox (bounds, state->textbox_create_project, 64, state->textbox_create_project_selected)) state->textbox_create_project_selected = !state->textbox_create_project_selected;

    // -----------------------------------------------------------------------------
    // New project
    // -----------------------------------------------------------------------------
    bounds = CLITERAL(Rectangle) { .x = (state->anchor.x + (state->width / 2)) + 8, .y = (state->anchor.y + state->height) - 40, .width = 304, .height = 32, };
    state->button_create_project = GuiButton (bounds, "#6#New Project");
}

#endif

#ifdef __cplusplus
}
#endif

#endif // WELCOME_H
