#include "game/pouch_order.h"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

namespace order = auto_auto_sort::game::pouch_order;

int g_failures = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                    \
        }                                                                    \
    } while (false)

// Sort key of an item: here the ID itself, except item 9 sorts before everything else.
std::uint32_t Key(std::uint32_t id) {
    return id == 9 ? 0 : id;
}

bool Sorted(std::vector<std::uint32_t> ids) {
    return order::IsSorted(std::span<const std::uint32_t>(ids), Key);
}

bool StartsWith(std::vector<std::uint32_t> bar, std::vector<std::uint32_t> expected) {
    return order::BarStartsWith(bar, expected);
}

void TestIsSorted() {
    CHECK(Sorted({}));
    CHECK(Sorted({0, 0, 0}));
    CHECK(Sorted({1, 5, 65, 174, 0, 0}));
    CHECK(Sorted({9, 1, 5}));                 // the key decides, not the ID
    CHECK(!Sorted({1, 9, 5}));
    CHECK(!Sorted({5, 1, 0}));
    CHECK(!Sorted({1, 0, 5}));                // a gap before a filled slot
    CHECK(!Sorted({0, 1}));
    CHECK(Sorted({5, 5, 6}));                 // equal keys keep their order (stable)
}

void TestBarStartsWith() {
    CHECK(StartsWith({}, {}));
    CHECK(StartsWith({1, 5, 9, 0, 0}, {1, 5, 9}));
    CHECK(StartsWith({1, 5, 9, 0x3ED, 0}, {1, 5, 9}));   // a game-added entry after the pouch items
    CHECK(StartsWith({1, 0, 5, 9}, {1, 5, 9}));           // empty entries are skipped
    CHECK(!StartsWith({5, 1, 9}, {1, 5, 9}));
    CHECK(!StartsWith({1, 5}, {1, 5, 9}));                // a new pouch item is missing
    CHECK(!StartsWith({0x3ED, 1, 5, 9}, {1, 5, 9}));
    CHECK(StartsWith({1, 5, 9}, {}));
}

void TestIdList() {
    order::IdList<3> list;
    list.Add(0);
    list.Add(7);
    list.Add(7);
    list.Add(2);
    list.Add(3);
    list.Add(4);                              // over capacity, dropped
    CHECK(list.size == 3);
    CHECK(list.ids[0] == 7 && list.ids[1] == 2 && list.ids[2] == 3);
    CHECK(list.Contains(2) && !list.Contains(4));
}

}  // namespace

int main() {
    TestIsSorted();
    TestBarStartsWith();
    TestIdList();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d failure(s)\n", g_failures);
        return 1;
    }
    std::puts("pouch_order: all checks passed");
    return 0;
}
