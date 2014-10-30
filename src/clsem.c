/*
   -	clsem --
   -		Command line interface to a semaphore.
   -
   .	See clsem (1).
   .
   .	Copyright (c) 2011, Gordon D. Carrie. All rights reserved.
   .	
   .	Redistribution and use in source and binary forms, with or without
   .	modification, are permitted provided that the following conditions
   .	are met:
   .	
   .	    * Redistributions of source code must retain the above copyright
   .	    notice, this list of conditions and the following disclaimer.
   .
   .	    * Redistributions in binary form must reproduce the above copyright
   .	    notice, this list of conditions and the following disclaimer in the
   .	    documentation and/or other materials provided with the distribution.
   .	
   .	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   .	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   .	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   .	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   .	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   .	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   .	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   .	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   .	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   .	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   .	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   .
   .	Please send feedback to dev0@trekix.net
   .	$Revision: $ $Date: $
 */

#include "unix_defs.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define CLSEM_VERSION "0.1"

#define PERM_FILE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static char *argv0;			/* Program name, argv[0] */

/* Convenience function */
static key_t get_key(char *, char *);
static int get_sem(char *, char *);

int main(int argc, char *argv[])
{
    char *op;
    char *path;				/* Path for key */
    char *key_id = "1";			/* Identifier for key */
    key_t key;				/* Key for path and type */
    int sem_id;				/* Semaphore identifier */
    short n;				/* Increment to semaphore value */
    union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
    } arg;
    struct semid_ds buf;		/* Buffer for semctl */
    struct sembuf sop;			/* Semaphore operation */
    

    argv0 = argv[0];
    if ( argc < 2 ) {
	fprintf(stderr, "Usage: %s option [id]\n", argv0);
	exit(EXIT_FAILURE);
    }
    op = argv[1];
    if ( strcmp(op, "-V") == 0 ) {
	printf("%s %s: Copyright (c) 2011. Gordon D. Carrie. "
		"All rights reserved.\n", argv0, CLSEM_VERSION);
	exit(EXIT_SUCCESS);
    } else if ( strcmp(op, "-c") == 0 ) {
	/*
	   Create a new semaphore. Fail if it already exists. Create it
	   writeable but not readable. This will indicate to other clsem
	   processes that the semaphore is still being initialized.  Make
	   the semaphore readable when done initializing.
	 */

	if ( argc == 3 ) {
	    path = argv[2];
	} else if ( argc == 4 ) {
	    path = argv[2];
	    key_id = argv[3];
	} else {
	    fprintf(stderr, "Usage: %s -c file [id]\n", argv0);
	    exit(EXIT_FAILURE);
	}
	key = get_key(path, key_id);
	if ( (sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | S_IWUSR)) == -1 ) {
	    fprintf(stderr, "%s: could not create new semaphore for "
		    "path %s, id %s\n%s\n",
		    argv0, path, key_id, strerror(errno));
	    exit(EXIT_FAILURE);
	}
	arg.val = 0;
	if ( semctl(sem_id, 0, SETVAL, arg) == -1 ) {
	    fprintf(stderr, "%s: could not initialize new semaphore for "
		    "path %s, id %s\n%s\n",
		    argv0, path, key_id, strerror(errno));
	    if ( semctl(sem_id, 0, IPC_RMID) == -1 ) {
		fprintf(stderr, "%s: could not remove semaphore for path %s, "
			"id %s.\n%s\nPlease remove it with\nipcrm -s %d\n",
			argv0, path, key_id, strerror(errno), sem_id);
	    }
	    exit(EXIT_FAILURE);
	}
	buf.sem_perm.uid = getuid();
	buf.sem_perm.gid = getgid();
	buf.sem_perm.mode = PERM_FILE;
	arg.buf = &buf;
	if ( semctl(sem_id, 0, IPC_SET, arg) == -1 ) {
	    fprintf(stderr, "%s: could not set permissions for new "
		    "semaphore for path %s, id %s.\n%s\n",
		    argv0, path, key_id, strerror(errno));
	    if ( semctl(sem_id, 0, IPC_RMID) == -1 ) {
		fprintf(stderr, "%s: could not remove semaphore for path %s, "
			"id %s.\n%s\nPlease remove it with\nipcrm -s %d\n",
			argv0, path, key_id, strerror(errno), sem_id);
	    }
	    exit(EXIT_FAILURE);
	}
	printf("%d\n", sem_id);
    } else if ( sscanf(argv[1], "%hd", &n) == 1 ) {
	/*
	   Increment semaphore value.
	 */

	if ( argc == 3 ) {
	    path = argv[2];
	} else if ( argc == 4 ) {
	    path = argv[2];
	    key_id = argv[3];
	} else {
	    fprintf(stderr, "Usage: %s increment path [id]\n", argv0);
	    exit(EXIT_FAILURE);
	}
	sem_id = get_sem(path, key_id);
	sop.sem_num = 0;
	sop.sem_op = n;
	sop.sem_flg = 0;
	if ( semop(sem_id, &sop, 1) == -1 ) {
	    fprintf(stderr, "%s: could not adjust semaphore for path %s, id %s,"
		    " by %d\n%s\n", argv0, path, key_id, n, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    } else if ( strcmp(op, "-v") == 0 ) {
	/*
	   Print semaphore value.
	 */

	int v;

	if ( argc == 3 ) {
	    path = argv[2];
	} else if ( argc == 4 ) {
	    path = argv[2];
	    key_id = argv[3];
	} else {
	    fprintf(stderr, "Usage: %s -v path [id]\n", argv0);
	    exit(EXIT_FAILURE);
	}
	sem_id = get_sem(path, key_id);
	v = semctl(sem_id, 0, GETVAL);
	if ( v == -1 ) {
	    fprintf(stderr, "%s: could not get value of semaphore for path %s,"
		    " id %s\n%s\n", argv0, path, key_id, strerror(errno));
	    exit(EXIT_FAILURE);
	}
	printf("%d\n", v);
    } else if ( strcmp(op, "-d") == 0 ) {
	/*
	   Delete semaphore
	 */

	if ( argc == 3 ) {
	    path = argv[2];
	} else if ( argc == 4 ) {
	    path = argv[2];
	    key_id = argv[3];
	} else {
	    fprintf(stderr, "Usage: %s -d path [id]\n", argv0);
	    exit(EXIT_FAILURE);
	}
	sem_id = get_sem(path, key_id);
	if ( semctl(sem_id, 0, IPC_RMID, 0) == -1 ) {
	    fprintf(stderr, "%s: could not delete semaphore for id "
		    "%s\n%s\n", argv0, key_id, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    } else {
	fprintf(stderr, "Usage: %s [-c|-v|-d|n] path [id]\n", argv0);
	exit(EXIT_FAILURE);
    }

    return 0;
}

static key_t get_key(char *path, char *key_id)
{
    key_t key;

    if ( strlen(key_id) > 1 ) {
	fprintf(stderr, "%s: key id must be one character.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( (key = ftok(path, *key_id)) == -1 ) {
	fprintf(stderr, "%s: could not get key for path %s, id %s\n%s\n",
		argv0, path, key_id, strerror(errno));
	exit(EXIT_FAILURE);
    }
    return key;
}

/*
   Obtain id of semaphore associated with key key_id and file path.
   If semaphore does not exist or is not readable, assume it is still
   being created or initialized. Attempt to get semaphore id num_tries
   times. If still unsuccessful, process exits.
 */

static int get_sem(char *path, char *key_id)
{
    int n, num_tries = 9;
    int sem_id;
    key_t key;

    key = get_key(path, key_id);
    for (n = 0, sem_id = -1; n < num_tries && sem_id == -1; n++ ) {
        sem_id = semget(key, 1, PERM_FILE);
        if ( sem_id == -1 ) {
            if ( errno == ENOENT || errno == EACCES ) {
                sleep(1);
            } else {
                fprintf(stderr, "%s: could not get semaphore for path %s,"
			" id %s.\n%s\n", argv0, path, key_id, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }
    if ( n == num_tries ) {
        fprintf(stderr, "%s: gave up waiting for semaphore for path %s,"
		" id %s.\n", argv0, path, key_id);
	exit(EXIT_FAILURE);
    }
    return sem_id;
}
