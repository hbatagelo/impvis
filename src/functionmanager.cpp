/**
 * @file functionmanager.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "functionmanager.hpp"

#include "util.hpp"

#include <filesystem>
#include <set>

#include <cppitertools/itertools.hpp>
#include <fmt/core.h>
#include <gsl/gsl>
#include <toml.hpp>

namespace {

std::vector<Function> loadCatalog(toml::table const &table) {
  std::vector<Function> functions;

  Function::Data data;

  // Iterate over an array of tables of parameters and load the parameters into
  // data
  auto loadParameters{[&data](toml::array const *paramArray) {
    for (auto const &paramTable : *paramArray) {
      if (!paramTable.is_table()) {
        continue;
      }

      Function::Parameter parameter;

      // Read parameter name and value
      for (auto &&[tableKey, tableValue] : *paramTable.as_table()) {
        if (tableKey.str() == "name") {
          parameter.name = tableValue.value_or("");
        } else if (tableKey.str() == "value") {
          parameter.value = tableValue.value_or(0.0f);
        }
      }

      if (!parameter.name.empty()) {
        data.parameters.push_back(std::move(parameter));
      }
    }
  }};

  for (auto &&[rootKey, rootValue] : table) {
    // Ignore top-level keys with values, such as the 'title' key
    if (rootValue.is_value()) {
      continue;
    }

    data = {};
    auto const &subTable{table[rootKey]};
    data.name = subTable["name"].value_or(data.name);
    data.thumbnail = subTable["thumbnail"].value_or(data.thumbnail);
    data.expression = subTable["expression"].value_or(data.expression);
    data.codeLocal = subTable["code_local"].value_or(data.codeLocal);
    data.codeGlobal = subTable["code_global"].value_or(data.codeGlobal);
    data.comment = subTable["comment"].value_or(data.comment);
    data.boundsShape = subTable["bounds_shape"].value_or(data.boundsShape);
    data.boundsRadius = subTable["bounds_radius"].value_or(data.boundsRadius);
    data.isosurfaceRaymarchMethod =
        subTable["isosurface_raymarch_method"].value_or(
            data.isosurfaceRaymarchMethod);
    data.isosurfaceRaymarchSteps =
        subTable["isosurface_raymarch_steps"].value_or(
            data.isosurfaceRaymarchSteps);
    data.isosurfaceRaymarchRootTest =
        subTable["isosurface_raymarch_root_test"].value_or(
            data.isosurfaceRaymarchRootTest);
    data.isosurfaceRaymarchGradientEvaluation =
        subTable["isosurface_raymarch_gradient"].value_or(
            data.isosurfaceRaymarchGradientEvaluation);
    data.scale = subTable["scale"].value_or(data.scale);
    data.dvrRaymarchSteps =
        subTable["dvr_raymarch_steps"].value_or(data.dvrRaymarchSteps);
    data.dvrFalloff = subTable["dvr_falloff"].value_or(data.dvrFalloff);
    data.gaussianCurvatureFalloff =
        subTable["gaussian_curvature_falloff"].value_or(
            data.gaussianCurvatureFalloff);
    data.meanCurvatureFalloff =
        subTable["mean_curvature_falloff"].value_or(data.meanCurvatureFalloff);
    data.maxAbsCurvatureFalloff =
        subTable["max_abs_curvature_falloff"].value_or(
            data.maxAbsCurvatureFalloff);
    data.normalLengthFalloff =
        subTable["normal_length_falloff"].value_or(data.normalLengthFalloff);
    if (subTable["parameters"].is_array_of_tables()) {
      loadParameters(subTable["parameters"].as_array());
    }

    if (!data.expression.empty()) {
      functions.emplace_back(data);
    }
  }

  return functions;
}

} // namespace

FunctionManager::~FunctionManager() { onDestroy(); }

void FunctionManager::loadFromDirectory(std::filesystem::path const &path) {
  onDestroy();

  std::set<std::string> functionFilenames;
  for (auto const &entry : std::filesystem::directory_iterator{path}) {
    if (entry.is_regular_file() && entry.path().extension() == ".toml") {
      functionFilenames.insert(entry.path().string());
    }
  }

  for (auto const &filename : functionFilenames) {
    try {
      toml::table table = toml::parse_file(filename);
      m_groups.push_back(
          {table["title"].value_or("Undefined"), loadCatalog(table)});
    } catch (toml::parse_error const &exception) {
      fmt::print(stderr, "Error parsing file '{}'\n{} (line {}, column {})\n",
                 exception.source().path->c_str(), exception.description(),
                 exception.source().begin.line,
                 exception.source().begin.column);
    }
  }

  onCreate();
}

void FunctionManager::addUserDefined(Function const &function) {
  auto it{std::ranges::find_if(m_groups, [](auto const &group) {
    return group.name == kUserDefinedGroupName;
  })};
  if (it == m_groups.end()) {
    m_groups.emplace_back(std::string{kUserDefinedGroupName},
                          std::vector{function});
    return;
  }
  auto &functions{it->functions};
  if (functions.empty()) {
    functions.push_back(function);
  } else {
    functions.front() = function;
  }
}

std::optional<FunctionManager::FunctionId>
FunctionManager::getId(std::string_view name) const {
  auto const lowerName{ivUtil::toLower(name)};

  for (auto &&[group_index, group] : iter::enumerate(m_groups)) {
    for (auto &&[function_index, function] : iter::enumerate(group.functions)) {
      if (ivUtil::toLower(function.getData().name) == lowerName) {
        return std::optional{
            FunctionId{.group = group_index, .index = function_index}};
      }
    }
  }

  return std::nullopt;
}

std::optional<Function> FunctionManager::getFunction(FunctionId id) const {
  if (id.group >= m_groups.size()) {
    return std::nullopt;
  }

  auto const &functions{m_groups[id.group].functions};
  if (id.index >= functions.size()) {
    return std::nullopt;
  }

  return functions[id.index];
}

std::vector<FunctionManager::FunctionGroup> const &
FunctionManager::getGroups() const noexcept {
  return m_groups;
}

void FunctionManager::onCreate() {
  for (auto &group : m_groups) {
    for (auto &function : group.functions) {
      function.onCreate();
    }
  }
}

void FunctionManager::onDestroy() {
  for (auto &group : m_groups) {
    for (auto &function : group.functions) {
      function.onDestroy();
    }
  }
  m_groups.clear();
}