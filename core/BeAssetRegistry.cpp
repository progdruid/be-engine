#include "BeAssetRegistry.h"

BeAssetRegistry::BeAssetRegistry()  = default;
BeAssetRegistry::~BeAssetRegistry() {
    _props.clear();
    _materials.clear();
    _textures.clear();
}
