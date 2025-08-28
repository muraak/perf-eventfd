#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sys/mman.h>
#include <sched.h>
#include <sched.h>
#include <sys/resource.h>


int set_realtime(int prio) {
    struct sched_param sp;
    sp.sched_priority = prio;
    if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0) {
        perror("sched_setscheduler");
        return -1;
    }
    return 0;
}


int set_cpu(int core) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    if (sched_setaffinity(0, sizeof(set), &set) < 0) {
        perror("sched_setaffinity");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int use_realtime = 0;
    int rt_prio = 0;
    int use_cpu = 0;
    int parent_core = -1, child_core = -1;
    int use_mlock = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--realtime" && i+1 < argc) {
            use_realtime = 1;
            rt_prio = std::stoi(argv[++i]);
        } else if (std::string(argv[i]) == "--cpu" && i+2 < argc) {
            use_cpu = 1;
            parent_core = std::stoi(argv[++i]);
            child_core = std::stoi(argv[++i]);
        } else if (std::string(argv[i]) == "--mlock") {
            use_mlock = 1;
        }
    }

    if (use_mlock) {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            perror("mlockall");
            std::cerr << "メモリロックに失敗しました" << std::endl;
        }
    }

    int efd1 = eventfd(0, EFD_NONBLOCK);
    int efd2 = eventfd(0, EFD_NONBLOCK);
    if (efd1 == -1 || efd2 == -1) {
        perror("eventfd");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { // Child process
        if (use_cpu && child_core >= 0) {
            if (set_cpu(child_core) < 0) {
                std::cerr << "[child] CPU割り当て失敗\n";
            }
        }
        if (use_realtime) {
            if (set_realtime(rt_prio) < 0) {
                std::cerr << "[child] リアルタイム優先度設定失敗\n";
            }
        }
        int epfd = epoll_create1(0);
        if (epfd == -1) {
            perror("epoll_create1");
            return 1;
        }
        struct epoll_event ev = {0};
        ev.events = EPOLLIN;
        ev.data.fd = efd1;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd1, &ev) == -1) {
            perror("epoll_ctl");
            return 1;
        }
        while (true) {
            struct epoll_event events[1];
            int n = epoll_wait(epfd, events, 1, -1);
            if (n > 0) {
                uint64_t val;
                read(efd1, &val, sizeof(val));
                // 応答
                uint64_t resp = 1;
                write(efd2, &resp, sizeof(resp));
            }
        }
    } else { // Parent process
        if (use_cpu && parent_core >= 0) {
            if (set_cpu(parent_core) < 0) {
                std::cerr << "[parent] CPU割り当て失敗\n";
            }
        }
        if (use_realtime) {
            if (set_realtime(rt_prio) < 0) {
                std::cerr << "[parent] リアルタイム優先度設定失敗\n";
            }
        }
        int epfd = epoll_create1(0);
        if (epfd == -1) {
            perror("epoll_create1");
            return 1;
        }
        struct epoll_event ev = {0};
        ev.events = EPOLLIN;
        ev.data.fd = efd2;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd2, &ev) == -1) {
            perror("epoll_ctl");
            return 1;
        }
        constexpr int N = 10000;
        double total_us = 0;
        double min_us = 1e9, max_us = 0;
        std::vector<double> times;
        times.reserve(N);
        for (int i = 0; i < N; ++i) {
            uint64_t val = 1;
            auto start = std::chrono::high_resolution_clock::now();
            write(efd1, &val, sizeof(val));
            struct epoll_event events[1];
            int n = epoll_wait(epfd, events, 1, -1);
            if (n > 0) {
                uint64_t resp;
                read(efd2, &resp, sizeof(resp));
                auto end = std::chrono::high_resolution_clock::now();
                double us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                total_us += us;
                if (us < min_us) min_us = us;
                if (us > max_us) max_us = us;
                times.push_back(us);
            }
        }
        std::sort(times.begin(), times.end());
        double median_us = (N % 2 == 0) ? (times[N/2-1] + times[N/2]) / 2.0 : times[N/2];
        std::cout << "平均応答時間: " << (total_us / N) << " us\n";
        std::cout << "中央値応答時間: " << median_us << " us\n";
        std::cout << "最小応答時間: " << min_us << " us\n";
        std::cout << "最大応答時間: " << max_us << " us" << std::endl;
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);

    }
    return 0;
}
