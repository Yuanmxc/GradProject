#include <iostream>
#include <dlfcn.h>    // 动态链接库操作相关函数
#include <unistd.h>   // sleep函数
#include <random>     // 随机数生成
#include <chrono>     // 时间相关功能
#include <thread>     // 线程休眠

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
 * 2. 依次解析多个数学函数
 * 3. 测试每个解析到的函数
 * 4. 最后卸载动态库
 * 
 * 在整个过程中会添加随机延迟，以便更好地观察监控效果
 */
int main() {
    std::cout << "开始测试动态库加载和函数调用..." << std::endl;
    
    // 显式加载数学库
    // RTLD_LAZY表示延迟绑定，只在实际调用函数时才解析符号
    void* handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        std::cerr << "无法加载 libm.so: " << dlerror() << std::endl;
        return 1;
    }

    // 定义函数指针类型，用于存储解析到的数学函数
    typedef double (*math_func)(double);
    
    // 要测试的数学函数列表
    const char* math_functions[] = {
        "cos", "sin", "tan",       // 三角函数
        "acos", "asin", "atan",    // 反三角函数
        "sqrt", "log", "exp",      // 指数和对数函数
        "ceil", "floor", "round"   // 取整函数
    };
    
    // 创建函数指针数组，用于存储解析到的函数地址
    math_func func_ptrs[sizeof(math_functions) / sizeof(math_functions[0])] = {nullptr};
    
    // 依次解析每个数学函数
    for (size_t i = 0; i < sizeof(math_functions) / sizeof(math_functions[0]); ++i) {
        // 添加随机延迟，使监控效果更明显
        random_sleep();
        
        // 解析函数符号
        func_ptrs[i] = (math_func)dlsym(handle, math_functions[i]);
        if (!func_ptrs[i]) {
            std::cerr << "无法获取 " << math_functions[i] << " 函数: " << dlerror() << std::endl;
            dlclose(handle);
            return 1;
        }
        
        // 测试解析到的函数，传入参数1.0进行计算
        double result = func_ptrs[i](1.0);
        std::cout << math_functions[i] << "(1.0) = " << result << std::endl;
    }

    // 等待3秒，给用户时间观察监控结果
    std::cout << "所有函数已解析完成，等待结束..." << std::endl;
    sleep(3);

    // 卸载动态库，释放资源
    dlclose(handle);
    std::cout << "测试完成" << std::endl;
    
    return 0;
} 