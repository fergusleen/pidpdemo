/*
 * pidpdemo - PiDP-11 / 2.11BSD demonstration menu
 * Repository: https://github.com/fergusleen/pidp11infosuite
 *
 * actions.c
 * Menu actions, demo entry points, and command dispatch.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "actions.h"
#include "menu.h"
#include "pager.h"

#ifdef USE_CURSES
#include <curses.h>
#endif

#if defined(O_NONBLOCK)
#define PIDPDEMO_NONBLOCK_FLAG O_NONBLOCK
#elif defined(O_NDELAY)
#define PIDPDEMO_NONBLOCK_FLAG O_NDELAY
#elif defined(FNDELAY)
#define PIDPDEMO_NONBLOCK_FLAG FNDELAY
#else
#define PIDPDEMO_NONBLOCK_FLAG 0
#endif

static int copy_string(char *dst, int size, const char *src)
{
    int i;

    if (size < 1) {
        return -1;
    }
    for (i = 0; i < size - 1 && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
    if (src[i] != '\0') {
        return -1;
    }
    return 0;
}

static int append_string(char *dst, int size, const char *src)
{
    int used;
    int i;

    used = (int)strlen(dst);
    if (used >= size) {
        return -1;
    }
    for (i = 0; used + i < size - 1 && src[i] != '\0'; ++i) {
        dst[used + i] = src[i];
    }
    dst[used + i] = '\0';
    if (src[i] != '\0') {
        return -1;
    }
    return 0;
}

static int expand_self_command(const char *src, const char *self_name,
    char *dst, int size)
{
    const char *mark;
    int prefix_len;

    mark = strstr(src, "%SELF%");
    if (mark == 0) {
        return copy_string(dst, size, src);
    }

    prefix_len = (int)(mark - src);
    if (prefix_len >= size) {
        return -1;
    }
    memcpy(dst, src, (size_t)prefix_len);
    dst[prefix_len] = '\0';
    if (append_string(dst, size, self_name) != 0) {
        return -1;
    }
    if (append_string(dst, size, mark + 6) != 0) {
        return -1;
    }
    return 0;
}

static int search_path(const char *name)
{
    const char *path;
    const char *p;
    char dir[PIDPDEMO_MAX_PATH];
    char full[PIDPDEMO_MAX_PATH];
    int i;
    int j;

    path = getenv("PATH");
    if (path == 0 || path[0] == '\0') {
        path = "/bin:/usr/bin:/usr/ucb:/usr/games";
    }

    p = path;
    while (*p != '\0') {
        i = 0;
        while (*p != '\0' && *p != ':' && i < PIDPDEMO_MAX_PATH - 1) {
            dir[i++] = *p++;
        }
        dir[i] = '\0';
        if (*p == ':') {
            ++p;
        }
        if (dir[0] == '\0') {
            if (copy_string(full, sizeof(full), name) != 0) {
                continue;
            }
        } else {
            if (copy_string(full, sizeof(full), dir) != 0) {
                continue;
            }
            if (append_string(full, sizeof(full), "/") != 0) {
                continue;
            }
            if (append_string(full, sizeof(full), name) != 0) {
                continue;
            }
        }
        j = access(full, X_OK);
        if (j == 0) {
            return 1;
        }
    }
    return 0;
}

static int command_available(const char *probe)
{
    if (probe == 0 || probe[0] == '\0') {
        return 1;
    }
    if (strcmp(probe, "%SELF%") == 0) {
        return 1;
    }
    if (strchr(probe, '/') != 0) {
        if (access(probe, X_OK) == 0) {
            return 1;
        }
        return 0;
    }
    return search_path(probe);
}

static void actions_clear_plain(MenuContext *ctx)
{
    if (!ctx->interactive || ctx->use_curses) {
        return;
    }
    printf("\033[H\033[J");
    fflush(stdout);
}

static void prompt_continue(MenuContext *ctx)
{
    char line[PIDPDEMO_MAX_LINE];

    if (!ctx->interactive) {
        return;
    }
    printf("\nPress RETURN to continue: ");
    fflush(stdout);
    fgets(line, sizeof(line), stdin);
    actions_clear_plain(ctx);
}

static int run_command(const MenuItem *item, MenuContext *ctx)
{
    char command[PIDPDEMO_MAX_COMMAND];
    char page[PIDPDEMO_MAX_PATH];
    int status;

    if (!command_available(item->probe)) {
        if (copy_string(page, sizeof(page),
            "This item is not available on this system.\n") != 0) {
            return -1;
        }
        if (item->probe != 0 && item->probe[0] != '\0') {
            if (append_string(page, sizeof(page), "Missing command: ") != 0) {
                return -1;
            }
            if (append_string(page, sizeof(page), item->probe) != 0) {
                return -1;
            }
            if (append_string(page, sizeof(page), "\n") != 0) {
                return -1;
            }
        }
        return pager_show_text(ctx, item->label, page);
    }

    if (expand_self_command(item->target, ctx->self_name,
        command, sizeof(command)) != 0) {
        return pager_show_text(ctx, item->label,
            "Command line is too long to run safely.");
    }

#ifdef USE_CURSES
    if (ctx->use_curses) {
        def_prog_mode();
        endwin();
    }
#endif

    printf("\n[%s]\n\n", item->label);
    fflush(stdout);
    status = system(command);
    if (status == -1) {
        printf("Command could not be started: %s\n", strerror(errno));
    } else if (status != 0) {
        printf("\nCommand returned status %d.\n", status);
    }
    prompt_continue(ctx);

#ifdef USE_CURSES
    if (ctx->use_curses) {
        reset_prog_mode();
        clear();
        refresh();
    }
#endif

    return 0;
}

static int show_page(const MenuItem *item, MenuContext *ctx)
{
    char path[PIDPDEMO_MAX_PATH];

    if (copy_string(path, sizeof(path), ctx->page_dir) != 0 ||
        append_string(path, sizeof(path), "/") != 0 ||
        append_string(path, sizeof(path), item->target) != 0) {
        return pager_show_text(ctx, item->label,
            "Page path is too long.");
    }

    if (pager_show_file(ctx, item->label, path) != 0) {
        return pager_show_text(ctx, item->label,
            "The requested page file could not be opened.");
    }
    return 0;
}

static int build_sysinfo_command(char *command, int size, const char *outfile)
{
    if (copy_string(command, size, "sh ./scripts/sysinfo.sh") != 0) {
        return -1;
    }
    if (outfile != 0) {
        if (append_string(command, size, " > ") != 0 ||
            append_string(command, size, outfile) != 0 ||
            append_string(command, size, " 2>&1") != 0) {
            return -1;
        }
    }
    return 0;
}

int actions_system_info(void)
{
    char command[PIDPDEMO_MAX_COMMAND];

    if (build_sysinfo_command(command, sizeof(command), 0) != 0) {
        fprintf(stderr, "pidpdemo: system summary command is too long.\n");
        return 1;
    }
    if (system(command) != 0) {
        fprintf(stderr, "pidpdemo: could not run scripts/sysinfo.sh.\n");
        return 1;
    }
    return 0;
}

int actions_show_system_info(MenuContext *ctx)
{
    char command[PIDPDEMO_MAX_COMMAND];
    char path[PIDPDEMO_MAX_PATH];
    int rc;

    sprintf(path, "/tmp/pidpdemo.%ld", (long)getpid());
    if (build_sysinfo_command(command, sizeof(command), path) != 0) {
        return pager_show_text(ctx, "System Summary",
            "The system summary command is too long.");
    }
    if (system(command) != 0) {
        unlink(path);
        return pager_show_text(ctx, "System Summary",
            "The system summary script could not be run.");
    }
    rc = pager_show_file(ctx, "System Summary", path);
    unlink(path);
    if (rc != 0) {
        return pager_show_text(ctx, "System Summary",
            "The generated system summary could not be opened.");
    }
    return 0;
}

int actions_execute(const MenuItem *item, MenuContext *ctx)
{
    if (item->type == MENU_ACTION_PAGE) {
        return show_page(item, ctx);
    }
    if (item->type == MENU_ACTION_BUILTIN) {
        if (strcmp(item->target, "sysinfo") == 0) {
            return actions_show_system_info(ctx);
        }
        return pager_show_text(ctx, item->label,
            "This built-in item is not implemented.");
    }
    if (item->type == MENU_ACTION_COMMAND) {
        return run_command(item, ctx);
    }
    return 0;
}

#define PIDPDEMO_PI_DECIMALS 2000
#define PIDPDEMO_PI_GROUPS (((PIDPDEMO_PI_DECIMALS + 1) + 3) / 4)
#define PIDPDEMO_PI_WORK ((PIDPDEMO_PI_GROUPS * 14) + 1)

static void cpu_demo_wait_for_return(void)
{
    char line[PIDPDEMO_MAX_LINE];

    if (!isatty(0)) {
        return;
    }
    printf("\nPress RETURN to begin a pi calculation to %d decimal places: ",
        PIDPDEMO_PI_DECIMALS);
    fflush(stdout);
    fgets(line, sizeof(line), stdin);
    printf("\n");
}

static int cpu_demo_begin_key_watch(int *saved_flags)
{
    int flags;

    *saved_flags = 0;
    if (!isatty(0)) {
        return 0;
    }

    flags = fcntl(0, F_GETFL, 0);
    if (flags < 0) {
        flags = 0;
    }
    *saved_flags = flags;

    if (system("stty raw -echo >/dev/null 2>&1") != 0) {
        return -1;
    }
#if PIDPDEMO_NONBLOCK_FLAG != 0
    if (fcntl(0, F_SETFL, flags | PIDPDEMO_NONBLOCK_FLAG) < 0) {
        system("stty -raw echo >/dev/null 2>&1");
        return -1;
    }
#endif
    return 1;
}

static void cpu_demo_end_key_watch(int key_mode, int saved_flags)
{
    if (key_mode <= 0) {
        return;
    }
#if PIDPDEMO_NONBLOCK_FLAG != 0
    fcntl(0, F_SETFL, saved_flags);
#else
    saved_flags = saved_flags;
#endif
    system("stty -raw echo >/dev/null 2>&1");
}

static int cpu_demo_stop_requested(int key_mode)
{
    char ch;

    if (key_mode <= 0) {
        return 0;
    }
    if (read(0, &ch, 1) == 1) {
        return 1;
    }
    return 0;
}

static int cpu_demo_stream_pi(void)
{
    static long work[PIDPDEMO_PI_WORK];
    long d;
    long e;
    long group_value;
    char group[8];
    int total_digits;
    int groups;
    int c;
    int printed_digits;
    int decimals_printed;
    int i;
    int b;
    int key_mode;
    int saved_flags;

    total_digits = PIDPDEMO_PI_DECIMALS + 1;
    groups = PIDPDEMO_PI_GROUPS;
    c = groups * 14;
    if (c + 1 > PIDPDEMO_PI_WORK) {
        return -1;
    }

    for (i = 1; i <= c; ++i) {
        work[i] = 2000L;
    }

    printed_digits = 0;
    decimals_printed = 0;
    e = 0L;
    key_mode = cpu_demo_begin_key_watch(&saved_flags);

    printf("Pi stream begins.  Press any key to stop.\n");

    while (c > 0 && printed_digits < total_digits) {
        d = 0L;
        for (b = c; b > 0; --b) {
            d += work[b] * 10000L;
            work[b] = d % (long)(2 * b - 1);
            d /= (long)(2 * b - 1);
            if (b > 1) {
                d *= (long)(b - 1);
            }
        }
        group_value = e + (d / 10000L);
        sprintf(group, "%04ld", group_value);
        for (i = 0; i < 4 && printed_digits < total_digits; ++i) {
            if (printed_digits == 0) {
                putchar(group[i]);
                putchar('.');
            } else {
                putchar(group[i]);
                ++decimals_printed;
                if ((decimals_printed % 50) == 0) {
                    putchar('\n');
                    if (decimals_printed != PIDPDEMO_PI_DECIMALS) {
                        printf("  ");
                    }
                } else if ((decimals_printed % 10) == 0) {
                    putchar(' ');
                }
            }
            ++printed_digits;
            if (cpu_demo_stop_requested(key_mode)) {
                cpu_demo_end_key_watch(key_mode, saved_flags);
                printf("\n\nPi stream stopped by keypress.\n");
                return 0;
            }
        }
        fflush(stdout);
        e = d % 10000L;
        c -= 14;
    }

    cpu_demo_end_key_watch(key_mode, saved_flags);
    printf("\n\nPi stream reached %d decimal places.\n",
        PIDPDEMO_PI_DECIMALS);
    return 0;
}

int actions_cpu_demo(void)
{
    long a;
    long b;
    long next;
    long sum;
    long candidate;
    long divisor;
    long limit;
    long prime_count;
    long largest_prime;
    int is_prime;
    int i;
    time_t started;
    time_t finished;
    long elapsed;

    printf("Prime number / CPU timing demonstration\n");
    printf("---------------------------------------\n");
    printf("Small integer work was a common quick test on modest systems.\n");
    printf("\n");

    a = 1;
    b = 1;
    printf("First twelve Fibonacci values:\n");
    for (i = 1; i <= 12; ++i) {
        printf("%2d: %ld\n", i, a);
        next = a + b;
        a = b;
        b = next;
    }

    printf("\n");
    sum = 0;
    for (i = 1; i <= 1000; ++i) {
        sum += (long)i;
    }
    printf("Sum 1..1000 = %ld\n", sum);
    printf("Product 1234 * 37 = %ld\n", 1234L * 37L);
    limit = 20000L;
    prime_count = 0L;
    largest_prime = 0L;
    started = time((time_t *)0);
    printf("Prime search now begins.\n");
    printf("One dot marks ten primes found.\n");
    printf("Upper limit: %ld\n\n", limit);
    fflush(stdout);

    for (candidate = 2L; candidate <= limit; ++candidate) {
        if (candidate == 2L) {
            is_prime = 1;
        } else if ((candidate % 2L) == 0) {
            is_prime = 0;
        } else {
            is_prime = 1;
            for (divisor = 3L; divisor * divisor <= candidate; divisor += 2L) {
                if ((candidate % divisor) == 0) {
                    is_prime = 0;
                    break;
                }
            }
        }
        if (is_prime) {
            ++prime_count;
            largest_prime = candidate;
            if ((prime_count % 10L) == 0L) {
                putchar('.');
                fflush(stdout);
            }
            if ((prime_count % 500L) == 0L) {
                printf(" %ld\n", prime_count);
                fflush(stdout);
            }
        }
    }

    finished = time((time_t *)0);
    elapsed = (long)(finished - started);
    printf("\n\n");
    printf("Prime search up to %ld completed.\n", limit);
    printf("Primes found: %ld\n", prime_count);
    printf("Largest prime: %ld\n", largest_prime);
    if (elapsed <= 0L) {
        printf("Elapsed time: less than one second\n");
    } else {
        printf("Elapsed time: %ld second", elapsed);
        if (elapsed != 1L) {
            printf("s");
        }
        printf("\n");
    }

    cpu_demo_wait_for_return();

    if (cpu_demo_stream_pi() != 0) {
        printf("Pi stream could not be completed.\n");
        return 1;
    }
    return 0;
}

int actions_process_demo(void)
{
    static int static_cell;
    pid_t pid;
    int status;
    int stack_cell;

    printf("Process / memory demonstration\n");
    printf("------------------------------\n");
    printf("Parent pid: %ld\n", (long)getpid());
    printf("Parent ppid: %ld\n", (long)getppid());
    printf("Address of static cell: %lu\n",
        (unsigned long)(unsigned long)&static_cell);
    printf("Address of stack cell : %lu\n",
        (unsigned long)(unsigned long)&stack_cell);
    printf("\n");

    fflush(stdout);
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        printf("Child pid: %ld\n", (long)getpid());
        printf("Child ppid: %ld\n", (long)getppid());
        printf("Child sees static cell at: %lu\n",
            (unsigned long)(unsigned long)&static_cell);
        printf("Child sees stack cell  at: %lu\n",
            (unsigned long)(unsigned long)&stack_cell);
        printf("Child exiting normally.\n");
        fflush(stdout);
        exit(0);
    }

    wait(&status);
    printf("Child finished with status %d.\n", status);
    printf("The parent remained in control and can return to the menu.\n");
    return 0;
}

void actions_usage(const char *progname)
{
    printf("usage: %s [-p] [-c] [-d page_dir]\n", progname);
    printf("       %s --sysinfo\n", progname);
    printf("       %s --cpu-demo\n", progname);
    printf("       %s --proc-demo\n", progname);
}
