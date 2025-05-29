#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <vector>
#include <dlfcn.h>
#include <link.h>
#include <sys/sysinfo.h>
#include "dynlib_monitor.skel.h"

static volatile bool exiting = false;

struct event {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char comm[16];
    char lib_path[64];
    __u64 lib_addr;
    char symbol_name[32];
    int event_type;
    int flags;
    char dependent_lib[64];
    __u64 symbol_addr;
    int result;
};

void sig_handler(int sig)
{
    exiting = true;
}

// 获取当前的单调时间（以纳秒为单位）
static __u64 get_monotonic_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 获取当前的真实时间（以纳秒为单位）
static __u64 get_realtime_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

std::string get_formatted_timestamp(__u64 event_timestamp_ns)
{
    // 获取当前的单调时间和真实时间
    __u64 current_monotonic_ns = get_monotonic_ns();
    __u64 current_realtime_ns = get_realtime_ns();
    
    // 计算事件发生时的真实时间
    // current_realtime - current_monotonic 得到真实时间和单调时间的偏移
    // event_timestamp 是事件发生时的单调时间
    // 将偏移加到事件时间戳上，就得到事件发生时的真实时间
    __u64 real_event_time_ns = current_realtime_ns - current_monotonic_ns + event_timestamp_ns;
    
    // 转换为秒和纳秒
    time_t seconds = real_event_time_ns / 1000000000ULL;
    __u64 nanoseconds = real_event_time_ns % 1000000000ULL;
    
    // 格式化时间字符串
    struct tm *tm = localtime(&seconds);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
    
    // 添加微秒部分
    std::stringstream ss;
    ss << buffer << "." << std::setfill('0') << std::setw(6) << (nanoseconds / 1000);
    return ss.str();
}

std::string get_dlopen_flags(int flags)
{
    std::vector<std::string> flag_strings;
    
    if (flags & RTLD_LAZY) flag_strings.push_back("RTLD_LAZY");
    if (flags & RTLD_NOW) flag_strings.push_back("RTLD_NOW");
    if (flags & RTLD_GLOBAL) flag_strings.push_back("RTLD_GLOBAL");
    if (flags & RTLD_LOCAL) flag_strings.push_back("RTLD_LOCAL");
    
    if (flag_strings.empty()) return "0";
    
    std::string result = flag_strings[0];
    for (size_t i = 1; i < flag_strings.size(); i++) {
        result += " | " + flag_strings[i];
    }
    return result;
}

std::string get_lib_real_path(const std::string& lib_name)
{
    void* handle = dlopen(lib_name.c_str(), RTLD_LAZY);
    if (!handle) {
        return lib_name;
    }

    link_map* map;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0) {
        std::string result = map->l_name;
        dlclose(handle);
        return result.empty() ? lib_name : result;
    }

    dlclose(handle);
    return lib_name;
}

static void handle_event(void *ctx, int cpu, void *data, __u32 data_size)
{
    const struct event *e = static_cast<const struct event*>(data);
    std::string timestamp = get_formatted_timestamp(e->timestamp);

    switch (e->event_type) {
        case 1: // explicit load
            if (e->lib_addr == 0) {
                std::string real_path = get_lib_real_path(e->lib_path);
                std::cout << "[" << timestamp << "] 事件：显式库加载请求\n"
                         << "调用函数: dlopen\n"
                         << "请求路径: " << real_path << "\n"
                         << "标志: " << get_dlopen_flags(e->flags) << "\n";
            } else {
                std::cout << "加载基址: 0x" << std::hex << e->lib_addr << std::dec << "\n\n";
            }
            break;

        case 2: // implicit load
            std::cout << "[" << timestamp << "] 事件：动态库隐式加载\n"
                     << "已加载库: " << e->lib_path << "\n"
                     << "依赖关系: " << e->dependent_lib << "\n\n";
            break;

        case 3: // unload
            std::cout << "[" << timestamp << "] 事件：显式库卸载\n"
                     << "调用函数: dlclose\n"
                     << "目标句柄: 0x" << std::hex << e->lib_addr << std::dec << "\n";
            if (strlen(e->lib_path) > 0) {
                std::cout << "卸载库路径: " << e->lib_path << "\n";
            }
            std::cout << "卸载结果: 成功\n\n";
            break;

        case 4: // symbol resolve
            if (e->symbol_addr == 0) {
                std::cout << "[" << timestamp << "] 事件：符号解析事件\n";
                if (strlen(e->lib_path) > 0) {
                    std::cout << "所属库: " << e->lib_path << "\n";
                }
                std::cout << "查找库句柄: 0x" << std::hex << e->lib_addr << std::dec << "\n"
                         << "请求符号: " << e->symbol_name << "\n";
            } else {
                std::cout << "解析地址: 0x" << std::hex << e->symbol_addr << std::dec << "\n\n";
            }
            break;

        case 5: // function call
            std::cout << "[" << timestamp << "] 事件：动态函数调用\n"
                     << "函数名称: " << e->symbol_name << "\n\n";
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

    std::cout << "动态库监控程序已启动...\n" << std::endl;

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