#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include "dynlib_monitor.skel.h"

static volatile bool exiting = false;

struct event {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char comm[16];
    char lib_path[256];
    __u64 lib_addr;
    char symbol_name[64];
    int event_type;
};

void sig_handler(int sig)
{
    exiting = true;
}

static void handle_event(void *ctx, int cpu, void *data, __u32 data_size)
{
    const struct event *e = static_cast<const struct event*>(data);
    struct tm *tm;
    char ts[32];
    time_t t;
    
    time(&t);
    tm = localtime(&t);
    strftime(ts, sizeof(ts), "%H:%M:%S", tm);

    std::string event_type;
    switch (e->event_type) {
        case 1:
            event_type = "加载动态库";
            printf("[%s] 进程: %s (PID: %u) %s: %s\n",
                   ts, e->comm, e->pid, event_type.c_str(), e->lib_path);
            break;
        case 2:
            event_type = "卸载动态库";
            printf("[%s] 进程: %s (PID: %u) %s: 地址 0x%llx\n",
                   ts, e->comm, e->pid, event_type.c_str(), e->lib_addr);
            break;
        case 3:
            event_type = "符号解析";
            printf("[%s] 进程: %s (PID: %u) %s: %s (库地址: 0x%llx)\n",
                   ts, e->comm, e->pid, event_type.c_str(), e->symbol_name, e->lib_addr);
            break;
    }
}

static void handle_lost_events(void *ctx, int cpu, __u64 lost_cnt)
{
    printf("丢失 %llu 个事件\n", lost_cnt);
}

int main(int argc, char *argv[])
{
    struct dynlib_monitor_bpf *skel;
    struct perf_buffer *pb = NULL;
    int err;

    // 设置信号处理
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 打开和加载 BPF 程序
    skel = dynlib_monitor_bpf__open_and_load();
    if (!skel) {
        std::cerr << "无法打开和加载 BPF 程序" << std::endl;
        return 1;
    }

    // 附加 BPF 程序
    err = dynlib_monitor_bpf__attach(skel);
    if (err) {
        std::cerr << "无法附加 BPF 程序" << std::endl;
        goto cleanup;
    }

    // 设置 perf buffer
    pb = perf_buffer__new(bpf_map__fd(skel->maps.events), 64,
                         handle_event, handle_lost_events, NULL, NULL);
    if (!pb) {
        err = -1;
        std::cerr << "无法创建 perf buffer" << std::endl;
        goto cleanup;
    }

    std::cout << "动态库监控程序已启动..." << std::endl;

    // 主循环
    while (!exiting) {
        err = perf_buffer__poll(pb, 100);
        if (err < 0 && err != -EINTR) {
            printf("错误: perf_buffer__poll 返回 %d\n", err);
            break;
        }
    }

cleanup:
    perf_buffer__free(pb);
    dynlib_monitor_bpf__destroy(skel);
    return err < 0 ? -err : 0;
} 