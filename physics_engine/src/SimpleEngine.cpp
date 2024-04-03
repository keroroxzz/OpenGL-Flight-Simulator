#include "includes/SimpleEngine.h"
#include <iostream>

using namespace std;


iSimpleEngine* create() {
    return static_cast<iSimpleEngine*>(new SimpleEngine());
}

void SimpleEngine::test() {
    printf("Lol");
}
