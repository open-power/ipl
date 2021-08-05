extern "C"
{

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libpdbg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
}

#include <libekb.H>
#include <libipl/libipl.H>

static bool isstring(const char* arg)
{
    size_t i;

    if (!arg || arg[0] == '\0' || !isalpha(arg[0]))
        return false;

    for (i = 1; i < strlen(arg); i++)
    {
        if (!isalnum(arg[i]) && arg[i] != '_')
            return false;
    }

    return true;
}

static bool is_range(const char* arg, int* begin, int* end)
{
    int ret;

    ret = sscanf(arg, "%d..%d", begin, end);
    if (ret == 2)
        return true;

    return false;
}

static bool run_istep(const char* arg, int* rc)
{
    char *ptr1 = NULL, *ptr2 = NULL;
    long int major, minor;
    int begin = -1, end = -1;

    assert(arg);

    if (isstring(arg))
    {
        printf("Running istep procedure %s\n", arg);
        *rc = ipl_run_step(arg);
        return true;
    }

    if (is_range(arg, &begin, &end))
    {
        int i;

        if (begin < 0 || begin > MAX_ISTEP)
        {
            fprintf(stderr, "Invalid range %s\n", arg);
            return false;
        }

        if (end < 0 || end > MAX_ISTEP)
        {
            fprintf(stderr, "Invalid range %s\n", arg);
            return false;
        }

        if (end < begin)
        {
            fprintf(stderr, "Invalid range %s\n", arg);
            return false;
        }

        for (i = begin; i <= end; i++)
        {
            printf("Running istep %d\n", i);
            *rc = ipl_run_major(i);
            if (*rc)
                break;
        }

        return true;
    }

    errno = 0;
    major = strtol(arg, &ptr1, 10);
    if (errno)
    {
        fprintf(stderr, "Invalid istep number %s\n", arg);
        return false;
    }

    if (!ptr1 || ptr1[0] == '\0')
    {
        printf("Running major istep %d\n", (int)major);
        *rc = ipl_run_major((int)major);
        return true;
    }

    if (*ptr1 != '.')
    {
        fprintf(stderr, "Invalid istep number %s\n", arg);
        return false;
    }

    errno = 0;
    minor = strtol(ptr1 + 1, &ptr2, 10);
    if (errno)
    {
        fprintf(stderr, "Invalid istep number %s\n", arg);
        return false;
    }

    if (!ptr2 || ptr2[0] == '\0')
    {
        printf("Running istep %d.%d\n", (int)major, (int)minor);
        *rc = ipl_run_major_minor((int)major, (int)minor);
        return true;
    }

    fprintf(stderr, "Invalid istep number %s\n", arg);
    return false;
}

static void usage(void)
{
    fprintf(stderr,
            "Usage: ipl [options] [<istep..<istep>] <istep> [<istep>...]\n");
    fprintf(stderr, "   Options:\n");
    fprintf(stderr, "      -b kernel  for kernel backend\n");
    fprintf(stderr, "      -b sbefifo  for sbefifo backend (default)\n");
    fprintf(stderr, "      -b cronus -d <host>  for cronus backend\n");
    fprintf(stderr, "      -D <0-5>  set log level\n");
}

int main(int argc, char* const* argv)
{
    const char* device = NULL;
    enum pdbg_backend backend = PDBG_BACKEND_SBEFIFO;
    int rc, i, opt, log_level = 0;
    bool do_backend = false;

    while ((opt = getopt(argc, argv, "b:d:D:")) != -1)
    {
        switch (opt)
        {
            case 'b':
                if (!strcmp(optarg, "kernel"))
                    backend = PDBG_BACKEND_KERNEL;
                else if (!strcmp(optarg, "fake"))
                    backend = PDBG_BACKEND_FAKE;
                else if (!strcmp(optarg, "cronus"))
                    backend = PDBG_BACKEND_CRONUS;
                else if (!strcmp(optarg, "sbefifo"))
                    backend = PDBG_BACKEND_SBEFIFO;
                else
                {
                    fprintf(stderr, "Invalid backend '%s'\n", optarg);
                    exit(1);
                }

                do_backend = true;
                break;

            case 'd':
                if (backend != PDBG_BACKEND_CRONUS)
                {
                    usage();
                    exit(1);
                }
                device = optarg;
                break;

            case 'D':
                log_level = atoi(optarg);
                if (log_level < 0)
                    log_level = 0;
                if (log_level > 5)
                    log_level = 5;
                break;

            default:
                usage();
                exit(1);
        }
    }
    if (argc - optind < 1)
    {
        usage();
        exit(1);
    }

    if (do_backend)
        pdbg_set_backend(backend, device);

    pdbg_set_loglevel(log_level);

    if (!pdbg_targets_init(NULL))
        exit(1);

    libekb_set_loglevel(log_level);

    if (libekb_init())
        exit(1);

    ipl_set_loglevel(log_level);

    if (ipl_init(IPL_HOSTBOOT))
        exit(1);

    for (i = optind; i < argc; i++)
    {
        rc = 0;
        if (!run_istep(argv[i], &rc))
            return -1;

        if (rc)
            break;
    }

    return 0;
}
