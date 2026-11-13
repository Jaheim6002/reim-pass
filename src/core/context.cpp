#include "headers/context.hpp"

void AppContext::initApp (){
    conf.loadConfig(
        &directory, 
        &sync, 
        &e2ePass, 
        &copyPassword, 
        &copySeconds, 
        &showPassword, 
        &genPassLenght
    );
}