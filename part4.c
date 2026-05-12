#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "string_parser.h"

typedef struct {
    char cmd[256];
    char state;
    long utime;
    long stime;
    long prev_utime;
	long prev_stime;
    long vm_rss;
    long voluntary_ctx;
    long nonvoluntary_ctx;
} proc_info;

void read_proc_stat(pid_t pid, proc_info* info) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE* file = fopen(path, "r");
    if (!file) {
		return;
	}

    char buf[4096];
    size_t bytes = fread(buf, 1, sizeof(buf) - 1, file);
	buf[bytes] = '\0';
    fclose(file);

    char* ptr = strrchr(buf, ')');
    if (!ptr) return;
    ptr++;

    //state, ppid, pgrp, session, tty, tpgid,
    //flags, minflt, cminflt, majflt, cmajflt, utime, stime
    char state;
    long discard;
    sscanf(ptr, " %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
        &state,
        &discard, &discard, &discard, &discard, &discard,
        &discard, &discard, &discard, &discard, &discard,
        &info->utime, &info->stime);

    info->state = state;
}

void read_proc_status(pid_t pid, proc_info* info) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
 
    FILE* f = fopen(path, "r");
    if (!f) return;
 
    char buf[4096];
    size_t bytes = fread(buf, 1, sizeof(buf) - 1, f);
    buf[bytes] = '\0';
    fclose(f);
 
    char* ptr;
 
    ptr = strstr(buf, "VmRSS:");
    if (ptr) sscanf(ptr, "VmRSS: %ld", &info->vm_rss);
 
    ptr = strstr(buf, "voluntary_ctxt_switches:");
    if (ptr) sscanf(ptr, "voluntary_ctxt_switches: %ld", &info->voluntary_ctx);
 
    ptr = strstr(buf, "nonvoluntary_ctxt_switches:");
    if (ptr) sscanf(ptr, "nonvoluntary_ctxt_switches: %ld", &info->nonvoluntary_ctx);
}

void collect_proc_info(pid_t pid, proc_info* info) {
    info->prev_utime = info->utime;
    info->prev_stime = info->stime;
    read_proc_stat(pid, info);
    read_proc_status(pid, info);
}

//need these to be global now for access in signal handler
pid_t* pids = NULL;
int* alive = NULL;
proc_info* infos = NULL;
int count = 0;
int current = 0;
int cycle = 0;

void print_proc(int cycle) {
	long ticks_per_sec = sysconf(_SC_CLK_TCK);
	printf("Cycle %d stats:\n", cycle);
	for (int i = 0; i < count; i++) {
		if (!alive[i]) {
			continue;
		}
		proc_info* info = &infos[i];
		long delta_ticks = (info->utime + info->stime) - (info->prev_utime + info->prev_stime);
		long delta_ms = (delta_ticks * 1000) / ticks_per_sec;
		long total_ctx = info->voluntary_ctx + info->nonvoluntary_ctx;
		printf("[%d] CPU(ms): %ld MEM(KB): %ld CTX_SW: %ld\n", pids[i], delta_ms, info->vm_rss, total_ctx);
	}
	fflush(stdout);
}

void sigalrm_handler(int sig) {
	(void)sig;
	cycle++;

	for (int i = 0; i < count; i++) {
		if (alive[i]) {
			collect_proc_info(pids[i], &infos[i]);
		}
	}
	print_proc(cycle);

    //stop current process if still alive
    if (alive[current]) {
		printf("Suspending process [%d]\n", pids[current]);
        kill(pids[current], SIGSTOP);
    }

    //find next alive process
    int next = (current + 1) % count;
    int checked = 0;
    while (!alive[next] && checked < count) {
        next = (next + 1) % count;
        checked++;
    }

    //if process is alive continue it
    if (alive[next]) {
        current = next;
		printf("Resuming process [%d]\n", pids[current]);
        kill(pids[current], SIGCONT);
    }

    alarm(1);
}

int main() {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    //open file for reading
    FILE* input = fopen("input.txt", "r");
    if (input == NULL) {
        perror("fopen failed");
        exit(1);
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);

    //run through file line by line, split each line into args
    while ((read = getline(&line, &len, input)) != -1) {
        fflush(stdout);
        command_line cmd = get_args(line);

        //fork a new pid for each process
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        }
		//sigwait then execvp the process
		//free memory, exit with status 1 if execvp fails
        else if (pid == 0) {
            printf("Process [%s] waiting for signal...\n", cmd.command_list[0]);
            int sig;
            if (sigwait(&set, &sig) == 0) {
                execvp(cmd.command_list[0], cmd.command_list);
                perror("execvp failed");
                fclose(input);
                free_cmd(&cmd);
                free(line);
                free(pids);
                free(alive);
				free(infos);
                exit(1);
            } else {
                perror("sigwait failed");
                exit(1);
            }
        }
        //if execvp is succesful, print that it has been launched, add it to pid array
		//add to alive arr
        else {
            printf("Launching: [%d]: %s\n", pid, line);
            fflush(stdout);
            pids = realloc(pids, (count + 1) * sizeof(pid_t));
            alive = realloc(alive, (count + 1) * sizeof(int));
			infos = realloc(infos, (count + 1) * sizeof(proc_info));
            pids[count] = pid;
            alive[count] = 1;
			memset(&infos[count], 0, sizeof(proc_info));
            count++;
        }
        free_cmd(&cmd);
    }

    for (int i = 0; i < count; i++) {
        kill(pids[i], SIGUSR1);
    }
    sleep(1);
    printf("Processes started\n");
    for (int i = 0; i < count; i++) {
        kill(pids[i], SIGSTOP);
    }
    printf("Processes paused\n");

    signal(SIGALRM, sigalrm_handler);

    //start first alive process and begin scheduling
    current = 0;
    kill(pids[current], SIGCONT);
    alarm(1);

	//wait all processes with status, to see if stopped or finished
    int finished = 0;
    while (finished < count) {
        for (int i = 0; i < count; i++) {
            if (!alive[i]) continue;
            int status;
            pid_t result = waitpid(pids[i], &status, WUNTRACED | WNOHANG);
            if (result > 0) {
                if (WIFEXITED(status)) {
                    alive[i] = 0;
                    finished++;
                    printf("Finished [%d]: exited with status %d\n", pids[i], WEXITSTATUS(status));
                    fflush(stdout);
                }
            }
        }
        sleep(1);
    }

    alarm(0);

    fclose(input);
    free(line);
    free(pids);
    free(alive);
	free(infos);
    exit(0);
}
