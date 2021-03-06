#include <assert.h>

#include "../main.h"

#ifdef __OBJC__
#include "../cocoa/main.h"
#else
#include "../xlib/main.h"
#endif

bool native_create_dir(const uint8_t *filepath) {
    const int status = mkdir(filepath, S_IRWXU);
    if (status == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

// TODO: DRY. This function exists in both posix/filesys.c and in android/main.c
static void opts_to_sysmode(UTOX_FILE_OPTS opts, char *mode) {
    if (opts & UTOX_FILE_OPTS_READ) {
        mode[0] = 'r';
    }

    if (opts & UTOX_FILE_OPTS_APPEND) {
        mode[0] = 'a';
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        mode[0] = 'w';
    }

    mode[1] = 'b';

    if ((opts & (UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_APPEND)) && (opts & UTOX_FILE_OPTS_READ)) {
        mode[2] = '+';
    }

    mode[3] = 0;

    return;
}

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts) {
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    // native_get_file should never be called with DELETE in combination with other FILE_OPTS.
    assert(opts <= UTOX_FILE_OPTS_DELETE);
    // WRITE and APPEND are mutually exclusive. WRITE will serve you a blank file. APPEND will append (duh).
    assert((opts & UTOX_FILE_OPTS_WRITE && opts & UTOX_FILE_OPTS_APPEND) == false);

    if (opts & UTOX_FILE_OPTS_READ || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir(path)) {
            return NULL;
        }
    }

    if (strlen((char *)path) + strlen((char *)name) >= UTOX_FILE_NAME_LENGTH) {
        debug("NATIVE:\tLoad directory name too long\n");
        return NULL;
    } else {
        snprintf((char *)path + strlen((char *)path), UTOX_FILE_NAME_LENGTH - strlen((char *)path), "%s", name);
    }

    if (opts == UTOX_FILE_OPTS_DELETE) {
        remove(path);
        return NULL;
    }

    char mode[4] = { 0 };
    opts_to_sysmode(opts, mode);

    FILE *fp = fopen(path, mode);

    if (fp == NULL) {
        debug_notice("NATIVE:\tCould not open %s\n", path);
        return NULL;
    }

    if (size != NULL) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}
