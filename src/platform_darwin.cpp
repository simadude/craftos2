/*
 * platform_darwin.cpp
 * CraftOS-PC 2
 * 
 * This file implements functions specific to macOS.
 * 
 * This code is licensed under the MIT license.
 * Copyright (c) 2019 JackMacWindows.
 */

#ifdef __APPLE__ // disable error checking on Windows
extern "C" {
#include <lua.h>
}
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <libgen.h>
#include <pthread.h>
#include <glob.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <sstream>
#include "mounter.hpp"
#include "platform.hpp"

#ifdef CUSTOM_ROM_DIR
const char * rom_path = CUSTOM_ROM_DIR;
char * rom_path_expanded = NULL;
#else
const char * rom_path = "/usr/local/share/craftos";
#endif
const char * base_path = "$HOME/.craftos";
char * base_path_expanded = NULL;

void platformFree() {
    if (base_path_expanded != NULL) free(base_path_expanded);
    #ifdef CUSTOM_ROM_DIR
    if (rom_path_expanded != NULL) free(rom_path_expanded);
    #endif
}

const char * getBasePath() {
    if (base_path_expanded != NULL) return base_path_expanded;
    wordexp_t p;
    wordexp(base_path, &p, 0);
    int size = 0;
    for (int i = 0; i < p.we_wordc; i++) size += strlen(p.we_wordv[i]);
    base_path_expanded = (char*)malloc(size + 1);
    strcpy(base_path_expanded, p.we_wordv[0]);
    for (int i = 1; i < p.we_wordc; i++) strcat(base_path_expanded, p.we_wordv[i]);
    wordfree(&p);
    return base_path_expanded;
}

#ifdef CUSTOM_ROM_DIR
void platformInit(Computer *comp) {
    addMount(comp, (std::string(getROMPath()) + "/rom").c_str(), "rom", config.rootReadOnly);
}

const char * getROMPath() {
    if (rom_path_expanded != NULL) return rom_path_expanded;
    wordexp_t p;
    wordexp(rom_path, &p, 0);
    int size = 0;
    for (int i = 0; i < p.we_wordc; i++) size += strlen(p.we_wordv[i]);
    char * retval = (char*)malloc(size + 1);
    strcpy(retval, p.we_wordv[0]);
    for (int i = 1; i < p.we_wordc; i++) strcat(retval, p.we_wordv[i]);
    rom_path_expanded = retval;
    wordfree(&p);
    return retval;
}

char * getBIOSPath() {
    const char * romPath = getROMPath();
    char * retval = (char*)malloc(strlen(romPath) + 10);
    strcpy(retval, romPath);
    strcat(retval, "/bios.lua");
    return retval;
}

std::string getPlugInPath() { return std::string(getROMPath()) + "/plugins/"; }
#else
void platformInit(Computer *comp) {
    addMount(comp, (std::string(rom_path) + "/rom").c_str(), "rom", config.rootReadOnly);
}

const char * getROMPath() { return rom_path; }

char * getBIOSPath() {
    char * retval = (char*)malloc(strlen(rom_path) + 10);
    strcpy(retval, rom_path);
    strcat(retval, "/bios.lua");
    return retval;
}

std::string getPlugInPath() { return std::string(rom_path) + "/plugins/"; }
#endif

void setThreadName(std::thread &t, const char * name) {}

int createDirectory(const char * path) {
    if (mkdir(path, 0777) != 0) {
        if (errno == ENOENT && strcmp(path, "/") != 0) {
            char * dir = (char*)malloc(strlen(path) + 1);
            strcpy(dir, path);
            if (createDirectory(dirname(dir))) {free(dir); return 1;}
            free(dir);
            mkdir(path, 0777);
        } else return 1;
    }
    return 0;
}

int removeDirectory(char *path) {
    struct stat statbuf;
    if (!stat(path, &statbuf)) {
        if (S_ISDIR(statbuf.st_mode)) {
            DIR *d = opendir(path);
            size_t path_len = strlen(path);
            int r = -1;
            if (d) {
                struct dirent *p;
                r = 0;
                while (!r && (p=readdir(d))) {
                    int r2 = -1;
                    char *buf;
                    size_t len;

                    /* Skip the names "." and ".." as we don't want to recurse on them. */
                    if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) continue;
                    len = path_len + strlen(p->d_name) + 2; 
                    buf = (char*)malloc(len);
                    if (buf) {
                        snprintf(buf, len, "%s/%s", path, p->d_name);
                        r2 = removeDirectory(buf);
                        free(buf);
                    }
                    r = r2;
                }
                closedir(d);
            }
            if (!r) r = rmdir(path);
            return r;
        } else return unlink(path);
    } else return -1;
}

unsigned long long getFreeSpace(char* path) {
	struct statvfs st;
	if (statvfs(path, &st) != 0) return 0;
	return st.f_bavail * st.f_bsize;
}

#if defined(__i386__) || defined(__i386) || defined(i386)
#define ARCHITECTURE "i386"
#elif defined(__amd64__) || defined(__amd64)
#define ARCHITECTURE "amd64"
#elif defined(__x86_64__) || defined(__x86_64)
#define ARCHITECTURE "x86_64"
#elif defined(__arm__) || defined(__arm)
#define ARCHITECTURE "armv7"
#elif defined(__arm64__) || defined(__arm64)
#define ARCHITECTURE "arm64"
#elif defined(__aarch64__) || defined(__aarch64)
#define ARCHITECTURE "aarch64"
#elif defined(__powerpc__) || defined(__powerpc) || defined(__ppc__)
#define ARCHITECTURE "powerpc"
#elif defined(__powerpc64__) || defined(__ppc64)
#define ARCHITECTURE "ppc64"
#else
#define ARCHITECTURE "unknown"
#endif

void pushHostString(lua_State *L) {
    struct utsname host;
    uname(&host);
    lua_pushfstring(L, "%s %s %s", host.sysname, ARCHITECTURE, host.release);
}

#endif // __INTELLISENSE__