#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "geom.h"
#include "dict.h"

#define GRISTMGR_DEBUG 1

#define GRISTMGR_INIT 1
#define GRISTMGR_GETX 2
#define GRISTMGR_SETX 4
#define GRISTMGR_PUTX 8
#define GRISTMGR_DELX 16
#define GRISTMGR_UNSE 32
#define GRISTMGR_STAT 64

static int verbose_flag = 0;
static int action_flag = 0;

static int gristmgr_pid = 0;

void report(char* msg) {
    if(GRISTMGR_DEBUG) {
        printf("gristmgr(%d):\t%s\n", gristmgr_pid, msg);
    }
    return;
}

void usage() {
    printf("usage goes here\n");
    printf("gristmgr v0.01\n\nusage: gristmgr <options ...> dbfilename\n\n");
    printf("\t--verbose\t\t\tverbose mode\n");
    printf("\t--brief\t\t\t\tbrief mode\n");
    printf("\t--init\t\t\t\tinitialize database\n");
    printf("\t--stat\t\t\t\tdb status\n");
    printf("\n");
    return;
}

int parseopts(int argc, char** argv) {
    
    int c;
    
    static struct option lopts[] = {
        {"verbose", no_argument, &verbose_flag, 1},
        {"brief", no_argument, &verbose_flag, 0},
        {"init", no_argument, &action_flag, GRISTMGR_INIT},
        {"stat", no_argument, &action_flag, GRISTMGR_STAT}
    };

    while(1) {
        int opt_idx = 0;

        c = getopt_long(argc, argv, "", lopts, &opt_idx);

        if(c == -1) break;

        switch(c) {
            case 0:
                if(lopts[opt_idx].flag != 0) break;
                printf("lopt %s set", lopts[opt_idx].name);
                if(optarg) printf(", arg: %s\n", optarg);
                printf("\n");
                break;
            default:
                abort();
                break;
        }

    }

    return optind;
}

int main(int argc, char** argv) {
    
    gristmgr_pid = getpid();

    int optind = parseopts(argc, argv);
    if(optind >= argc) {
        usage();
        exit(EXIT_FAILURE);
    }

    int fnmsz = strlen(argv[argc-1])+1;
    char* fnm = malloc(fnmsz);
    strcpy(fnm, argv[argc-1]);
    fnm[fnmsz-1] = '\0';
    printf("fnm: %s\n", fnm);

    return EXIT_SUCCESS;   
}
