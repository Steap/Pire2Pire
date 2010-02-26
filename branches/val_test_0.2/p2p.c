#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "user_interface.h"
#include "client_interface.h"

pid_t user_interface_pid = 0, client_interface_pid = 0;

static void
kill_from_sigint (int signum) {
    int status;
    (void)signum;
    printf ("Received SIGINT\n");
    kill (user_interface_pid, SIGINT);
    kill (client_interface_pid, SIGINT);
    waitpid (-1, &status, 0);
    printf("Killed everyone successfully\n");
    exit (EXIT_SUCCESS);
}

static void
kill_from_user (int signum) {
    (void)signum;
    kill (client_interface_pid, SIGINT);
    sleep(1);
    exit (EXIT_SUCCESS);
}

// Should not happen, useful for tests
static void
kill_from_client (int signum) {
    (void)signum;
    kill (user_interface_pid, SIGINT);
    sleep(1);
    exit (EXIT_SUCCESS);
}

// I sleep the two interfaces until the father is ready and awakens them
static void
wake_up (int signum) {
    (void)signum;
}

int
main (int argc, char **argv) {
/*
    int client_server_pipe_fd[2];
    int server_client_pipe_fd[2];
*/
    pid_t father_pid;
    pid_t pid;
    int port;

    if (argc != 2) {
        printf ("Usage : ./p2p PORT\n");
        exit (EXIT_FAILURE);
    }
    if (sscanf (argv[1], "%d", &port) == EOF) {
        perror ("sscanf");
        printf ("Error reading your port. See stderr for more details.\n");
        exit (EXIT_FAILURE);
    }
    if (port < 1024 || port > 49151) {
        printf ("Please use a port between 1024 and 49151.\n");
        exit (EXIT_FAILURE);
    }

    // Creating the pipes for IPC
/*
    if (pipe (client_server_pipe_fd) < 0) {
        perror ("pipe 1");
        exit (EXIT_FAILURE);
    }
    if (pipe (client_server_pipe_fd) < 0) {
        perror ("pipe 2");
        exit (EXIT_FAILURE);
    }
*/

    father_pid = getpid();

    // Creating the user interface process
    pid = fork ();
    switch (pid) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);
        case 0:
 /*
            close (client_server_pipe_fd[1]);
            close (server_client_pipe_fd[0]);
*/
            signal (SIGUSR1, wake_up);
            pause ();
            user_interface (father_pid);
            printf ("User interface ended unexpectedly\n");
            kill (father_pid, SIGUSR1);
            printf ("User interface killed father\n");
            exit (EXIT_FAILURE);
            break;
        default:
            user_interface_pid = pid;
            break;
    }

    // Creating the server-side
    pid = fork ();
    switch (pid) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);
        case 0:
/*
            close (client_server_pipe_fd[0]);
            close (server_client_pipe_fd[1]);
*/
            signal (SIGUSR1, wake_up);
            pause ();
            client_interface (father_pid, port);
            printf ("Server-side ended unexpectedly\n");
            kill(father_pid, SIGUSR2);
            exit (EXIT_FAILURE);
            break;
        default:
            client_interface_pid = pid;
            break;
    }

    sleep(1);

    // Prepares the signal handling then wakes his children
    signal (SIGINT, kill_from_sigint);
    signal (SIGUSR1, kill_from_user);
    signal (SIGUSR2, kill_from_client);
    kill (user_interface_pid, SIGUSR1);
    kill (client_interface_pid, SIGUSR1);

    // The father waits until he is signaled
    for(;;) {
        pause();
        printf("Father has been awakened\n");
    }

    return EXIT_SUCCESS;
}
