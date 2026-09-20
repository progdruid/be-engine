#pragma once
#include <cstdint>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-libassert.h>

template <typename Entry, typename Handle>
class SenSlotMap {

    hide struct Slot {
        Entry Value;
        uint32_t Generation = 0;
        bool Alive = false;
    };

    hide std::vector<Slot> _slots;
    hide std::vector<uint32_t> _free;

    expose auto Create(Entry value) -> Handle {
        uint32_t index = 0;
        if (!_free.empty()) {
            index = _free.back();
            _free.pop_back();
        }
        else {
            index = static_cast<uint32_t>(_slots.size());
            _slots.emplace_back();
        }

        auto& slot = _slots[index];
        ++slot.Generation;
        if (slot.Generation == 0) {
            slot.Generation = 1;
        }
        slot.Value = std::move(value);
        slot.Alive = true;

        return Handle { index, slot.Generation };
    }

    expose auto Get(Handle handle) -> Entry& {
        be_assert(Contains(handle), "SenSlotMap: stale handle", handle.Index, handle.Generation);
        return _slots[handle.Index].Value;
    }

    expose auto Contains(Handle handle) const -> bool {
        if (handle.Index >= _slots.size()) {
            return false;
        }
        const auto& slot = _slots[handle.Index];
        return slot.Alive && slot.Generation == handle.Generation;
    }

    expose auto Destroy(Handle handle) -> void {
        be_assert(Contains(handle), "SenSlotMap: stale handle", handle.Index, handle.Generation);
        auto& slot = _slots[handle.Index];
        slot.Value = {};
        slot.Alive = false;
        _free.push_back(handle.Index);
    }

    expose auto GetLiveHandles() const -> std::vector<Handle> {
        auto handles = std::vector<Handle>();
        for (uint32_t index = 0; index < _slots.size(); ++index) {
            if (_slots[index].Alive) {
                handles.push_back(Handle { index, _slots[index].Generation });
            }
        }
        return handles;
    }
};
