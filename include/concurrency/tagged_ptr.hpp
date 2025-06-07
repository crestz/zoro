#pragma once

#include <cstdint>

namespace zoro
{
    template <typename T>
    class alignas(16) tagged_ptr
    {
    public:
        // Added default constructor
        tagged_ptr() noexcept : data_(nullptr), tag_(0) {}


        // Constructor with pointer and tag
        explicit tagged_ptr(T *ptr, uint64_t tag = generate_tag()) noexcept
            : data_(ptr), tag_(tag)
        {
        }

        const T *operator->() const
        {
            return data_;
        }

        T *operator->()
        {
            return data_;
        }

        const T &operator*() const
        {
            return *data_;
        }

        T &operator*()
        {
            return *data_;
        }

        bool operator==(const tagged_ptr &other) const noexcept
        {
            return data_ == other.data_ && tag_ == other.tag_;
        }

        bool operator!=(const tagged_ptr &other) const noexcept
        {
            return !(*this == other);
        }

        T *raw() const { return data_; }
        uint64_t tag() const { return tag_; }

    private:
        static uint64_t generate_tag() noexcept
        {
            static thread_local uint64_t tag_counter_{0};
            return tag_counter_++;
        }

        T *data_;
        uint64_t tag_;
    };
}