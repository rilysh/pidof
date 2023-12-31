#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <kvm.h>
#include <getopt.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>
#include <sys/user.h>

/* Option flags. */
struct opt_flags {
	int one_flag;
        int delim_flag;
	int pidomit_flag;
};

/* Convert string-type integers to actual integers */
static int xatoi(const char *src)
{
	char *eptr;
	long ret;

	ret = strtol(src, &eptr, 10);

	/* We can't return anything here,
	   so we'll exit. */
	if (eptr == src) {
		fputs("pidof: argument is not an integer.\n",
		      stderr);
		exit(EXIT_FAILURE);
	}

	return (int)((int)(ret) & INT32_MAX);
}

/* Print all spawned process PIDs */
static void print_process_pids(
	int once, int pid_omit, const char *delim,
	const char *name)
{
	kvm_t *kvm;
	struct kinfo_proc *kproc;
	int nproc, is_first, is_living, i;

	is_living = 1;
	is_first = 1;

	kvm = kvm_open(NULL, "/dev/null", NULL, O_RDONLY, "kvm_open");
	if (kvm == NULL) {
		perror("kvm_open()");
	        exit(EXIT_FAILURE);
	}

	/* Get a list of every single process
	   on current system, except threads. */
	kproc = kvm_getprocs(kvm, KERN_PROC_PROC, 0, &nproc);
	if (kproc == NULL) {
		perror("kvm_getprocs()");
	        exit(EXIT_FAILURE);
	}

	for (i = 0; i < nproc; i++) {
		if (strcmp(kproc[i].ki_comm, name) == 0) {
			is_living = 0;
			if (kproc[i].ki_pid == pid_omit)
				continue;
			if (is_first) {
				fprintf(stdout, "%s: %d", name,
					kproc[i].ki_pid);
				is_first = 0;
			} else {
				fprintf(stdout, "%s%d", delim,
					kproc[i].ki_pid);
			}

			if (once)
				break;
		}
        }

	/* Exit if no process was found with that name */
	if (is_living) {
		fprintf(stderr, "pidof: no process called \"%s\" was found.\n",
			name);
		kvm_close(kvm);
		exit(EXIT_FAILURE);
	}
        
	fputc('\n', stdout);
	kvm_close(kvm);
}

/* Show the usage */
__dead2
static void usage(int status)
{
	fputs("Usage: pidof [OPTION]... <process>\n"
	      "Retrieve one or more process PID(s)\n\n"
	      "Options:\n"
	      "  -o\t\tprint single matching process PID\n"
	      "  -r\t\tomit a PID from being displayed\n"
	      "  -d\t\tadd a delimiter to seperate PIDs\n"
	      "  -h\t\tprint this help page\n"
	      , status == EXIT_FAILURE ? stderr : stdout);

	exit(status);
}

int main(int argc, char **argv)
{
	int opt, opid, is_okay, i;
	struct opt_flags flag = {0};
	char *delim;

        is_okay = 0;
	delim = " ";

	if (argc < 2 || (argv[1][0] == '-' && argv[1][1] == '\0'))
		usage(EXIT_FAILURE);

	while ((opt = getopt(argc, argv, "os:r:d:h")) != -1) {
		switch (opt) {
		case 'o':
			/* -o with no argument(s) */
		        flag.one_flag = 1;
			break;

		case 'r':
			/* -r with an argument */
			flag.pidomit_flag = 1;
			opid = xatoi(optarg);
			break;

		case 'd':
			/* -d with an argument */
		        flag.delim_flag = 1;
			delim = optarg;
			break;

		case 'h':
			/* -h with no argument(s) */
			usage(EXIT_SUCCESS);

		default:
			exit(EXIT_FAILURE);
		}
	}

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			continue;

		is_okay = 1;
		print_process_pids(flag.one_flag, opid, delim, argv[i]);
	}

	/* Oops, not okay - means no arguments were passed */
	if (is_okay == 0) {
	        fputs("pidof: no process name(s) were passed.\n",
		      stderr);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
