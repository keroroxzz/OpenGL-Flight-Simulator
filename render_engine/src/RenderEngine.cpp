#include "includes/RenderEngine.h"

#include <iostream>

using namespace std;

iRenderEngine* iRenderEngine::create() {
  return static_cast<iRenderEngine*>(new RenderEngine());
}

void RenderEngine::render() {
  printf("render!!\n");
}