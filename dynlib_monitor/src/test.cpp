#include <iostream>
#include <dlfcn.h>    // 动态链接库操作相关函数
#include <unistd.h>   // sleep函数
#include <random>     // 随机数生成
#include <chrono>     // 时间相关功能
#include <thread>     // 线程休眠
#include <string>     // 字符串操作

/**
 * @brief 随机休眠函数
 * 
 * 该函数使用C++11的随机数生成器和时间库，
 * 在每次调用时随机休眠100ms到1000ms之间的时间。
 * 这样可以模拟真实程序中的不规则操作间隔。
 */
void random_sleep() {
    static std::random_device rd;                          // 硬件随机数生成器
    static std::mt19937 gen(rd());                        // Mersenne Twister伪随机数生成器
    static std::uniform_int_distribution<> dis(100, 1000); // 均匀分布在100ms到1000ms之间
    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
}

/**
 * @brief 主函数
 * 
 * 测试程序，用于测试动态库监控工具的功能。
 * 程序会执行以下操作：
 * 1. 显式加载数学库libm.so
 * 2. 显式加载pthread库libpthread.so
 * 3. 显式加载crypt库libcrypt.so
 * 4. 依次解析和测试各个库中的函数
 * 5. 最后卸载所有动态库
 */
int main() {
    std::cout << "开始测试动态库加载和函数调用..." << std::endl;
    
    // 加载数学库
    void* math_handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!math_handle) {
        std::cerr << "无法加载 libm.so: " << dlerror() << std::endl;
        return 1;
    }

    // 加载pthread库
    void* pthread_handle = dlopen("libpthread.so.0", RTLD_LAZY);
    if (!pthread_handle) {
        std::cerr << "无法加载 libpthread.so: " << dlerror() << std::endl;
        dlclose(math_handle);
        return 1;
    }

    // 加载crypt库
    void* crypt_handle = dlopen("libcrypt.so.2", RTLD_LAZY);
    if (!crypt_handle) {
        std::cerr << "无法加载 libcrypt.so: " << dlerror() << std::endl;
        dlclose(math_handle);
        dlclose(pthread_handle);
        return 1;
    }

    // 定义函数指针类型
    typedef double (*math_func)(double);
    typedef int (*pthread_func)(pthread_mutex_t*);
    typedef char* (*crypt_func)(const char*, const char*);
    
    // 测试数学函数
    const char* math_functions[] = {
        "cos", "sin", "tan",       // 三角函数
        "sqrt", "log", "exp"       // 指数和对数函数
    };
    
    // 解析和测试数学函数
    for (const char* func_name : math_functions) {
        random_sleep();
        
        math_func func = (math_func)dlsym(math_handle, func_name);
        if (!func) {
            std::cerr << "无法获取 " << func_name << " 函数: " << dlerror() << std::endl;
            continue;
        }
        
        double result = func(1.0);
        std::cout << func_name << "(1.0) = " << result << std::endl;
    }

    // 测试pthread函数
    const char* pthread_functions[] = {
        "pthread_mutex_init",
        "pthread_mutex_destroy"
    };

    // 解析pthread函数
    for (const char* func_name : pthread_functions) {
        random_sleep();
        
        pthread_func func = (pthread_func)dlsym(pthread_handle, func_name);
        if (!func) {
            std::cerr << "无法获取 " << func_name << " 函数: " << dlerror() << std::endl;
            continue;
        }
        
        std::cout << "成功解析 " << func_name << " 函数" << std::endl;
    }

    // 测试crypt函数
    random_sleep();
    crypt_func crypt_fn = (crypt_func)dlsym(crypt_handle, "crypt");
    if (crypt_fn) {
        const char* result = crypt_fn("password", "salt");
        if (result) {
            std::cout << "crypt(\"password\", \"salt\") = " << result << std::endl;
        }
    }

    // 等待3秒，给用户时间观察监控结果
    std::cout << "所有函数已解析完成，等待结束..." << std::endl;
    sleep(3);

    // 卸载所有动态库
    dlclose(math_handle);
    dlclose(pthread_handle);
    //dlclose(crypt_handle);
    std::cout << "测试完成" << std::endl;
    
    return 0;
} 