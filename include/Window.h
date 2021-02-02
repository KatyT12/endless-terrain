#pragma once
#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "renderer.h"
#include "sceneRenderer.h"

    class Window
    {
        public:
            Window(uint32_t width,uint32_t height,std::string& title);
            Window();
            ~Window();
            GLFWwindow* GetWindow();
            bool Run(sceneRenderer* renderer);
        
        private:
            GLFWwindow* window;
            
            const uint32_t m_width;
            const uint32_t m_height;
            const std::string m_title;
            
    };