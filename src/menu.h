/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * menu.h
 * Menu data structures and public entry points.
 */

#ifndef MENU_H
#define MENU_H

#include "compat.h"

typedef struct menu_item {
    char key;
    const char *label;
    int type;
    const char *target;
    const char *probe;
    const char *note;
} MenuItem;

typedef struct menu_context {
    int interactive;
    int use_curses;
    const char *title;
    const char *page_dir;
    const char *self_name;
    char status[PIDPDEMO_MAX_STATUS];
} MenuContext;

enum {
    MENU_ACTION_PAGE = 1,
    MENU_ACTION_COMMAND,
    MENU_ACTION_BUILTIN,
    MENU_ACTION_EXIT
};

int menu_run(const MenuItem *items, int count, MenuContext *ctx);

#endif
