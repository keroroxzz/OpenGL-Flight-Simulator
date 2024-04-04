#include "includes/RenderEngine.h"

#include <iostream>

using namespace std;

RenderEngine_Interface* RenderEngine::create() {
  return static_cast<RenderEngine_Interface*>(new RenderEngine());
}

void RenderEngine::render() {
  printf("render!!\n");
}