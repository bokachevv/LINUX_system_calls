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
pid_t parent_pid;
clock_t start_time;

void handler(int sig) {
    if (launches >= max_launches) {
        struct tms final;
        clock_t end_time = times(&final);
        double total_real = (end_time - start_time) / static_cast<double>(clk_tck);
        cout << "Parent PID = " << parent_pid << ": total real time = " << total_real << " sec" << endl;
        struct itimerval zero = {{0, 0}, {0, 0}};
        setitimer(ITIMER_REAL, &zero, NULL);
        exit(0);
    }

    struct tms before;
    clock_t start = times(&before);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        time_t now = time(NULL);
        cout << "Child process " << (launches + 1) << ": PID = " << getpid()
             << ", start time = " << ctime(&now) << flush;

        struct tms child_start;
        clock_t child_start_time = times(&child_start);

        for (int i = 0; i < 100000000; i++);  // Нагрузка

        struct tms child_end;
        clock_t child_end_time = times(&child_end);
        double child_total = (child_end_time - child_start_time) / static_cast<double>(clk_tck);
        cout << "Child process " << (launches + 1) << ": total time = " << child_total << " sec" << endl;

        exit(0);
    } else {
        wait(NULL);

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
    } catch (const exception& e) {
        cerr << "Invalid arguments: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    clk_tck = sysconf(_SC_CLK_TCK);
    parent_pid = getpid();
    struct tms dummy;
    start_time = times(&dummy);

    struct sigaction act_ignore;
    act_ignore.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &act_ignore, NULL);

    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);

    struct itimerval it;
    it.it_interval.tv_sec = period_sec;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = period_sec;
    it.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &it, NULL);

    while (true) {
        pause();
    }

    return EXIT_SUCCESS;
}