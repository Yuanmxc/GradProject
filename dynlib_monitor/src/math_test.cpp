#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <cmath>

int main() {
    std::cout << "开始测试动态库加载和函数调用..." << std::endl;
    
    // 显式加载 libm.so
    void* handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        std::cerr << "无法加载 libm.so: " << dlerror() << std::endl;
        return 1;
    }

    // 获取 cos 函数的地址
    typedef double (*cos_func)(double);
    cos_func cos_ptr = (cos_func)dlsym(handle, "cos");
    if (!cos_ptr) {
        std::cerr << "无法获取 cos 函数: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    // 调用动态库函数
    double result = cos_ptr(3.14159);
    std::cout << "cos(π) = " << result << std::endl;

    // 使用标准库中的其他数学函数
    double sqrt_result = sqrt(16.0);
    double log_result = log(10.0);
    
    std::cout << "sqrt(16) = " << sqrt_result << std::endl;
    std::cout << "log(10) = " << log_result << std::endl;

    // 休眠一段时间以便观察
    sleep(2);

    // 关闭动态库
    dlclose(handle);
    std::cout << "测试完成" << std::endl;
    
    return 0;
} 