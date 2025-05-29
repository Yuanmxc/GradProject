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
    char lib_path[64];      // 进一步减小缓冲区
    __u64 lib_addr;
    char symbol_name[32];
    int event_type;
    int flags;
    char dependent_lib[64]; // 进一步减小缓冲区
    __u64 symbol_addr;
    int result;
};

// 定义 BPF map 用于存储句柄到库路径的映射
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u64);
    __type(value, char[64]);
} handle_to_path SEC(".maps");

// 定义 perf event map
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
    __uint(max_entries, 1024);
} events SEC(".maps");

// 定义 BPF map 用于存储临时路径
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, char[64]);
} temp_path SEC(".maps");

// 检查是否是目标进程
static inline bool is_target_process(void)
{
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));
    
    // 比较进程名是否为 "math_test"
    char target[] = "math_test";
    for (int i = 0; i < sizeof(target) - 1; i++) {
        if (comm[i] != target[i])
            return false;
    }
    return true;
}

// 跟踪动态库加载
SEC("uprobe//usr/lib/libc.so.6:dlopen")
int BPF_KPROBE(trace_dlopen, const char *filename, int flags)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    char path[64];
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    // 读取路径并存储到临时map中
    bpf_probe_read_user_str(path, sizeof(path), filename);
    __u32 key = 0;
    bpf_map_update_elem(&temp_path, &key, path, BPF_ANY);
    
    bpf_probe_read_user_str(e.lib_path, sizeof(e.lib_path), filename);
    e.event_type = 1;
    e.flags = flags;
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪动态库加载返回值
SEC("uretprobe//usr/lib/libc.so.6:dlopen")
int BPF_KRETPROBE(trace_dlopen_ret, void *retval)
{
    if (!is_target_process())
        return 0;

    __u64 handle = (__u64)retval;
    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = handle;
    e.event_type = 1;

    // 从临时map中获取路径并存储到handle_to_path中
    __u32 key = 0;
    char *temp = bpf_map_lookup_elem(&temp_path, &key);
    if (temp && handle != 0) {
        // 将路径存储到handle_to_path中
        char path[64];
        bpf_probe_read_kernel_str(path, sizeof(path), temp);
        bpf_map_update_elem(&handle_to_path, &handle, path, BPF_ANY);
        // 复制路径到事件中
        __builtin_memcpy(e.lib_path, path, sizeof(path));
        // 清理临时路径
        bpf_map_delete_elem(&temp_path, &key);
    }
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪动态库卸载
SEC("uprobe//usr/lib/libc.so.6:dlclose")
int BPF_KPROBE(trace_dlclose, void *handle)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    e.event_type = 3;

    // 获取库路径
    char *path = bpf_map_lookup_elem(&handle_to_path, &e.lib_addr);
    if (path) {
        // 直接复制路径
        __builtin_memcpy(e.lib_path, path, sizeof(e.lib_path));
        // 从映射中删除这个句柄
        bpf_map_delete_elem(&handle_to_path, &e.lib_addr);
    }
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪动态库卸载返回值 - 只在成功时发送事件
SEC("uretprobe//usr/lib/libc.so.6:dlclose")
int BPF_KRETPROBE(trace_dlclose_ret, int retval)
{
    // 不再发送事件，避免重复
    return 0;
}

// 跟踪符号解析
SEC("uprobe//usr/lib/libc.so.6:dlsym")
int BPF_KPROBE(trace_dlsym, void *handle, const char *symbol)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    bpf_probe_read_user_str(e.symbol_name, sizeof(e.symbol_name), symbol);
    e.event_type = 4;

    // 获取库路径
    char *path = bpf_map_lookup_elem(&handle_to_path, &e.lib_addr);
    if (path) {
        // 直接复制路径
        __builtin_memcpy(e.lib_path, path, sizeof(e.lib_path));
    }
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

// 跟踪符号解析返回值
SEC("uretprobe//usr/lib/libc.so.6:dlsym")
int BPF_KRETPROBE(trace_dlsym_ret, void *retval)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.symbol_addr = (__u64)retval;
    e.event_type = 4;  // symbol resolve (return)
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}