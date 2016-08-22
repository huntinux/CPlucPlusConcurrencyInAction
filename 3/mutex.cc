
#include <list>
#include <mutex>
#include <iostream>
#include <thread>
#include <algorithm>
#include <stack>
#include <string>
#include <exception>

//
// 1.
// std::mutex
// std::lock_guard
//
// Don’t pass pointers and references to protected data outside the scope of the lock, whether by
// returning them from a function, storing them in externally visible memory, or passing them as
// arguments to user-supplied functions.
//
// 2.
// 即使每个单独的接口都是thread-safe了(使用了mutex)，也还是有潜在问题的。这是接口问题（interface-problem）,而不是
// mutex有问题。比如对于std::stack,
// 它的empty()和size()接口在函数内部是thread-safe的，但是想象一下这样的场景。thread-1
// 使用empty()发现stack不为空，但是这时有另一个线程在“空隙”率先调用了pop，然后thread-1
// pop时stack已经为空了。
//
// 一种解决方法就是，将多个单独的操作放在一起使用mutex保护起来。
//
// 但是有人提出，要注意会抛出异常的情况，比如std::stack<std::vector<int>>,
// 在std::vector的copy ctor，有可能因为分配内存时抛出std::bad_alloc异常，这会产生问题
// 如果pop()被定义为返回栈首数据，同时将它从栈中删除，将会有潜在的问题：
// 被返回的value在stack被修改后才会返回给调用者，但是copying
// data的过程可能会抛出异常。
// 如果有异常抛出，pop出来的value将会丢失，因为它已经从stack中移除了，但是copy却失败了。
//
// 所以stack的设计者将该操作分成了两步： 
// a. 获取top元素： top()
// b. 移除top元素： pop()
//
// 关于拷贝构造函数的自赋值和异常安全问题可以看这里： http://blog.csdn.net/yang20141109/article/details/50404695
// 
//
// If the pop()
// function was defined to return the value popped, as well as remove it from the stack,
// you have a potential problem: the value being popped is returned to the caller only
// after the stack has been modified, but the process of copying the data to return to the
// caller might throw an exception. If this happens, the data just popped is lost; it has
// been removed from the stack, but the copy was unsuccessful! The designers of the
// std::stack interface helpfully split the operation in two: get the top element (top())
// and then remove it from the stack (pop()), so that if you can’t safely copy the data, it
// stays on the stack. If the problem was lack of heap memory, maybe the application can
// free some memory and try again.
// Unfortunately, it’s precisely this split that you’re trying to avoid in eliminating the
// race condition! Thankfully, there are alternatives, but they aren’t without cost.
//
// 翻译一下：
//
// 如果pop()不仅返回栈顶值，而且要将该值从栈中移除。
//
// 比如这样实现的：
// value_type pop()
// {
//  value_type r = stack.top(); // 1
//  stack.remove_top();         // 2
//  return r;                   // 3
// }
//
// 那么第三步将存在潜在的问题。因为是return-by-value，对于类类型，会调用拷贝构造函数。（但是C++FAQ提到编译器有优化！）
// 如果拷贝构造函数会抛出异常，而该值已经从栈中移除了, 那么这个值会被意外丢失。
//
// 解决方法：
//  
// 1. pass by reference
//  void pop(value_type&)
//
// 2. REQUIRE A NO-THROW COPY CONSTRUCTOR OR MOVE CONSTRUCTOR
//
// 3. The third option is to return a pointer to the popped item rather than return the item by value.
// 因为以指针作为返回值不会抛出异常，呵呵。 std::shared_ptr
//
// 4. 同时使用1和2 或 1和3 以提供灵活性
//
//
// 对于死锁 deadlock p47
// the C++ Standard Library has a cure for this in the form of std::lock
// a function that can lock two or more mutexes at once without risk of deadlock
//
// 如何避免死锁
// 1. 避免重复加锁
// 2. 当持有锁的时候，避免调用用户自定义的代码
// 3. 如果有多个锁，那么以固定的顺序加锁
// 4. USE A LOCK HIERARCHY

//
// thread-safe stack
//

struct empty_stack : std::exception {
  const char* what() const throw() {
    const char* r = "empty stack";
    return r;
  }
};

template<typename T>
class ThreadSafeStack {
 public:
  ThreadSafeStack(){}
  ThreadSafeStack(const ThreadSafeStack& other) {
    std::lock_guard<std::mutex> guard(other.mutex_);
    stack_ = other.stack_;
  }

  ThreadSafeStack& operator= (const ThreadSafeStack&) = delete; 

  void push(const T& v) {
    std::lock_guard<std::mutex> guard(mutex_);
    stack_.push(v);
  }

  std::shared_ptr<T> pop() {
    std::lock_guard<std::mutex> guard(mutex_);
    if(stack_.empty()) throw empty_stack();
    std::shared_ptr<T> r = std::make_shared<T>(stack_.top());
    stack_.pop();
    return r;
  }

  void pop(T& r) {
    std::lock_guard<std::mutex> guard(mutex_);
    if(stack_.empty()) throw empty_stack();
    r = stack_.top();
    stack_.pop();
  }

  bool empty() const{
    std::lock_guard<std::mutex> guard(mutex_);
    return stack_.empty();
  }

 private:
  std::mutex mutex_;
  std::stack<T> stack_;
};

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int new_value) {
  std::lock_guard<std::mutex> guard(some_mutex);
  some_list.push_back(new_value);
}

bool list_contains(int value) {
  return std::find(some_list.begin(), some_list.end(), value) != some_list.end();
}

void add_data() {
  for(int i = 0; i < 1000000; i++)
    if(i % 2 == 0) add_to_list(i);
}

void find_data() {
  for(int i = 0; i < 1000000; i++)
    if(list_contains(i))
      std::cout << i << " contains" << std::endl;
    else 
      std::cout << i << " NOT contains" << std::endl;
}

int main()
{
  //
  // Thread safe stack
  //
  
  ThreadSafeStack<int> tss;
  tss.push(1);
  int v;
  tss.pop(v);
  std::cout << v << std::endl;

  ThreadSafeStack<std::string> tss2;
  tss2.push("helloworld");
  std::string s;
  tss2.pop(s);
  std::cout << s << std::endl;
  tss2.push("joke");
  std::shared_ptr<std::string> r = tss2.pop();
  std::cout << *r << std::endl;

  tss2.pop(); // throw exception
  return 0;

  //
  // usage of std::mutex
  //
  std::thread producer(add_data);
  std::thread consumer(find_data);

  producer.join();
  consumer.join();

  return 0;
}
