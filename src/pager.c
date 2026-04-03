/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * pager.c
 * Text pager for page files and generated summaries.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pager.h"
#include "menu.h"

#ifdef USE_CURSES
#include <curses.h>
#endif

static int pager_ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return ch + ('a' - 'A');
    }
    return ch;
}

static int pager_env_rows(void)
{
    const char *value;
    int rows;

    value = getenv("LINES");
    if (value == 0 || value[0] == '\0') {
        return PIDPDEMO_DEFAULT_ROWS;
    }
    rows = atoi(value);
    if (rows < 10) {
        rows = PIDPDEMO_DEFAULT_ROWS;
    }
    return rows;
}

static int pager_rows(MenuContext *ctx)
{
#ifdef USE_CURSES
    if (ctx->use_curses) {
        if (LINES >= 10) {
            return LINES;
        }
        return PIDPDEMO_DEFAULT_ROWS;
    }
#endif
    return pager_env_rows();
}

static void pager_clear_plain(MenuContext *ctx)
{
    if (!ctx->interactive || ctx->use_curses) {
        return;
    }
    printf("\033[H\033[J");
    fflush(stdout);
}

static int pager_plain_prompt(int at_end, int can_back)
{
    char line[PIDPDEMO_MAX_LINE];
    int i;

    if (at_end) {
        if (can_back) {
            printf("\nEnd.  RETURN quits, B goes back: ");
        } else {
            printf("\nEnd.  RETURN quits: ");
        }
    } else if (can_back) {
        printf("\nRETURN next, B back, Q quit: ");
    } else {
        printf("\nRETURN next, Q quit: ");
    }
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == 0) {
        return 'q';
    }
    for (i = 0; line[i] != '\0'; ++i) {
        if (!isspace((unsigned char)line[i])) {
            return pager_ascii_lower((unsigned char)line[i]);
        }
    }
    return ' ';
}

static void pager_plain_header(const char *title, int page_no)
{
    printf("\n%s", title);
    if (page_no > 0) {
        printf(" (page %d)", page_no);
    }
    printf("\n");
    printf("------------------------------------------------------------\n");
}

static void pager_trim_line(const char *src, char *dst, int width)
{
    int i;

    if (width < 1) {
        width = 1;
    }
    for (i = 0; i < width - 1 && src[i] != '\0' && src[i] != '\n'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static int pager_has_more(FILE *fp, long mark)
{
    char line[PIDPDEMO_MAX_LINE];

    if (fseek(fp, mark, 0) != 0) {
        return 0;
    }
    if (fgets(line, sizeof(line), fp) == 0) {
        return 0;
    }
    return 1;
}

static int pager_show_file_plain(MenuContext *ctx, const char *title,
    FILE *fp)
{
    char line[PIDPDEMO_MAX_LINE];
    long marks[PIDPDEMO_MAX_PAGE_MARKS];
    long next_mark;
    int rows;
    int body_rows;
    int current;
    int last_mark;
    int page_no;
    int shown;
    int ch;
    int has_more;

    if (!ctx->interactive) {
        while (fgets(line, sizeof(line), fp) != 0) {
            fputs(line, stdout);
        }
        return 0;
    }

    rows = pager_rows(ctx);
    body_rows = rows - 4;
    if (body_rows < 5) {
        body_rows = 5;
    }

    marks[0] = 0L;
    current = 0;
    last_mark = 0;
    page_no = 1;

    for (;;) {
        if (fseek(fp, marks[current], 0) != 0) {
            return -1;
        }

        pager_clear_plain(ctx);
        pager_plain_header(title, page_no);
        shown = 0;
        while (shown < body_rows && fgets(line, sizeof(line), fp) != 0) {
            fputs(line, stdout);
            ++shown;
        }
        next_mark = ftell(fp);
        has_more = pager_has_more(fp, next_mark);

        ch = pager_plain_prompt(!has_more, current > 0);
        if (ch == 'q') {
            return 0;
        }
        if (ch == 'b' && current > 0) {
            --current;
            --page_no;
            continue;
        }
        if (!has_more) {
            return 0;
        }
        if (last_mark + 1 < PIDPDEMO_MAX_PAGE_MARKS && current == last_mark) {
            ++last_mark;
            marks[last_mark] = next_mark;
        }
        if (current < last_mark) {
            ++current;
            ++page_no;
        } else {
            return 0;
        }
    }
}

#ifdef USE_CURSES
static int pager_curses_prompt(int at_end, int can_back)
{
    int ch;

    move(LINES - 1, 0);
    clrtoeol();
    if (at_end) {
        if (can_back) {
            addstr("End.  RETURN quits, B goes back: ");
        } else {
            addstr("End.  RETURN quits: ");
        }
    } else if (can_back) {
        addstr("RETURN next, B back, Q quit: ");
    } else {
        addstr("RETURN next, Q quit: ");
    }
    refresh();

    ch = getch();
    if (ch == EOF) {
        return 'q';
    }
    return pager_ascii_lower((unsigned char)ch);
}

static int pager_show_file_curses(const char *title, FILE *fp)
{
    char line[PIDPDEMO_MAX_LINE];
    char shown_line[PIDPDEMO_MAX_LINE];
    long marks[PIDPDEMO_MAX_PAGE_MARKS];
    long next_mark;
    int rows;
    int body_rows;
    int width;
    int current;
    int last_mark;
    int page_no;
    int row;
    int ch;
    int has_more;

    rows = LINES;
    if (rows < 10) {
        rows = PIDPDEMO_DEFAULT_ROWS;
    }
    body_rows = rows - 4;
    if (body_rows < 5) {
        body_rows = 5;
    }
    width = COLS;
    if (width < 10) {
        width = 10;
    }

    marks[0] = 0L;
    current = 0;
    last_mark = 0;
    page_no = 1;

    for (;;) {
        if (fseek(fp, marks[current], 0) != 0) {
            return -1;
        }

        clear();
        mvaddstr(0, 0, title);
        mvprintw(1, 0, "Page %d", page_no);
        row = 3;
        while (row < 3 + body_rows && fgets(line, sizeof(line), fp) != 0) {
            pager_trim_line(line, shown_line, width);
            mvaddstr(row, 0, shown_line);
            ++row;
        }
        next_mark = ftell(fp);
        has_more = pager_has_more(fp, next_mark);
        ch = pager_curses_prompt(!has_more, current > 0);

        if (ch == 'q') {
            return 0;
        }
        if (ch == 'b' && current > 0) {
            --current;
            --page_no;
            continue;
        }
        if (!has_more) {
            return 0;
        }
        if (last_mark + 1 < PIDPDEMO_MAX_PAGE_MARKS && current == last_mark) {
            ++last_mark;
            marks[last_mark] = next_mark;
        }
        if (current < last_mark) {
            ++current;
            ++page_no;
        } else {
            return 0;
        }
    }
}
#endif

int pager_show_file(MenuContext *ctx, const char *title, const char *path)
{
    FILE *fp;
    int rc;

    fp = fopen(path, "r");
    if (fp == 0) {
        return -1;
    }

    rc = 0;
#ifdef USE_CURSES
    if (ctx->use_curses) {
        rc = pager_show_file_curses(title, fp);
    } else {
        rc = pager_show_file_plain(ctx, title, fp);
    }
#else
    rc = pager_show_file_plain(ctx, title, fp);
#endif

    fclose(fp);
    return rc;
}

int pager_show_text(MenuContext *ctx, const char *title, const char *text)
{
    char line[PIDPDEMO_MAX_LINE];
#ifdef USE_CURSES
    int row;
    int width;
    const char *p;
    int i;
    int ch;

    if (ctx->use_curses) {
        clear();
        mvaddstr(0, 0, title);
        mvaddstr(1, 0, "------------------------------------------------------------");
        row = 3;
        width = COLS;
        if (width < 10) {
            width = 10;
        }
        p = text;
        while (*p != '\0' && row < LINES - 2) {
            i = 0;
            while (*p != '\0' && *p != '\n' && i < width - 1) {
                line[i++] = *p++;
            }
            line[i] = '\0';
            mvaddstr(row++, 0, line);
            while (*p != '\0' && *p != '\n') {
                ++p;
            }
            if (*p == '\n') {
                ++p;
            }
        }
        mvaddstr(LINES - 1, 0, "Press RETURN to continue.");
        clrtoeol();
        refresh();
        for (;;) {
            ch = getch();
            if (ch == '\n' || ch == '\r' || ch == EOF) {
                break;
            }
        }
        return 0;
    }
#endif

    printf("\n%s\n", title);
    printf("------------------------------------------------------------\n");
    printf("%s\n", text);
    if (ctx->interactive) {
        printf("\nPress RETURN to continue: ");
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
    }
    return 0;
}
