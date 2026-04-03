/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * menu.c
 * Plain-text and optional curses menu handling.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "actions.h"

#ifdef USE_CURSES
#include <curses.h>
#endif

static int menu_ascii_upper(int ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return ch - ('a' - 'A');
    }
    return ch;
}

static const MenuItem *menu_find(const MenuItem *items, int count, int ch)
{
    int i;
    int want;

    want = menu_ascii_upper(ch);
    for (i = 0; i < count; ++i) {
        if (menu_ascii_upper(items[i].key) == want) {
            return &items[i];
        }
    }
    return 0;
}

static void menu_clear_plain(MenuContext *ctx)
{
    if (!ctx->interactive || ctx->use_curses) {
        return;
    }
    printf("\033[H\033[J");
    fflush(stdout);
}

static void menu_set_status(MenuContext *ctx, const char *message)
{
    int i;

    if (message == 0) {
        ctx->status[0] = '\0';
        return;
    }

    for (i = 0; i < PIDPDEMO_MAX_STATUS - 1 && message[i] != '\0'; ++i) {
        ctx->status[i] = message[i];
    }
    ctx->status[i] = '\0';
}

static int menu_read_selection(FILE *fp)
{
    char line[PIDPDEMO_MAX_LINE];
    int i;

    if (fgets(line, sizeof(line), fp) == 0) {
        return EOF;
    }
    for (i = 0; line[i] != '\0'; ++i) {
        if (!isspace((unsigned char)line[i])) {
            return menu_ascii_upper((unsigned char)line[i]);
        }
    }
    return '\0';
}

static int menu_run_plain(const MenuItem *items, int count, MenuContext *ctx)
{
    const MenuItem *item;
    int ch;
    int i;

    for (;;) {
        menu_clear_plain(ctx);
        printf("\n%s\n", ctx->title);
        printf("------------------------------------------------------------\n");
        for (i = 0; i < count; ++i) {
            printf(" %c. %s\n", items[i].key, items[i].label);
        }
        if (ctx->status[0] != '\0') {
            printf("\n%s\n", ctx->status);
        }
        printf("\nSelection: ");
        fflush(stdout);

        ch = menu_read_selection(stdin);
        if (ch == EOF) {
            putchar('\n');
            return 0;
        }
        if (ch == '\0') {
            menu_set_status(ctx, "Please enter a menu selection.");
            continue;
        }

        item = menu_find(items, count, ch);
        if (item == 0) {
            menu_set_status(ctx, "Unknown selection.");
            continue;
        }
        if (item->type == MENU_ACTION_EXIT) {
            return 0;
        }

        menu_set_status(ctx, 0);
        actions_execute(item, ctx);
    }
}

#ifdef USE_CURSES
static void menu_draw_curses(const MenuItem *items, int count, MenuContext *ctx)
{
    int i;
    int row;

    clear();
    mvaddstr(0, 0, ctx->title);
    mvaddstr(1, 0, "------------------------------------------------------------");

    row = 3;
    for (i = 0; i < count; ++i) {
        mvprintw(row, 0, " %c. %s", items[i].key, items[i].label);
        ++row;
    }

    if (ctx->status[0] != '\0') {
        mvaddstr(LINES - 3, 0, ctx->status);
    }
    mvaddstr(LINES - 1, 0, "Selection: ");
    clrtoeol();
    move(LINES - 1, 11);
    refresh();
}

static int menu_run_curses(const MenuItem *items, int count, MenuContext *ctx)
{
    const MenuItem *item;
    int ch;

    for (;;) {
        menu_draw_curses(items, count, ctx);
        ch = getch();
        if (ch == EOF) {
            return 0;
        }
        if (ch == '\n' || ch == '\r' || ch == ' ') {
            menu_set_status(ctx, "Please enter a menu selection.");
            continue;
        }

        item = menu_find(items, count, ch);
        if (item == 0) {
            menu_set_status(ctx, "Unknown selection.");
            continue;
        }
        if (item->type == MENU_ACTION_EXIT) {
            return 0;
        }

        menu_set_status(ctx, 0);
        actions_execute(item, ctx);
    }
}

static int menu_try_curses(const MenuItem *items, int count, MenuContext *ctx)
{
    const char *term;

    term = getenv("TERM");
    if (!ctx->interactive || term == 0 || term[0] == '\0') {
        return -1;
    }
    if (initscr() == 0) {
        return -1;
    }
    cbreak();
    noecho();
    keypad(stdscr, 1);

    menu_run_curses(items, count, ctx);
    endwin();
    return 0;
}
#endif

int menu_run(const MenuItem *items, int count, MenuContext *ctx)
{
#ifdef USE_CURSES
    if (ctx->use_curses) {
        if (menu_try_curses(items, count, ctx) == 0) {
            return 0;
        }
        ctx->use_curses = 0;
    }
#endif
    return menu_run_plain(items, count, ctx);
}
