#include <ros/console.h>
#include <string>

int main(int argc, char **argv) {
  float j = 12;
  ROS_INFO_STREAM("Hello World " << 213 << j);
  ROS_WARN_STREAM("asdfasdf "
                  << "Hello World " << 213.12 << j);
  ROS_INFO_STREAM(std::string("Hello World ") << 213 << j);
  using namespace std::string_literals;
  ROS_INFO_STREAM("Hello World "s << 213 << j);
  ROS_INFO_STREAM("True or " << (true ? false : true));
  return 0;
}
