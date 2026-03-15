/*
 * Common.h
 *
 * This file is part of the CodebookRuntimeCpp-Config project.
 *
 * Copyright (c) 2026 Sopranoworks, Osamu Takahashi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace CodebookRuntime::Config {

class Value;

using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

} // namespace CodebookRuntime::Config
