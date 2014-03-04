#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <atomic>

#include "tagged_ptr.hpp"

template <typename T>
class lqueue
{
  class node
  {
   public:
    node() noexcept : next(tagged_ptr<node>()) {}
    node(const T & v) : data(v) {
      auto old_next = next.load(std::memory_order_relaxed);
      tagged_ptr<node> new_next(nullptr, old_next.get_next_tag());
      next.store(new_next, std::memory_order_release);
    }
   private:
    std::atomic<tagged_ptr<node>> next;
    T data;
  };
};

#endif /* LOCKFREE_QUEUE_H */