#pragma once

#include <atomic>
#include <memory_resource>
#include <utility> // For std::move

namespace zoro
{

    template <typename T>
    class leaky_shared_head_only_list
    {
    private:
        struct node
        {
            T data_;
            node *next_;

            // Construct node directly with the data
            node() = default;
            explicit node(const T &data) : data_(data), next_(nullptr) {}
            explicit node(T &&data) : data_(std::move(data)), next_(nullptr) {}
        };

    public:
        leaky_shared_head_only_list()
            : head_(nullptr), allocator_{std::pmr::get_default_resource()} {}
        explicit leaky_shared_head_only_list(std::pmr::memory_resource *resource)
            : head_(nullptr), allocator_{resource} {}

        // Disable copy semantics for simplicity
        leaky_shared_head_only_list(const leaky_shared_head_only_list &) = delete;
        leaky_shared_head_only_list &operator=(const leaky_shared_head_only_list &) = delete;

        leaky_shared_head_only_list(leaky_shared_head_only_list &&other) noexcept
        {

            node* other_head = other.head_.load(std::memory_order_acquire);

            do
            {

                head_.store(other_head, std::memory_order_release);

            } while (!other.head_.compare_exchange_weak(other_head, nullptr,
                                                        std::memory_order_release,
                                                        std::memory_order_acquire));
            allocator_ = std::move(other.allocator_);
        }
        leaky_shared_head_only_list &operator=(leaky_shared_head_only_list &&other) noexcept
        {
            if (this != &other)
            {
                node* other_head = other.head_.load(std::memory_order_acquire);
                do
                {
                    head_.store(other_head, std::memory_order_release);
                } while (!other.head_.compare_exchange_weak(other_head, nullptr,
                                                            std::memory_order_acq_rel,
                                                            std::memory_order_acquire));

                // Move the other list's head and allocator
                allocator_ = std::move(other.allocator_);
            }
            return *this;
        }

        void push_front(const T &data)
        {
            // Allocate and construct node in one step
            node *new_node = allocator_.allocate(1);
            new (new_node) node(data); // Use placement new

            // Load the current head
            node *old_head = head_.load(std::memory_order_acquire);

            // Loop until the CAS succeeds
            do
            {
                new_node->next_ = old_head;
            } while (!head_.compare_exchange_weak(old_head, new_node,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed));
        }

        // Take data by value to allow for easy move semantics
        template <typename... Args>
        void emplace_front(Args&&... args)
        {
            // Allocate and construct node in one step
            node *new_node = allocator_.allocate(1);
            new (new_node) node(std::forward<Args>(args)...); // Use placement new with move

            // Load the current head
            node *old_head = head_.load(std::memory_order_acquire);

            // Loop until the CAS succeeds
            do
            {
                new_node->next_ = old_head;
            } while (!head_.compare_exchange_weak(old_head, new_node,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed));
        }

        bool empty() const noexcept
        {
            return head_.load(std::memory_order_acquire) == nullptr;
        }

    private:
        // Allocator for node objects
        std::pmr::polymorphic_allocator<node> allocator_;

        // head_ must be an atomic pointer to a node
        std::atomic<node *> head_;
    };

}