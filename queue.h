//
// Created by loki on 6/10/19.
//

#ifndef SUNSHINE_QUEUE_H
#define SUNSHINE_QUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>

#include "utility.h"

namespace safe {

template<class T>
class queue_t {
  using status_t = util::either_t<
    (std::is_same_v<T, bool> ||
     util::instantiation_of_v<std::unique_ptr, T> ||
     util::instantiation_of_v<std::shared_ptr, T> ||
     std::is_pointer_v<T>),
    T, std::optional<T>>;

public:
  template<class ...Args>
  void push(Args &&... args) {
    std::lock_guard lg{_lock};

    if(!_continue) {
      return;
    }

    _queue.emplace_back(std::forward<Args>(args)...);

    _cv.notify_all();
  }

  status_t pop() {
    std::unique_lock ul{_lock};

    if (!_continue) {
      return util::false_v<status_t>;
    }

    while (_queue.empty()) {
      _cv.wait(ul);

      if (!_continue) {
        return util::false_v<status_t>;
      }
    }

    auto val = std::move(_queue.front());
    _queue.erase(std::begin(_queue));

    return val;
  }

  std::vector<T> &unsafe() {
    return _queue;
  }

  void stop() {
    std::lock_guard lg{_lock};

    _continue = false;

    _cv.notify_all();
  }

  bool running() const {
    return _continue;
  }

private:

  bool _continue{true};

  std::mutex _lock;
  std::condition_variable _cv;
  std::vector<T> _queue;
};

}

#endif //SUNSHINE_QUEUE_H
