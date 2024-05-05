#include <sys/stat.h>
#include <parson.h>

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

static JSON_Status _CreateBlankProjectFile (const char* project_file, const char* project_name);

RSP_ProjectError RSP_CreateEmptyProject (const char* project_name, RSP_Project* project) {
    if (!DirectoryExists (DEFAULT_PROJECT_DIRECTORY)) MakeDirectoy (DEFAULT_PROJECT_DIRECTORY);

    const char* project_directory = TextFormat ("%s/%s/", DEFAULT_PROJECT_DIRECTORY, project_name);
    const char* project_file = TextFormat ("%s/projects%s", project_directory, DEFAULT_PROJECT_EXTENSION);

    if (FileExists (TextFormat (project_file))) {
        TraceLog (LOG_ERROR, "Project [%s] already exists!", project_name);

        return RSP_PROJECT_ERROR_EXISTS;
    }

    MakeDirectoy (project_directory);
    MakeDirectoy (TextFormat ("%s/textures", project_directory));

    _CreateBlankProjectFile (project_file, project_name);

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

void RSP_UnloadProject (RSP_Project* project) {
    UnloadDirectoryFiles (project->files);

    for (size_t i = 0; i < project->sprite_count; i++) {
        if (project->sprites[i].animation_frame_count == 0) continue;

        MemFree (project->sprites[i].animation_frames);
    }

    MemFree (project->sprites);

    UnloadRenderTexture (project->texture_atlas);
}

JSON_Status _CreateBlankProjectFile (const char* project_file, const char* project_name) {
    JSON_Value* root = json_value_init_object ();
    JSON_Object* root_object = json_value_get_object (root);

    json_object_set_number (root_object, "version", DEFAULT_PROJECT_VERSION);
    json_object_set_string (root_object, "name", project_name);

    JSON_Value* sprites_value = json_value_init_array ();

    json_object_set_value (root_object, "sprites", sprites_value);
    json_object_dotset_number (root_object, "atlas.size", DEFAULT_PROJECT_ATLAS_SIZE);
    json_object_dotset_number (root_object, "atlas.sprite_count", 0);

    return json_serialize_to_file_pretty (root, project_file);
}
