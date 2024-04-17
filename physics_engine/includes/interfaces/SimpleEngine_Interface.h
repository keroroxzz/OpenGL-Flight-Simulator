
class iSimpleEngine {
 public:
  virtual void test() = 0;
  static iSimpleEngine* create();
};