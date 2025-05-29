#include <iostream>
#include <dlfcn.h>
#include <unistd.h>

int main() {
    std::cout << "开始测试动态库加载和函数调用..." << std::endl;
    
    // 显式加载 libm.so
    void* handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        std::cerr << "无法加载 libm.so: " << dlerror() << std::endl;
        return 1;
    }

    // 获取 cos 函数的地址
    typedef double (*math_func)(double);
    
    // 解析 cos 函数
    math_func cos_ptr = (math_func)dlsym(handle, "cos");
    if (!cos_ptr) {
        std::cerr << "无法获取 cos 函数: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    // 解析 sqrt 函数
    math_func sqrt_ptr = (math_func)dlsym(handle, "sqrt");
    if (!sqrt_ptr) {
        std::cerr << "无法获取 sqrt 函数: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    // 解析 log 函数
    math_func log_ptr = (math_func)dlsym(handle, "log");
    if (!log_ptr) {
        std::cerr << "无法获取 log 函数: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    // 调用动态库函数
    double cos_result = cos_ptr(3.14159);
    double sqrt_result = sqrt_ptr(16.0);
    double log_result = log_ptr(10.0);
    
    std::cout << "cos(π) = " << cos_result << std::endl;
    std::cout << "sqrt(16) = " << sqrt_result << std::endl;
    std::cout << "log(10) = " << log_result << std::endl;

    // 休眠一段时间以便观察
    sleep(2);

    // 关闭动态库
    dlclose(handle);
    std::cout << "测试完成" << std::endl;
    
    return 0;
} 