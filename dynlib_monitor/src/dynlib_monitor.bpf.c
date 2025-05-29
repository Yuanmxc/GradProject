#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "GPL";

// 定义数据结构
struct event {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char comm[16];
    char lib_path[256];
    __u64 lib_addr;
    char symbol_name[64];
    int event_type;  // 1: load, 2: unload, 3: symbol_resolve, 4: function_call
};

// 定义 BPF map
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
    __uint(max_entries, 1024);
} events SEC(".maps");

// 检查是否是目标进程
static inline bool is_target_process(void)
{
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));
    
    // 比较进程名是否为 "math_test"
    char target[] = "math_test";
    for (int i = 0; i < sizeof(target); i++) {
        if (comm[i] != target[i])
            return false;
    }
    return true;
}

// 跟踪动态库加载
SEC("uprobe//usr/lib/libc.so.6:dlopen")
int BPF_KPROBE(trace_dlopen, const char *filename, int flags)
{
    // 只处理目标进程
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    bpf_probe_read_user_str(e.lib_path, sizeof(e.lib_path), filename);
    e.event_type = 1;
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪动态库卸载
SEC("uprobe//usr/lib/libc.so.6:dlclose")
int BPF_KPROBE(trace_dlclose, void *handle)
{
    // 只处理目标进程
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    e.event_type = 2;
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪符号解析
SEC("uprobe//usr/lib/libc.so.6:dlsym")
int BPF_KPROBE(trace_dlsym, void *handle, const char *symbol)
{
    // 只处理目标进程
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    bpf_probe_read_user_str(e.symbol_name, sizeof(e.symbol_name), symbol);
    e.event_type = 3;
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
} 