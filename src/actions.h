/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * actions.h
 * Public action and demo interfaces.
 */

#ifndef ACTIONS_H
#define ACTIONS_H

struct menu_item;
struct menu_context;

int actions_execute(const struct menu_item *item, struct menu_context *ctx);
int actions_system_info(void);
int actions_show_system_info(struct menu_context *ctx);
int actions_cpu_demo(void);
int actions_process_demo(void);
void actions_usage(const char *progname);

#endif
