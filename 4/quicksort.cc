#include <iostream>
#include <list>
#include <algorithm>
#include <future>


template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
  if(input.empty()) {
    return input;
  }

  std::list<T> result;
  result.splice(result.begin(), input, input.begin()); // 将input的第一个元素移动到result中作为pivot
  T const& pivot = *result.begin();

  // 使用pivot进行一趟快排
  auto divide_point = std::partition(input.begin(), input.end(), [&](T const& t){
                                                                     return t < pivot;
                                                                     });
  // 将pivot的左半部分移动到lower_part中, 右半部分在input中
  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

  // 对两部分分别进行排序
  auto new_lower(sequential_quick_sort(std::move(lower_part)));
  auto new_higher(sequential_quick_sort(std::move(input)));

  // 最后拼接得到结果
  result.splice(result.end(), new_higher);
  result.splice(result.begin(), new_lower);
  return result; 
}

//
// 并行
//
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
  if(input.empty()) {
    return input;
  }

  std::list<T> result;
  result.splice(result.begin(), input, input.begin()); // 将input的第一个元素移动到result中作为pivot
  T const& pivot = *result.begin();

  // 使用pivot进行一趟快排
  auto divide_point = std::partition(input.begin(), input.end(), [&](T const& t){
                                                                     return t < pivot;
                                                                     });
  // 将pivot的左半部分移动到lower_part中, 右半部分在input中
  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

  // 对两部分分别进行排序
  //auto new_lower(sequential_quick_sort(std::move(lower_part)));
  //auto new_higher(sequential_quick_sort(std::move(input)));
  std::future<std::list<T>> new_lower = 
      std::async(std::launch::async, &parallel_quick_sort<T>, std::move(lower_part));
  auto new_higher(sequential_quick_sort(std::move(input)));

  // 最后拼接得到结果
  result.splice(result.end(), new_higher);
  result.splice(result.begin(), new_lower.get());
  return result; 
}

int main() {
  std::list<int> l{
    10,6,1,6,8,22,4,94,28,74
  };
  //std::list<int> r = sequential_quick_sort(l);
  std::list<int> r = parallel_quick_sort(l);
  for_each(r.begin(), r.end(), [](int v){
            std::cout << v << " "<< std::endl;
           });
  return 0;
}
