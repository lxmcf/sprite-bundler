#ifndef WIDGET_TOOLBAR_H
#define WIDGET_TOOLBAR_H

#include <raygui.h>

typedef struct WGT_ToolbarState {
    Vector2 anchor;

    bool button_new_project_pressed;
    bool button_load_project_pressed;
} WGT_ToolbarState;

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

WGT_ToolbarState WGT_InitToolbarState (void);
void WGT_GuiToolbar (WGT_ToolbarState* state);

#define WGT_TOOLBAR_IMPL
#ifdef WGT_TOOLBAR_IMPL

WGT_ToolbarState WGT_InitToolbarState (void) {
    WGT_ToolbarState state = { 0 };

    state.anchor = CLITERAL(Vector2){ 0, 0 };

    state.button_new_project_pressed = false;
    state.button_load_project_pressed = false;

    GuiEnableTooltip ();

    return state;
}

void WGT_GuiToolbar (WGT_ToolbarState* state) {
    GuiPanel (CLITERAL(Rectangle){ state->anchor.x, state->anchor.y, GetScreenWidth (), 40 }, NULL);

    GuiSetTooltip ("New Project");
    state->button_new_project_pressed = GuiButton((Rectangle){ state->anchor.x + 6, state->anchor.y + 6, 28, 28 }, "#8#");
    GuiSetTooltip ("Load Project");
    state->button_load_project_pressed = GuiButton((Rectangle){ state->anchor.x + 40, state->anchor.y + 6, 28, 28 }, "#5#");
}

#endif

#ifdef __cplusplus
}
#endif


#endif // WIDGET_TOOLBAR_H
