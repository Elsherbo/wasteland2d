#include "ServiceLocator.h"

namespace engine {

std::mutex ServiceLocator::mutex_;
std::unordered_map<std::type_index, void*> ServiceLocator::services_;

} // namespace engine
