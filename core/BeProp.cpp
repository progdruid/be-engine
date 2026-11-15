#include "BeProp.h"

auto BePropSurface::AllocateId() -> uint32_t {
    static uint32_t nextId = 0;
    return nextId++;
}
