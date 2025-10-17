#include <sys/types.h>    // pid_t, time_t
#include <sys/times.h>    // times(), struct tms
#include <sys/time.h>     // setitimer, struct itimerval
#include <signal.h>       // sigaction, SIGALRM, SIGTSTP
#include <unistd.h>       // fork, pause, getpid, sysconf
#include <sys/wait.h>     // wait
#include <ctime>          // time, ctime
#include <iostream>       // cout, cerr
#include <cstdlib>        // exit, stoi
#include <string>         // stoi
#include <stdexcept>      // invalid_argument

using namespace std;

volatile int current_launch_num = 0;
long clk_tck;

void handler(int sig) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        time_t now = time(NULL);
        cout << "Child process " << (current_launch_num + 1) << ": PID = " << getpid()
             << ", start time = " << ctime(&now) << flush;

        struct tms child_start;
        clock_t child_start_time = times(&child_start);

        // Какая-то работа
        for (long i = 0; i < 200000000; i++);

        struct tms child_end;
        clock_t child_end_time = times(&child_end);

        double child_total = (child_end_time - child_start_time) / static_cast<double>(clk_tck);
        cout << "Child process " << (current_launch_num + 1) << ": total time = " << child_total << " sec" << endl;

        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " period_sec num_launches" << endl;
        return EXIT_FAILURE;
    }

    int max_launches;
    int period_sec;
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
    pid_t parent_pid = getpid();

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

    struct tms dummy;
    clock_t start_time = times(&dummy);

    setitimer(ITIMER_REAL, &it, NULL);

    for (current_launch_num = 0; current_launch_num < max_launches; current_launch_num++) {
        pause();
        wait(NULL);
    }

    struct itimerval zero = {{0, 0}, {0, 0}};
    setitimer(ITIMER_REAL, &zero, NULL);

    struct tms final_tms;
    clock_t end_time = times(&final_tms);
    double total_real = (end_time - start_time) / static_cast<double>(clk_tck);
    cout << "Parent PID = " << parent_pid << ": total real time = " << total_real << " sec" << endl;
    cout << "Program finished after " << max_launches << " launches." << endl;

    return EXIT_SUCCESS;
}