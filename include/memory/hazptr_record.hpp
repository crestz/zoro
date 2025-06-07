#pragma once

#include <atomic>

namespace zoro
{

    struct hazptr_record
    {
        std::atomic<void *> hazard;
        std::atomic<bool> active;

        hazptr_record() : hazard(nullptr), active(false) {}

        hazptr_record(const hazptr_record &) = delete;
        hazptr_record &operator=(const hazptr_record &) = delete;

        hazptr_record(hazptr_record &&) noexcept = delete;
        hazptr_record &operator=(hazptr_record &&) noexcept = delete;
    };

}