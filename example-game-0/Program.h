#pragma once
#include <glfw/glfw3.h>

class Program {
private:
    GLFWwindow* window;
    
public:
    Program();
    ~Program();
    
public:
    auto run() -> int;

private:
    auto terminate() -> void;
};
