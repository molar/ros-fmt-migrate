
#ifndef OUTOFORDER_HPP_
#define OUTOFORDER_HPP_

class A {
public:
  A() : e_(1), b_(1), a_(b_) {}
  A(int k, double af) {}
  A(int b) : b_(2) {}

private:
  int a_;
  int b_;
  float kk{12};
  int c_;
  double d_{1};
  float e_;
};

class B : public A {
public:
  B() : g_(21), f_(21), A{2, 23.1} {}
  B(int a, int b) : f_(21), A{2, 23.1} {}

private:
  int f_;
  int g_{21};
};

#endif
