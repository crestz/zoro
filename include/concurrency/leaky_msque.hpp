#pragma once

#include <atomic>
#include <memory_resource>

#include "concurrency/tagged_ptr.hpp" // Assuming TaggedPtr is in this file

namespace zoro
{
    // Michael Scott's Concurrent Queue that Leaks Memory
    template <typename T>
    class leaky_msque
    {
        struct node;                               // 1. Forward-declare node
        using tagged_ptr = zoro::tagged_ptr<node>; // 2. Define the alias
    public:
        leaky_msque()
            : allocator_(std::pmr::get_default_resource()), head_(), tail_()
        {
            node *dummy = allocator_.allocate(1);
            new (dummy) node();
            tagged_ptr dummy_ptr(dummy);
            head_.store(dummy_ptr, std::memory_order_relaxed);
            tail_.store(dummy_ptr, std::memory_order_relaxed);
        }

        explicit leaky_msque(std::pmr::memory_resource *mem_resource)
            : allocator_(mem_resource), head_(), tail_()
        {
            node *dummy = allocator_.allocate(1);
            new (dummy) node();
            tagged_ptr dummy_ptr(dummy);
            head_.store(dummy_ptr, std::memory_order_relaxed);
            tail_.store(dummy_ptr, std::memory_order_relaxed);
        }

        // Disable copy and move semantics for simplicity
        leaky_msque(const leaky_msque &) = delete;
        leaky_msque &operator=(const leaky_msque &) = delete;

        leaky_msque(leaky_msque &&) = delete;
        leaky_msque &operator=(leaky_msque &&) = delete;

        ~leaky_msque()
        {
            T temp;
            while (pop(temp))
            {
            }
            // Dequeue all elements
        }

        void push(const T &data)
        {
            node *new_node_ptr = allocator_.allocate(1);
            new (new_node_ptr) node(data);
            tagged_ptr new_node(new_node_ptr);

            while (true)
            {
                tagged_ptr old_tail = tail_.load(std::memory_order_acquire);
                tagged_ptr next = old_tail->next_.load(std::memory_order_acquire);

                if (old_tail == tail_.load(std::memory_order_acquire))
                {
                    if (next.raw() == nullptr)
                    {
                        if (old_tail->next_.compare_exchange_strong(next, new_node,
                                                                    std::memory_order_release, std::memory_order_relaxed))
                        {
                            tail_.compare_exchange_strong(old_tail, new_node,
                                                          std::memory_order_release, std::memory_order_relaxed);
                            size_.fetch_add(1, std::memory_order_release);
                            return;
                        }
                    }
                    else
                    {
                        tail_.compare_exchange_strong(old_tail, tagged_ptr(next.raw(), next.tag() + 1),
                                                      std::memory_order_release, std::memory_order_relaxed);
                    }
                }
            }
        }

        bool pop(T &data)
        {
            while (true)
            {
                tagged_ptr old_head = head_.load(std::memory_order_acquire);
                tagged_ptr old_tail = tail_.load(std::memory_order_acquire);
                tagged_ptr next = old_head->next_.load(std::memory_order_acquire);

                if (old_head == head_.load(std::memory_order_acquire))
                {
                    if (old_head.raw() == old_tail.raw())
                    {
                        if (next.raw() == nullptr)
                        {
                            return false; // Queue is empty
                        }
                        // Tail is lagging, try to advance it
                        tail_.compare_exchange_strong(old_tail, next,
                                                      std::memory_order_release, std::memory_order_relaxed);
                    }
                    else
                    {
                        // Try to swing head to the next node
                        if (head_.compare_exchange_strong(old_head, next,
                                                          std::memory_order_release, std::memory_order_relaxed))
                        {
                            data = std::move(next->data_);
                            size_.fetch_sub(1, std::memory_order_release);
                            return true;
                        }
                    }
                }
            }
        }

        bool empty() const noexcept
        {
            return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
        }

        size_t size() const noexcept
        {
            return size_.load(std::memory_order_acquire);
        }

    private:
        // 3. Define the full node struct using the alias
        struct node
        {
            T data_;
            std::atomic<tagged_ptr> next_;

            node() : next_(tagged_ptr(nullptr)) {}
            explicit node(const T &data) : data_(data), next_(tagged_ptr(nullptr)) {}
        };

        std::pmr::polymorphic_allocator<node> allocator_;
        std::atomic<tagged_ptr> head_;
        std::atomic<tagged_ptr> tail_;
        std::atomic<std::size_t> size_{0}; // Optional: to keep track of the size of the queue
    };
}