#include <sys/stat.h>
#include <parson.h>
#include <stdio.h>
#include <string.h>

#include "project.h"

#ifdef _WIN32
#   define MakeDirectory(x) _mkdir(x)
#else
#   define MakeDirectoy(x) mkdir(x, 0777)
#endif

#define DEFAULT_PROJECT_DIRECTORY "projects"
#define DEFAULT_PROJECT_EXTENSION ".rspp"
#define DEFAULT_PROJECT_ATLAS_SIZE 1024
#define DEFAULT_PROJECT_VERSION 1

static JSON_Status _SaveProjectFile (const char* project_file, RSP_Project* project);

void CopyFile (const char* source, const char* destination);

RSP_ProjectError RSP_CreateEmptyProject (const char* project_name, RSP_Project* project) {
    if (!DirectoryExists (DEFAULT_PROJECT_DIRECTORY)) MakeDirectoy (DEFAULT_PROJECT_DIRECTORY);

    if (!project) {
        TraceLog (LOG_ERROR, "Provided project is not valid!");

        return RSP_PROJECT_ERROR_INVALID;
    }

    const char* project_directory = TextFormat ("%s/%s/", DEFAULT_PROJECT_DIRECTORY, project_name);
    const char* project_file = TextFormat ("%s/projects%s", project_directory, DEFAULT_PROJECT_EXTENSION);

    if (FileExists (TextFormat (project_file))) {
        TraceLog (LOG_ERROR, "Project [%s] already exists!", project_name);

        return RSP_PROJECT_ERROR_EXISTS;
    }

    MakeDirectoy (project_directory);
    MakeDirectoy (TextFormat ("%s/textures", project_directory));

    {
        project->version = DEFAULT_PROJECT_VERSION;
        strncpy (project->name, project_name, MAX_PROJECT_NAME_LENGTH);

        project->sprites = NULL;

        project->atlas.size = DEFAULT_PROJECT_ATLAS_SIZE;
        project->atlas.sprite_count = 0;
        project->atlas.texture = LoadRenderTexture (DEFAULT_PROJECT_ATLAS_SIZE, DEFAULT_PROJECT_ATLAS_SIZE);
    }

    _SaveProjectFile (project_file, project);

    return RSP_PROJECT_ERROR_NONE;
}

RSP_ProjectError RSP_LoadProject (const char* project_name, RSP_Project* project) {
    const char* project_directory = TextFormat ("%s/%s/", DEFAULT_PROJECT_DIRECTORY, project_name);
    const char* project_file = TextFormat ("%s/projects%s", project_directory, DEFAULT_PROJECT_EXTENSION);

    if (!FileExists (project_file)) {
        TraceLog (LOG_ERROR, "Project [%s] does not exist!");

        return RSP_PROJECT_ERROR_NOT_EXIST;
    }

    return RSP_PROJECT_ERROR_NONE;
}

RSP_ProjectError RSP_SaveProject (RSP_Project* project) {
    const char* project_directory = TextFormat ("%s/%s/", DEFAULT_PROJECT_DIRECTORY, project->name);
    const char* project_file = TextFormat ("%s/projects%s", project_directory, DEFAULT_PROJECT_EXTENSION);

    if (!FileExists (project_file)) {
        TraceLog (LOG_ERROR, "Project [%s] does not exist!");

        return RSP_PROJECT_ERROR_NOT_EXIST;
    }

    const char* backup_file = TextFormat ("%s.bkp", project_file);
    if (FileExists (backup_file)) remove (backup_file);

    CopyFile (project_file, backup_file);

    remove (project_file);

    _SaveProjectFile (project_file, project);

    return RSP_PROJECT_ERROR_NONE;
}

void RSP_UnloadProject (RSP_Project* project) {
    UnloadDirectoryFiles (project->files);

    for (size_t i = 0; i < project->atlas.sprite_count; i++) {
        if (project->sprites[i].animation_frame_count == 0) continue;

        MemFree (project->sprites[i].animation_frames);
    }

    MemFree (project->sprites);

    UnloadRenderTexture (project->atlas.texture);
}

static JSON_Status _SaveProjectFile (const char* project_file, RSP_Project* project) {
    JSON_Value* root = json_value_init_object ();
    JSON_Object* root_object = json_value_get_object (root);

    json_object_set_number (root_object, "version", project->version);
    json_object_set_string (root_object, "name", project->name);

    JSON_Value* sprites_value = json_value_init_array ();
    JSON_Array* sprits_array = json_value_get_array (sprites_value);

    for (size_t i = 0; i < project->atlas.sprite_count; i++) {
        RSP_Sprite* current_sprite = &project->sprites[i];

        JSON_Value* sprite_value = json_value_init_object ();
        JSON_Object* sprite_object = json_value_get_object (sprite_value);

        // Generic data
        json_object_set_number (sprite_object, "hash", current_sprite->hash);
        json_object_set_string (sprite_object, "name", current_sprite->name);

        // Sprite source
        json_object_dotset_number (sprite_object, "source.x", current_sprite->source.x);
        json_object_dotset_number (sprite_object, "source.y", current_sprite->source.y);
        json_object_dotset_number (sprite_object, "source.width", current_sprite->source.width);
        json_object_dotset_number (sprite_object, "source.height", current_sprite->source.height);

        // Origin
        json_object_dotset_number (sprite_object, "origin.x", current_sprite->origin.x);
        json_object_dotset_number (sprite_object, "origin.y", current_sprite->origin.y);

        json_array_append_value (sprits_array, sprite_value);
    }

    json_object_set_value (root_object, "sprites", sprites_value);
    json_object_dotset_number (root_object, "atlas.size", project->atlas.size);
    json_object_dotset_number (root_object, "atlas.sprite_count", project->atlas.sprite_count);

    return json_serialize_to_file_pretty (root, project_file);
}
