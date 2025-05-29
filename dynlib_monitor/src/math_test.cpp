#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <random>
#include <chrono>
#include <thread>

// 随机休眠一段时间（100ms到1000ms之间）
void random_sleep() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100, 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
}

int main() {
    std::cout << "开始测试动态库加载和函数调用..." << std::endl;
    
    // 显式加载 libm.so
    void* handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        std::cerr << "无法加载 libm.so: " << dlerror() << std::endl;
        return 1;
    }

    typedef double (*math_func)(double);
    
    // 定义要解析的数学函数列表
    const char* math_functions[] = {
        "cos", "sin", "tan",
        "acos", "asin", "atan",
        "sqrt", "log", "exp",
        "ceil", "floor", "round"
    };
    
    // 存储函数指针
    math_func func_ptrs[sizeof(math_functions) / sizeof(math_functions[0])] = {nullptr};
    
    // 解析所有函数
    for (size_t i = 0; i < sizeof(math_functions) / sizeof(math_functions[0]); ++i) {
        random_sleep();  // 随机延迟
        
        func_ptrs[i] = (math_func)dlsym(handle, math_functions[i]);
        if (!func_ptrs[i]) {
            std::cerr << "无法获取 " << math_functions[i] << " 函数: " << dlerror() << std::endl;
            dlclose(handle);
            return 1;
        }
        
        // 测试函数调用
        double result = func_ptrs[i](1.0);
        std::cout << math_functions[i] << "(1.0) = " << result << std::endl;
    }

    // 休眠一段时间以便观察
    std::cout << "所有函数已解析完成，等待结束..." << std::endl;
    sleep(3);

    // 关闭动态库
    dlclose(handle);
    std::cout << "测试完成" << std::endl;
    
    return 0;
} 