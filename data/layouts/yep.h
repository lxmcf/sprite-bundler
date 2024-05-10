/*******************************************************************************************
*
*   LayoutName v1.0.0 - Tool Description
*
*   MODULE USAGE:
*       #define GUI_LAYOUT_NAME_IMPLEMENTATION
*       #include "gui_layout_name.h"
*
*       INIT: GuiLayoutNameState state = InitGuiLayoutName();
*       DRAW: GuiLayoutName(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>     // Required for: strcpy()

#ifndef GUI_LAYOUT_NAME_H
#define GUI_LAYOUT_NAME_H

typedef struct {
    bool WindowBox000Active;
    int ListView001ScrollIndex;
    int ListView001Active;
    bool Button002Pressed;
    bool Button003Pressed;
    bool Button005Pressed;
    bool TextBox007EditMode;
    char TextBox007Text[128];
    bool DropdownBox009EditMode;
    int DropdownBox009Active;
    bool ValueBOx010EditMode;
    int ValueBOx010Value;
    bool LabelButton011Pressed;

    Rectangle layoutRecs[12];

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required

} GuiLayoutNameState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiLayoutNameState InitGuiLayoutName(void);
void GuiLayoutName(GuiLayoutNameState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_LAYOUT_NAME_H

/***********************************************************************************
*
*   GUI_LAYOUT_NAME IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_LAYOUT_NAME_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiLayoutNameState InitGuiLayoutName(void)
{
    GuiLayoutNameState state = { 0 };

    state.WindowBox000Active = true;
    state.ListView001ScrollIndex = 0;
    state.ListView001Active = 0;
    state.Button002Pressed = false;
    state.Button003Pressed = false;
    state.Button005Pressed = false;
    state.TextBox007EditMode = false;
    strcpy(state.TextBox007Text, "Ludum Dare 100");
    state.DropdownBox009EditMode = false;
    state.DropdownBox009Active = 0;
    state.ValueBOx010EditMode = false;
    state.ValueBOx010Value = 0;
    state.LabelButton011Pressed = false;

    state.layoutRecs[0] = (Rectangle){ 0, 0, 640, 328 };
    state.layoutRecs[1] = (Rectangle){ 8, 32, 624, 128 };
    state.layoutRecs[2] = (Rectangle){ 8, 168, 304, 24 };
    state.layoutRecs[3] = (Rectangle){ 328, 168, 304, 24 };
    state.layoutRecs[4] = (Rectangle){ 8, 208, 624, 112 };
    state.layoutRecs[5] = (Rectangle){ 472, 288, 152, 24 };
    state.layoutRecs[6] = (Rectangle){ 152, 224, 96, 24 };
    state.layoutRecs[7] = (Rectangle){ 16, 224, 128, 24 };
    state.layoutRecs[8] = (Rectangle){ 152, 288, 96, 24 };
    state.layoutRecs[9] = (Rectangle){ 16, 288, 128, 24 };
    state.layoutRecs[10] = (Rectangle){ 16, 256, 128, 24 };
    state.layoutRecs[11] = (Rectangle){ 152, 256, 96, 24 };

    // Custom variables initialization

    return state;
}

void GuiLayoutName(GuiLayoutNameState *state)
{
    if (state->DropdownBox009EditMode) GuiLock();

    if (state->WindowBox000Active)
    {
        state->WindowBox000Active = !GuiWindowBox(state->layoutRecs[0], "#015#Projects");
        GuiListView(state->layoutRecs[1], "ONE;TWO;THREE;FOUR;FIVE", &state->ListView001ScrollIndex, &state->ListView001Active);
        state->Button002Pressed = GuiButton(state->layoutRecs[2], "#005#Load Selected Project"); 
        state->Button003Pressed = GuiButton(state->layoutRecs[3], "#143#Delete Selected Project"); 
        GuiGroupBox(state->layoutRecs[4], "New Project");
        state->Button005Pressed = GuiButton(state->layoutRecs[5], "#008#Create & Load Project"); 
        GuiLabel(state->layoutRecs[6], "Project Name");
        if (GuiTextBox(state->layoutRecs[7], state->TextBox007Text, 128, state->TextBox007EditMode)) state->TextBox007EditMode = !state->TextBox007EditMode;
        GuiLabel(state->layoutRecs[8], "Atlas Size");
        if (GuiValueBox(state->layoutRecs[10], NULL, &state->ValueBOx010Value, 0, 100, state->ValueBOx010EditMode)) state->ValueBOx010EditMode = !state->ValueBOx010EditMode;
        state->LabelButton011Pressed = GuiLabelButton(state->layoutRecs[11], "Alignment Value");
        if (GuiDropdownBox(state->layoutRecs[9], "512;1024;2048;4096;8192", &state->DropdownBox009Active, state->DropdownBox009EditMode)) state->DropdownBox009EditMode = !state->DropdownBox009EditMode;
    }
    
    GuiUnlock();
}

#endif // GUI_LAYOUT_NAME_IMPLEMENTATION
