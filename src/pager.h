/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * pager.h
 * Pager interfaces used by menu actions.
 */

#ifndef PAGER_H
#define PAGER_H

struct menu_context;

int pager_show_file(struct menu_context *ctx, const char *title,
    const char *path);
int pager_show_text(struct menu_context *ctx, const char *title,
    const char *text);

#endif
