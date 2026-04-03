/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * compat.h
 * Small portability constants shared across the program.
 */

#ifndef COMPAT_H
#define COMPAT_H

#define PIDPDEMO_MAX_LINE 256
#define PIDPDEMO_MAX_PATH 256
#define PIDPDEMO_MAX_COMMAND 512
#define PIDPDEMO_MAX_STATUS 128
#define PIDPDEMO_MAX_PAGE_MARKS 128
#define PIDPDEMO_DEFAULT_ROWS 24

#ifndef X_OK
#define X_OK 1
#endif

#endif
