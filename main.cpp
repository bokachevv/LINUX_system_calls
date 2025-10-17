#include <sys/types.h>    // pid_t, time_t
#include <sys/times.h>    // times(), struct tms
#include <sys/time.h>     // setitimer, struct itimerval
#include <signal.h>       // sigaction, SIGALRM, SIGTSTP
#include <unistd.h>       // fork, pause, getpid, sysconf
#include <sys/wait.h>     // wait
#include <time.h>         // time, ctime
#include <iostream>       // cout, cerr
#include <cstdlib>        // exit, stoi
#include <string>

using namespace std;

int launches = 0;
int max_launches = 0;
int period_sec = 0;
long clk_tck;

void handler(int signum) {

    if (launches >= max_launches) {
        struct itimerval zero = {{0, 0}, {0, 0}};
        if (setitimer(ITIMER_REAL, &zero, NULL) == -1) {
            cerr << "setitimer failed" << endl;
            exit(1);
        }
        exit(0);
    }

    struct tms before;
    clock_t start = times(&before);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }
    // Child
    else if (pid == 0) {
        time_t now = time(NULL);
        cout << "Child process" << (launches + 1) << ": PID = " << getpid() << ", start time = " << ctime(&now);
        for (int i = 0; i < 100000; i++);
        exit(0);
    }
    // Parent
    else {
        wait(NULL);
        struct tms after;
        clock_t end = times(&after);

        double elapsed_real = (end - start) / static_cast<double>(clk_tck);
        double parent_user = (after.tms_utime - before.tms_utime) / static_cast<double>(clk_tck);
        double parent_sys = (after.tms_stime - before.tms_stime) / static_cast<double>(clk_tck);
        double child_user = (after.tms_cutime - before.tms_cutime) / static_cast<double>(clk_tck);
        double child_sys = (after.tms_cstime - before.tms_cstime) / static_cast<double>(clk_tck);

        cout << "Parent: real time " << elapsed_real << " sec, user " << parent_user << ", sys " << parent_sys << endl;
        cout << "Child: user " << child_user << ", sys " << child_sys << endl;

        launches++;
    }
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " period_sec num_launches" << endl;
        return EXIT_FAILURE;
    }
    try {
        period_sec = stoi(argv[1]);
        max_launches = stoi(argv[2]);
        if (period_sec <= 0 || max_launches <= 0) {
            throw invalid_argument("Arguments must be positive");
        }
    }
    catch (const exception& e) {
        cerr << "Invalid arguments: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    clk_tck = sysconf(_SC_CLK_TCK);
    if (clk_tck == -1) {
        perror("sysconf failed");
        return EXIT_FAILURE;
    }

    struct sigaction act_ignore;
    act_ignore.sa_handler = SIG_IGN;
    if (sigaction(SIGTSTP, &act_ignore, NULL) == -1) {
        perror("sigaction SIGTSTP failed");
        return EXIT_FAILURE;
    }

    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        perror("sigaction SIGALRM failed");
        return EXIT_FAILURE;
    }

    struct itimerval it;
    it.it_interval.tv_sec = period_sec;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = period_sec;
    it.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &it, NULL) == -1) {
        perror("setitimer failed");
        return EXIT_FAILURE;
    }

    while (true) {
        pause();
    }

    return EXIT_SUCCESS;
}