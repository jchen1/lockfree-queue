#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <atomic>   /* std::atomic */
#include <cstddef>  /* std::size_t */
#include <limits>   /* std::numeric_limits */
#include <utility>  /* std::pair, std::make_pair */

#include "tagged_ptr.hpp"

template <typename T>
class lqueue
{
 public:
  class node
  {
   public:
    node() noexcept : next(nullptr) {}
    node(const T & v) : data(v)
    {
      auto old_next = next.load(std::memory_order_relaxed);
      tagged_ptr<node> new_next(nullptr, old_next.next_tag());
      next.store(new_next, std::memory_order_release);
    }

    std::atomic<tagged_ptr<node>> next;
    T data;
  };

  lqueue()
  {
    tagged_ptr<node> dummy{};
    head = dummy;
    tail = dummy;
  }

  bool push(const T & data)
  {
    node* n = new node(data);
    for (;;)
    {
      auto old_tail = tail.load(std::memory_order_acquire);
      auto old_next = tail->next.load(std::memory_order_acquire);
      if (old_tail == tail.load(std::memory_order_acquire) && !old_next)
      {
        tagged_ptr<node> new_next(n, old_next.next_tag());
        if (tail->next.compare_exchange_weak(old_next, new_next))
        {
          tagged_ptr<node> new_tail(n, old_tail.next_tag());
          tail.compare_exchange_strong(old_tail, new_tail);
        }
      }
      else
      {
        tagged_ptr<node> new_tail(old_next.get_ptr(), old_tail.next_tag());
        tail.compare_exchange_strong(old_tail, new_tail);
      }
    }
  }

  bool pop(T & ret)
  {
    tagged_ptr<node> old_head;
    for (;;)
    {
      old_head = head.load(std::memory_order_acquire);
      auto old_tail = tail.load(std::memory_order_acquire);
      auto old_next = old_head->next.load(std::memory_order_acquire);
      if (old_head == head.load(std::memory_order_acquire))
      {
        if (old_head == old_tail)
        {
          if (!old_next)
          {
            return false;   //empty
          }
          else
          {
            tagged_ptr<node> new_tail(old_next.get_ptr(), old_tail.next_tag());
            tail.compare_exchange_strong(old_tail, new_tail);
          }
        }
        else
        {
          ret = *old_next;
          tagged_ptr<node> new_head(old_next.get_ptr(), old_head.next_tag());
          if (head.compare_exchange_strong(old_head, new_head))
          {
            break;
          }
        }
      }
    }
    if (old_head)
    {
      delete old_head.get_ptr();
    }
    return true;
  }

 private:
  std::atomic<tagged_ptr<node>> head, tail;
};

#endif /* LOCKFREE_QUEUE_H */