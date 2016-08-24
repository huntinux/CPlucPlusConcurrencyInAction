//
// Waiting for one-off(一次性的) events with futures P76
//

//
// 好文章：http://www.tuicool.com/articles/6j2u2qa
//

//
// Returning values from background tasks
//
//You use std::async to start an asynchronous task for which you don’t need the
//result right away. Rather than giving you back a std::thread object to wait on,
//std::async returns a std::future object, which will eventually hold the return value
//of the function. When you need the value, you just call get() on the future, and the
//thread blocks until the future is ready and then returns the value. 
//可以使用std::async启动一个异步任务（你并不想立即得到它的执行结果）。std::async将返回
//一个std::futures对象，该对象持有该任务的结果，你可以使用get来得到这个结果。get会一直
//阻塞直到future变为ready最后返回执行结果。
//
// std::async第一个参数可以为std::launch::deferred or std::launch::async
//
// std::launch::deferred to indicate that the function call is to be
// deferred until either wait() or get() is called on the future, std::launch::async to
// indicate that the function must be run on its own thread, or std::launch::deferred |
// std::launch::async to indicate that the implementation may choose. This last option
// is the default. If the function call is deferred, it may never actually run. 
// defered只指直到调用wait或get时才启动任务。launch::async表示任务立即在一个新的线程上
// 运行。

//
// 总结：std::async 帮你封装好了std::promise std::packaged_task , 使用它就可以了
//

#include <future>
#include <iostream>

int find_something() {
  std::cout << "find_something threadid:"<< std::this_thread::get_id() << std::endl;
  for(int i = 0; i < 1000000; ) {
     i = i + 1;    
  }
  return 0x11;
}


int do_other_stuff() {
  std::cout << "do_other_stuff threadid:"<< std::this_thread::get_id() << std::endl;
  for(int i = 0; i < 1000000; ) {
     i = i + 1;    
  }
  return 0;
}

int main() {
  //
  // std::async
  //
  // 异步地执行一个task，通过get/wait获取执行结果
  //
  std::cout << "main threadid:"<<std::this_thread::get_id() << std::endl;
  std::future<int> answer = std::async(std::launch::async, find_something); // 在新线程上运行该task
  //std::future<int> answer = std::async(std::launch::deferred, find_something); // 在调用get/wait时执行task，不创建新线程
  do_other_stuff();
  std::cout << "The answer is:" << answer.get() << std::endl;

  //
  // std::promise 可以方便的从线程得到返回的值
  //
  std::promise<int> pr;
  std::thread t([](std::promise<int> &p){
                  p.set_value_at_thread_exit(8);
                  }, std::ref(pr)); // 注意std::ref的使用
  std::future<int> f = pr.get_future();
  auto r = f.get();
  std::cout << "result:" << r << std::endl;
  t.join();

  //
  // std::packaged_task 
  //
  // 包装了一个可调用对象（如function, lambda expression, 
  // bind expression, or another function object）,
  // 以便异步调用，它和promise在某种程度上有点像，promise保存了一个共享状态的值，而packaged_task保存的是一个函数
  //
  std::packaged_task<int()> task([](){ return 6; });
  std::thread t2(std::ref(task));
  std::future<int> f2 = task.get_future();
  auto r2 = f2.get();
  std::cout << "result:" << r2 << std::endl;
  t2.join();

  return 0;
}
