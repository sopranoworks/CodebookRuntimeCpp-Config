#pragma once

#include <map>
#include <string>
#include <vector>

namespace CodebookRuntime::Config {

class Value;

using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

} // namespace CodebookRuntime::Config
