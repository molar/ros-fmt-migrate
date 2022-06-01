#include <ros/console.h>

int main(int argc, char **argv) {
  float j = 12;
  ROS_INFO_STREAM("Hello World " << 213 << j);
  return 0;
}
