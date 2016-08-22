

#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

class background_task 
{
 public:
  void operator()() const {
    std::cout << ">_<" << std::endl;
  }
};

class access_local_ref
{
 public:
  access_local_ref(int &i) : i_(i) {}
  void work()
  {
    for(int i = 0; i < 10; i++)
      std::cout << i_ << std::endl;
  }
 private:
  int &i_;
};

void oops_again();

//
// RAII thread, join in dtor
//
class scoped_thread
{
 public:
  explicit scoped_thread(std::thread t) : t_(std::move(t)) {}
  ~scoped_thread(){
    t_.join();
  }
  scoped_thread(const scoped_thread&) = delete;
  scoped_thread& operator=(const scoped_thread&) = delete;
 private:
  std::thread t_;
};

int main()
{
  background_task f;
  std::thread t1(f);
  std::thread t2{background_task()};

  // lambda
  std::thread t3([](){std::cout << "lambda" << std::endl;});

  // std::bind, don't forget & for member function
  int n = 0x10;
  access_local_ref alr(n);
  std::thread t4(std::bind(&access_local_ref::work, &alr));

  // join, wait thread exit
  t1.join();
  t2.join();
  t3.join();

  t4.join();
  //t4.detach(); // don't wait thread finished ,ERROR!, local reference to 'n' will destroyed when main thread
  //exit


  //
  //Exception safty
  //
  //This means that the call to join() is liable to
  //be skipped if an exception is thrown after the thread has been started but before the
  //call to join().
  //To avoid your application being terminated when an exception is thrown, you
  //therefore need to make a decision on what to do in this case.
  //
  //Solution: use RAII idiom(dtor will be called when leaves the scope eithor
  //normally or exception raised)
  //
  //join the thread in destructor
  //


  //
  // Parameter are copied to thread by default
  //
  //it’s important to bear in mind that by default the arguments are copied into inter-
  //nal storage, where they can be accessed by the newly created thread of execution,
  //even if the corresponding parameter in the function is expecting a reference.
  //
  //see function oops()
  //
  oops_again();

  //
  // use scoped_thread
  //
  scoped_thread(std::thread([](){std::cout << "doing something" << std::endl;}));

  //
  // use std::vector
  //
  std::vector<std::thread> threads;
  for(int i = 0; i < 10; i++) {
    threads.push_back(std::thread([](){std::cout << "working" << std::endl;}));
  }
  std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
  //std::for_each(threads.begin(), threads.end(), std::bind(&std::thread::join, std::placeholders::_1));
  return 0;
}

//
// Parameter are copied
//
void f(int i, const std::string &s)
{ }
void oops()
{
  char buffer[1024];
  //std::thread t(f, 3, buffer); // ERROR: 在buffer被转换为std::string之前,有可能oops已经return了, 所以thread有可能访问一个dangle pointer！
  std::thread t(f, 3, std::string(buffer)); 
  t.detach();
}
//
// use std::ref when you want a reference
//
void b(int &i){ i++; }
void oops_again()
{
  int n = 0x10;
  std::thread t(b, std::ref(n));
  t.join();
  std::cout << n << std::endl;
}
