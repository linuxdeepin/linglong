// Thish file is generated by /tools/run-quicktype.sh
// DO NOT EDIT IT.

// clang-format off

//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     Project.hpp data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>
#include "linglong/builder/project/helper.hpp"

#include "linglong/builder/project/Base.hpp"
#include "linglong/builder/project/Build.hpp"
#include "linglong/builder/project/Depend.hpp"
#include "linglong/builder/project/Package.hpp"
#include "linglong/builder/project/Runtime.hpp"
#include "linglong/builder/project/Source.hpp"

namespace linglong {
namespace builder {
namespace project {
/**
* Linglong project build file.
*/

using nlohmann::json;

/**
* Linglong project build file.
*/
struct Project {
std::optional<Base> base;
Build build;
std::optional<std::vector<Depend>> depends;
std::optional<std::map<std::string, std::string>> environment;
Package package;
std::optional<Runtime> runtime;
Source source;
std::optional<std::map<std::string, std::string>> variables;
std::string version;
};
}
}
}

// clang-format on
