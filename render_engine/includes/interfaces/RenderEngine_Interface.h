
class iRenderEngine {
 public:
  virtual void render() = 0;
  static iRenderEngine* create();
};
