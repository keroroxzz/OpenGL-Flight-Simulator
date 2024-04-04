
class RenderEngine_Interface {
 public:
  virtual void render() = 0;
  static RenderEngine_Interface* create();
};
