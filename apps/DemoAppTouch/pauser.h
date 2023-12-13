#if !defined(__pauser_h)
#define __pauser_h

class Pauser {
private:
  bool _useTouch;
  bool _firstTime;
public:
  Pauser(bool useTouch=true) : _useTouch(useTouch), _firstTime(true) {}
  void wait();
};

#endif
