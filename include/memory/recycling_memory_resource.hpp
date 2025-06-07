#pragma once

#include <memory_resource>
#include <functional>

#include "concurrency/leaky_msque.hpp"

namespace zoro
{


    class recycling_memory_resource : public std::pmr::memory_resource
    {
    public:
        recycling_memory_resource()
            : user_memory_resource_(std::pmr::get_default_resource())
        {
        }

        explicit recycling_memory_resource(std::pmr::memory_resource *upstream)
            : user_memory_resource_(upstream)
        {
        }

        // Disable copy and move semantics for simplicity
        recycling_memory_resource(const recycling_memory_resource &) = delete;
        recycling_memory_resource &operator=(const recycling_memory_resource &) = delete;

        ~recycling_memory_resource() override {
            // Clear the recycle queue
            void *memchunk;
            while (recycle_queue_.pop(memchunk))
            {
                user_memory_resource_->deallocate(memchunk, 0, 0);
            }

            memory_resource_que_.release(); // Release the memory resource

        }

        void *do_allocate(size_t bytes, size_t alignment) override
        {

            void *memchunk;
            if (recycle_queue_.pop(memchunk))
            {
                // If we have a recycled memory chunk, return it
                return memchunk;
            }

            return user_memory_resource_->allocate(bytes, alignment);
        }

        void do_deallocate(void *p, size_t bytes, size_t alignment) override
        {
            if (recycle_queue_.size() < 1024)
            {
                // If the recycle queue is not too large, push the memory chunk back to the queue
                recycle_queue_.push(p);
                return;
            }

            // If the queue is too large, deallocate the memory chunk directly
            user_memory_resource_->deallocate(p, bytes, alignment);
        }

        bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
        {
            return this == &other;
        }

    private:
        std::pmr::memory_resource* user_memory_resource_;

        std::array<std::byte, 1024> buffer_; // Buffer for memory allocation
        std::pmr::monotonic_buffer_resource memory_resource_que_{buffer_.data(), buffer_.size()};
        leaky_msque<void *> recycle_queue_{&memory_resource_que_}; // Queue for recycled memory chunks
    };

}