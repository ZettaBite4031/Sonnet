#define NOB_IMPLEMENTATION
#include "nob.h"


int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    
    const char* compiler = "g++";
    Nob_Cmd cmd = {0};
    
    nob_mkdir_if_not_exists("build");

    const char* cxxflags[] = {
        "-std=c++26",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Iinclude",
        "-ggdb",
        "-fPIC",
        NULL
    };

    const char* lib_srcs[] = {
        "src/error.cpp",
        "src/sonnet.cpp",
        "src/value.cpp",
        NULL
    };


    Nob_File_Paths objs = {0};

    for (int i = 0; lib_srcs[i]; i++) {
        const char* src = lib_srcs[i];
        Nob_String_Builder sb = {0};
        nob_sb_append_cstr(&sb, "build/");
        nob_sb_append_cstr(&sb, nob_path_name(src));
        nob_sb_append_cstr(&sb, ".o");
        Nob_String_View sv = nob_sb_to_sv(sb);
        const char* obj = nob_temp_sv_to_cstr(sv);

        nob_log(NOB_INFO, "Compiling %s -> %s", src, obj);

        nob_cmd_append(&cmd, compiler);
        nob_cmd_append(&cmd, "-c", src);
        for (int j = 0; cxxflags[j]; j++) {
            nob_cmd_append(&cmd, cxxflags[j]);
        }
        nob_cmd_append(&cmd, "-o", obj);

        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Failed to compile %s", src);
            return 1;
        }

        nob_da_append(&objs, obj);
    }

    const char* examples[] = {
        "examples/json.cpp",
        NULL
    };
    const char* example_exes[] = {
        "build/json",
        NULL
    };

    for (int i = 0; examples[i]; i++) {

        nob_cmd_append(&cmd, compiler);
        nob_cmd_append(&cmd, "-o", example_exes[i]);
        nob_cmd_append(&cmd, examples[i]);
        for (int i = 0; cxxflags[i]; i++) {
            nob_cmd_append(&cmd, cxxflags[i]);
        }
        
        for (size_t i = 0; i < objs.count; i++) {
            nob_cmd_append(&cmd, objs.items[i]);
        }
        
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Failed to link %s", example_exes[i]);
            return 1;
        }
        
        nob_log(NOB_INFO, "Built %s", example_exes[i]);
    }

    nob_cmd_append(&cmd, "ar", "rcs", "build/libsonnet.a");
    for (int i = 0; i < objs.count; i++) {
        nob_cmd_append(&cmd, objs.items[i]);
    }
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Failed to create library 'libsonnet.a'");
        return 1;
    }

    nob_cmd_append(&cmd, "gcc", "-shared", "-o", "build/libsonnet.so");
    for (int i = 0; i < objs.count; i++) {
        nob_cmd_append(&cmd, objs.items[i]);
    }
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Failed to create shared library 'libsonnet.so'");
        return 1;
    }

    return 0;
}