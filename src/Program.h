#pragma once
#include <GLFW/glfw3.h>

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
