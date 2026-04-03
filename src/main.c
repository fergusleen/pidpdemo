/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * main.c
 * Program entry point and static menu table.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "menu.h"
#include "actions.h"

static const MenuItem g_menu_items[] = {
    { '1', "Welcome / Introduction", MENU_ACTION_PAGE,
        "intro.txt", 0, 0 },
    { '2', "About the PDP-11", MENU_ACTION_PAGE,
        "pdp11.txt", 0, 0 },
    { '3', "About UNIX", MENU_ACTION_PAGE,
        "unix.txt", 0, 0 },
    { '4', "About 2.11BSD", MENU_ACTION_PAGE,
        "bsd211.txt", 0, 0 },
    { '5', "PiDP-11 Front Panel Notes", MENU_ACTION_PAGE,
        "panel.txt", 0, 0 },
    { '6', "Games On This System", MENU_ACTION_PAGE,
        "games.txt", 0, 0 },
    { '7', "Show system information", MENU_ACTION_BUILTIN,
        "sysinfo", 0, 0 },
    { '8', "Prime number / CPU timing demo", MENU_ACTION_COMMAND,
        "%SELF% --cpu-demo", "%SELF%", 0 },
    { '9', "Process or memory demo", MENU_ACTION_COMMAND,
        "%SELF% --proc-demo", "%SELF%", 0 },
    { 'A', "Directory listing / filesystem demo", MENU_ACTION_COMMAND,
        "pwd; ls -la", "ls", 0 },
    { 'B', "Adventure (if installed)", MENU_ACTION_COMMAND,
        "/usr/games/adventure", "/usr/games/adventure", 0 },
    { 'C', "Trek (if installed)", MENU_ACTION_COMMAND,
        "/usr/games/trek", "/usr/games/trek", 0 },
    { 'Q', "Quit", MENU_ACTION_EXIT, 0, 0, 0 }
};

static int is_same(const char *left, const char *right)
{
    if (strcmp(left, right) == 0) {
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    MenuContext ctx;
    const char *page_dir;
    int force_plain;
    int force_curses;
    int i;

    page_dir = "pages";
    force_plain = 0;
    force_curses = 0;

    for (i = 1; i < argc; ++i) {
        if (is_same(argv[i], "-p")) {
            force_plain = 1;
        } else if (is_same(argv[i], "-c")) {
            force_curses = 1;
        } else if (is_same(argv[i], "-d")) {
            if (i + 1 >= argc) {
                actions_usage(argv[0]);
                return 1;
            }
            page_dir = argv[++i];
        } else if (is_same(argv[i], "-h") || is_same(argv[i], "--help")) {
            actions_usage(argv[0]);
            return 0;
        } else if (is_same(argv[i], "--sysinfo")) {
            return actions_system_info();
        } else if (is_same(argv[i], "--cpu-demo")) {
            return actions_cpu_demo();
        } else if (is_same(argv[i], "--proc-demo")) {
            return actions_process_demo();
        } else {
            actions_usage(argv[0]);
            return 1;
        }
    }

    ctx.interactive = isatty(0) && isatty(1);
    ctx.use_curses = 0;
#ifdef USE_CURSES
    if (!force_plain && ctx.interactive) {
        ctx.use_curses = 1;
    }
    if (force_curses && ctx.interactive) {
        ctx.use_curses = 1;
    }
#else
    if (force_curses) {
        fprintf(stderr, "pidpdemo: curses support was not compiled in.\n");
    }
#endif
    if (force_plain) {
        ctx.use_curses = 0;
    }

    ctx.title = "PiDP-11 Demonstration System";
    ctx.page_dir = page_dir;
    ctx.self_name = argv[0];
    ctx.status[0] = '\0';

    return menu_run(g_menu_items,
        (int)(sizeof(g_menu_items) / sizeof(g_menu_items[0])), &ctx);
}
