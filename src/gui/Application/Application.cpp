#include "Application.h"


// this function should be mutex locked if we multithread later
Application& Application::instance()
{
    static Application instance;
    return instance;
}

Application::Application()
{    
}
