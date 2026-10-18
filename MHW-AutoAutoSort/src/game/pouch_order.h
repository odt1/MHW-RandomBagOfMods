#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace auto_auto_sort::game::pouch_order {

// True when the game's Auto-sort would leave this pouch area unchanged. Its prepare step
// (sub_14135D7D0) inserts each filled slot after every slot whose sort key is not greater, so the
// result is a stable ascending sort by key with the empty slots (ID 0) last.
template <typename KeyFn>
bool IsSorted(std::span<const std::uint32_t> ids, KeyFn&& key) {
    bool seen_empty = false;
    bool have_previous = false;
    std::uint32_t previous = 0;
    for (const std::uint32_t id : ids) {
        if (id == 0) {
            seen_empty = true;
            continue;
        }
        if (seen_empty) {
            return false;
        }
        const std::uint32_t current = key(id);
        if (have_previous && current < previous) {
            return false;
        }
        previous = current;
        have_previous = true;
    }
    return true;
}

// The bar's filled entries, in order, begin with `expected`. Entries after it (ones the game adds
// itself) and empty entries are ignored.
inline bool BarStartsWith(std::span<const std::uint32_t> bar, std::span<const std::uint32_t> expected) {
    std::size_t matched = 0;
    for (const std::uint32_t id : bar) {
        if (matched == expected.size()) {
            return true;
        }
        if (id == 0) {
            continue;
        }
        if (id != expected[matched]) {
            return false;
        }
        ++matched;
    }
    return matched == expected.size();
}

// Fixed-capacity list of distinct, nonzero item IDs in insertion order.
template <std::size_t Capacity>
struct IdList {
    std::uint32_t ids[Capacity]{};
    std::size_t size = 0;

    bool Contains(std::uint32_t id) const {
        return std::find(ids, ids + size, id) != ids + size;
    }
    void Add(std::uint32_t id) {
        if (id != 0 && size != Capacity && !Contains(id)) {
            ids[size++] = id;
        }
    }
    std::span<const std::uint32_t> View() const { return {ids, size}; }
};

}  // namespace auto_auto_sort::game::pouch_order
