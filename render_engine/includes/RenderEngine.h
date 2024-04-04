#include "interfaces/RenderEngine_Interface.h"

class RenderEngine : public RenderEngine_Interface {
  void render();
  static RenderEngine_Interface* create();
};