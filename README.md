# 🧠 zoro-hazptr: A Lightweight Hazard Pointer Memory Reclamation Library

A minimal, production-oriented C++17 hazard pointer implementation for safe memory reclamation in lock-free data structures.

---

## ✨ Features

- 🔒 **Safe memory reclamation** for lock-free structures.
- ⚡ **Thread-local caching** of hazard pointers.
- 🔁 **Threshold-based opportunistic reclamation** (no background threads).
- 🧹 **Leader election** using atomic flag for serialized reclamation.
- 💥 **Lock-free global retired list** using `leaky_shared_head_only_list`.
- 🪣 **Deferred leftovers list** for failed reclaim attempts.
- 🧠 Backed by a custom `recycling_memory_resource`.

---

## 📦 Components

### 1. `hazptr_domain`
- Central domain managing all hazard pointers.
- Maintains global retired lists and handles thread coordination.

### 2. `hazptr_record`
- Thread-local hazard pointer wrapper.
- Uses fixed-sized cache and TLS for fast access.

### 3. `leaky_shared_head_only_list`
- Lock-free singly-linked list supporting thread-safe pushes and atomic steals.

### 4. `recycling_memory_resource`
- Custom allocator reusing previously reclaimed memory blocks.

---

## 🔧 Reclamation Heuristics

- When `retired_nodes_ >= 64`, the current thread *attempts* to reclaim.
- Leader elected via atomic CAS on `reclamation_in_progress_`.
- Reclaimer **steals** global list using an atomic exchange.
- Unreclaimed nodes are deferred to a **secondary leftovers list**, to be retried later.
- All logic is lock-free and contention-aware.

---

## ⚙️ Example Usage

```cpp
#include "hazptr_domain.hpp"

struct MyNode {
  int value;
  MyNode* next;
};

auto* node = new MyNode{42, nullptr};

// Use hazptr_record to protect
zoro::hazptr_record rec;
rec.protect(node);

// Safe access...
int val = node->value;

// Later retire
rec.retire(node, [](void* ptr) {
  delete static_cast<MyNode*>(ptr);
});
