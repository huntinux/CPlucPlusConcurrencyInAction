
/**
 * 本章主要学习线程间的“通知”技术，即“条件变量”
 */

//
// The most basic mechanism for waiting for an event
// to be triggered by another thread (such as the presence of additional work in the
// pipeline mentioned previously) is the condition variable. 
// Conceptually, a condition variable is associated with some event or other condition, and one or more threads can wait
// for that condition to be satisfied. When some thread has determined that the condi-
// tion is satisfied, it can then notify one or more of the threads waiting on the condition
// variable, in order to wake them up and allow them to continue processing
//
//
// std::condition_variable and std::condition_variable_any. Both of
// these are declared in the <condition_variable>
//
// 关于Linux下条件变量的用法：http://blog.csdn.net/huntinux/article/details/51384065
//

#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

//
// 使用条件变量
//
std::queue<int> queue;
std::mutex mut;
std::condition_variable cond;

void producer() {
  while(true) {
    std::unique_lock<std::mutex> lk(mut);
    queue.push(1); 
    cond.notify_one();
  }
}

void consumer() {
  while(true) {
    std::unique_lock<std::mutex> lk(mut);
    cond.wait(lk, [](){ return !queue.empty();}); // 条件作为参数，Linux下一般是用while循环包裹住wait
    auto data = queue.front();
    queue.pop();
    lk.unlock(); // 尽量较少持锁时间
    std::cout << std::this_thread::get_id() << " get "<< data << std::endl;
  }
}

int main() {
  std::thread p(producer);
  std::vector<std::thread> consumers;
  for(int i = 0; i < 10; i++) {
    consumers.push_back(std::thread(consumer));
  }

  for(auto& c : consumers) {
    c.join();
  }
  p.join();
  return 0; 
}
