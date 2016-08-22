// g++ *.cc -std=c++11 -pthread

#include <iostream>
#include <thread>

void hello()
{
  std::cout << "Hello World" << std::endl;
}

int main()
{
  std::thread t(hello);
  t.join();
  return 0;
}
