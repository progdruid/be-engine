#include "BeProp.h"

#include "BeBackend.h"

BeProp::~BeProp() {
    if (Mesh.IsValid()) {
        BeBackend::ReleaseMesh(Mesh);
    }
}

auto BePropSurface::AllocateId() -> uint32_t {
    static uint32_t nextId = 0;
    return nextId++;
}
