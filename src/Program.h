#pragma once
#include <GLFW/glfw3.h>
#include <d3d11.h>
#include <dxgi1_2.h>

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
