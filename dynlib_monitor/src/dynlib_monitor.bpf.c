#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// 指定GPL许可证，eBPF程序必需
char LICENSE[] SEC("license") = "GPL";

/**
 * @brief 事件数据结构
 * 用于在内核和用户空间之间传递动态链接相关的事件信息
 */
struct event {
    __u64 timestamp;      ///< 事件发生的时间戳（纳秒）
    __u32 pid;           ///< 进程ID
    __u32 uid;           ///< 用户ID
    char comm[16];       ///< 进程名
    char lib_path[64];   ///< 动态库路径
    __u64 lib_addr;      ///< 动态库加载地址或句柄
    char symbol_name[32]; ///< 符号名称
    in event_type;      ///< 事件类型（1:加载, 2:卸载, 3:符号解析）
    int flags;           ///< dlopen的标志
    __u64 symbol_addr;   ///< 符号地址
    int result;          ///< 操作结果
};

/**
 * @brief 句柄到库路径的映射
 * 用于跟踪已加载库的句柄和对应的路径
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u64);
    __type(value, char[64]);
} handle_to_path SEC(".maps");

/**
 * @brief 事件输出缓冲区
 * 用于将事件数据从内核空间传递到用户空间
 */
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
    __uint(max_entries, 1024);
} events SEC(".maps");

/**
 * @brief 临时路径存储
 * 用于在dlopen的enter和return之间传递库路径
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, char[64]);
} temp_path SEC(".maps");

/**
 * @brief 目标进程名存储
 * 存储要监控的目标进程名
 */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, char[16]);
} target_process SEC(".maps");

/**
 * @brief 监控程序名存储
 * 存储监控程序自己的名字，用于避免自我监控
 */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, char[16]);
} monitor_process SEC(".maps");

/**
 * @brief 检查当前进程是否是目标进程
 * 
 * 通过以下步骤确定是否需要监控当前进程：
 * 1. 获取当前进程名
 * 2. 检查是否是监控程序自己（如果是则不监控）
 * 3. 检查是否有指定的目标进程
 * 4. 如果有目标进程，则比较进程名
 * 
 * @return true 如果当前进程需要被监控
 * @return false 如果当前进程不需要被监控
 */
static inline bool is_target_process(void)
{
    char comm[16];
    bpf_get_current_comm(&comm, sizeof(comm));
    
    // 首先检查是否是监控程序自己
    __u32 key = 0;
    char *monitor = bpf_map_lookup_elem(&monitor_process, &key);
    if (monitor && monitor[0] != '\0') {
        // 比较是否是监控程序
        for (int i = 0; i < sizeof(comm); i++) {
            if (monitor[i] != comm[i]) {
                break;
            }
            if (monitor[i] == '\0') {
                return false;  // 是监控程序自己，不监控
            }
        }
    }
    
    // 获取目标进程名
    char *target = bpf_map_lookup_elem(&target_process, &key);
    
    // 如果没有设置目标进程名，则监控所有进程
    if (!target || target[0] == '\0') {
        return true;
    }
    
    // 比较进程名
    for (int i = 0; i < sizeof(comm); i++) {
        if (target[i] != comm[i]) {
            return false;
        }
        if (target[i] == '\0') {
            break;
        }
    }
    return true;
}

/**
 * @brief 跟踪dlopen函数调用
 * 
 * 当进程调用dlopen加载动态库时触发此探针
 * 记录库路径和加载标志，并发送事件
 */
SEC("uprobe//usr/lib/libc.so.6:dlopen")
int BPF_KPROBE(trace_dlopen, const char *filename, int flags)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    char path[64];
    
    // 填充事件基本信息
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    // 读取并保存库路径
    bpf_probe_read_user_str(path, sizeof(path), filename);
    __u32 key = 0;
    bpf_map_update_elem(&temp_path, &key, path, BPF_ANY);
    
    bpf_probe_read_user_str(e.lib_path, sizeof(e.lib_path), filename);
    e.event_type = 1;  // 加载事件
    e.flags = flags;
    
    // 发送事件到用户空间
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

/**
 * @brief 跟踪dlopen函数返回
 * 
 * 当dlopen返回时触发此探针
 * 记录加载的库句柄，并更新句柄到路径的映射
 */
SEC("uretprobe//usr/lib/libc.so.6:dlopen")
int BPF_KRETPROBE(trace_dlopen_ret, void *retval)
{
    if (!is_target_process())
        return 0;

    __u64 handle = (__u64)retval;
    struct event e = {};
    
    // 填充事件基本信息
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = handle;
    e.event_type = 1;  // 加载事件

    // 从临时map中获取路径并更新句柄映射
    __u32 key = 0;
    char *temp = bpf_map_lookup_elem(&temp_path, &key);
    if (temp && handle != 0) {
        char path[64];
        bpf_probe_read_kernel_str(path, sizeof(path), temp);
        bpf_map_update_elem(&handle_to_path, &handle, path, BPF_ANY);
        __builtin_memcpy(e.lib_path, path, sizeof(path));
        bpf_map_delete_elem(&temp_path, &key);
    }
    
    // 发送事件到用户空间
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

/**
 * @brief 跟踪dlclose函数调用
 * 
 * 当进程调用dlclose卸载动态库时触发此探针
 * 记录被卸载的库句柄和路径
 */
SEC("uprobe//usr/lib/libc.so.6:dlclose")
int BPF_KPROBE(trace_dlclose, void *handle)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    // 填充事件基本信息
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    e.event_type = 2;  // 卸载事件

    // 查找并记录库路径
    char *path = bpf_map_lookup_elem(&handle_to_path, &e.lib_addr);
    if (path) {
        __builtin_memcpy(e.lib_path, path, sizeof(e.lib_path));
        // 清理句柄映射
        bpf_map_delete_elem(&handle_to_path, &e.lib_addr);
    }
    
    // 发送事件到用户空间
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

/**
 * @brief 跟踪dlclose函数返回
 * 
 * 为避免重复事件，这里不再发送事件
 */
SEC("uretprobe//usr/lib/libc.so.6:dlclose")
int BPF_KRETPROBE(trace_dlclose_ret, int retval)
{
    return 0;
}

/**
 * @brief 跟踪dlsym函数调用
 * 
 * 当进程调用dlsym解析符号时触发此探针
 * 记录库句柄和符号名称
 */
SEC("uprobe//usr/lib/libc.so.6:dlsym")
int BPF_KPROBE(trace_dlsym, void *handle, const char *symbol)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    // 填充事件基本信息
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.lib_addr = (__u64)handle;
    bpf_probe_read_user_str(e.symbol_name, sizeof(e.symbol_name), symbol);
    e.event_type = 3;  // 符号解析事件

    // 查找并记录库路径
    char *path = bpf_map_lookup_elem(&handle_to_path, &e.lib_addr);
    if (path) {
        __builtin_memcpy(e.lib_path, path, sizeof(e.lib_path));
    }
    
    // 发送事件到用户空间
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

/**
 * @brief 跟踪dlsym函数返回
 * 
 * 当dlsym返回时触发此探针
 * 记录解析到的符号地址
 */
SEC("uretprobe//usr/lib/libc.so.6:dlsym")
int BPF_KRETPROBE(trace_dlsym_ret, void *retval)
{
    if (!is_target_process())
        return 0;

    struct event e = {};
    
    // 填充事件基本信息
    e.timestamp = bpf_ktime_get_ns();
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&e.comm, sizeof(e.comm));
    
    e.symbol_addr = (__u64)retval;
    e.event_type = 3;  // 符号解析事件
    
    // 发送事件到用户空间
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}