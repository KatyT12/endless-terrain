#include "Window.h"


    Window::Window(uint32_t width, uint32_t height,std::string& title)
    :m_width(width),m_height(height),m_title(title)
    {
        if (!glfwInit())
            ASSERT(false);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);  
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);   
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glViewport(0,0,width,height);

        window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    

        if (!window)
        {
            glfwTerminate();
            ASSERT(false);
        }

        
        glfwMakeContextCurrent(window);

        glfwSwapInterval(1);

        if(glewInit() != GLEW_OK)
        {
            std::cout << "Error!" << std::endl;
        }	  

        return;
        
    }

    Window::~Window()
    {
        std::cout << "Destroyed" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();

    }

    bool Window::Run(sceneRenderer* renderer)
    {
        if(glfwWindowShouldClose(window))
        {
            return true;
        }

        if(glIsEnabled(GL_DEPTH_TEST))glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        else glClear(GL_COLOR_BUFFER_BIT);
        
        renderer->DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
        return false;
    }

    GLFWwindow* Window::GetWindow()
    {
        return window;
    }


