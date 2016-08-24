
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

//
// 注意mutable的使用
//
// 与std::lock_guard相比，std::unique_lock 更加灵活，可以提前unlock，以减少持锁时间
//

template<typename T>
class MyThreadSafeQueue {
 public:
  MyThreadSafeQueue(){}
  MyThreadSafeQueue(const MyThreadSafeQueue& other){
    std::lock_guard<std::mutex> guard(other.mut_);
    q_ = other.q_;
  }

  void push(T v) {
    std::lock_guard<std::mutex> guard(mut_);
    q_.push(v);
    cond_.notify_one();
  }

  void wait_and_pop(T& v) {
    std::unique_lock<std::mutex> ul(mut_);
    cond_.wait(ul, [&](){return !q_.empty();});
    v = q_.front();
    q_.pop();
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> ul(mut_);
    cond_.wait(ul, [&](){return !q_.empty();});
    std::shared_ptr<T> r = std::make_shared<T>(q_.front());
    q_.pop();
    return r;
  }

  bool try_pop(T& v) {
    std::lock_guard<std::mutex> guard(mut_);
    if(q_.empty()) return false;
    v = q_.front();
    q_.pop();
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> guard(mut_);
    if(q_.empty()) return std::shared_ptr<T>();
    std::shared_ptr<T> r = (std::make_shared<T>(q_.front()));
    q_.pop();
    return r;
  }

  bool empty() const {
    std::lock_guard<std::mutex> guard(mut_);
    return q_.empty();
  }

 private:
  std::queue<T> q_;
  mutable std::mutex mut_; // mut 必须是mutable的，因为在const member function也会被修改,还有copy construction，参数为const
  std::condition_variable cond_;
};

MyThreadSafeQueue<int> mtsq;

void producer() {
  while(true) {
    mtsq.push(0xFF);
  }
}

void consumer() {
  while(true) {
    //int v = 0;
    //mtsq.wait_and_pop(v);
    //std::cout << std::this_thread::get_id() << " get " << v << std::endl;  
    auto v = mtsq.wait_and_pop();
    std::cout << std::this_thread::get_id() << " get " << *v << std::endl;  
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

  // MyThreadSafeQueue<int> mtsq;
  // mtsq.push(0x10);
  // int v = 0;
  // mtsq.wait_and_pop(v);
  // std::cout << v << std::endl;
  return 0;
}
