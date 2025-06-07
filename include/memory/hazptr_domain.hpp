#pragma once

#include <atomic>
#include <functional>

#include "memory/hazptr_fwd.hpp"
#include "memory/hazptr_record.hpp"
#include "concurrency/leaky_shared_head_only_list.hpp"
#include "concurrency/leaky_msque.hpp"
#include "memory/recycling_memory_resource.hpp"

namespace zoro
{
    class hazptr_domain
    {
    public:
        hazptr_domain()
            : record_upstream_resource_{4096},
              record_memory_resource_{&record_upstream_resource_},
              hazard_list_{&record_memory_resource_},
              retired_upstream_resource_{},
              retired_memory_resource_{&retired_upstream_resource_},
              retired_list_{&retired_memory_resource_},
              leftovers_list_{&retired_memory_resource_},
              retired_nodes_{0},
              reclamation_in_progress_{false}

        {
        }

        // Disable copy and move semantics for simplicity
        hazptr_domain(const hazptr_domain &) = delete;
        hazptr_domain &operator=(const hazptr_domain &) = delete;

        hazptr_domain(hazptr_domain &&) = delete;
        hazptr_domain &operator=(hazptr_domain &&) = delete;

        ~hazptr_domain()
        {

            record_upstream_resource_.release(); // Release the memory resource for hazptr_record
            retired_upstream_resource_.release();
        }

    private:
        struct hazptr_retired
        {
            void *object_;
            std::function<void(void)> deleter_; // Function to reclaim the object
        };

        struct hazptr_tls
        {
            hazptr_domain &domain_;                                               // Reference to the hazptr_domain
            leaky_shared_head_only_list<hazptr_retired> tls_retired_list_;        // Thread-local list of retired objects
            std::array<hazptr_record *, 2> tls_hazard_records_{nullptr, nullptr}; // Thread-local hazardous pointers
            // Thread-local list of retired objects

            hazptr_tls(hazptr_domain &domain)
                : domain_{domain}, tls_retired_list_(&domain_.retired_memory_resource_)
            {
            }
        };

        hazptr_tls &get_tls_pool()
        {
            static thread_local hazptr_tls pool{*this};
            return pool;
        }

        std::pmr::monotonic_buffer_resource record_upstream_resource_;   // Memory resource for hazptr_record
        std::pmr::synchronized_pool_resource retired_upstream_resource_; // Memory resource for retired objects
        recycling_memory_resource record_memory_resource_;               // Memory resource
        recycling_memory_resource retired_memory_resource_;              // Memory resource for retired objects
        leaky_shared_head_only_list<hazptr_record> hazard_list_;         // List of hazardous pointers
        leaky_shared_head_only_list<hazptr_retired> retired_list_;       // shared global List of retired objects
        leaky_shared_head_only_list<hazptr_retired> leftovers_list_;     // List of previous reclaimain cycle leftover retired records
        std::atomic<uint32_t> retired_nodes_;                            // Count of retired nodes
        std::atomic<bool> reclamation_in_progress_;                      // Reclamation in progress flag
    };

} // namespace zoro
