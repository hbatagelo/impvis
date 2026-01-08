/**
 * @file functionmanager.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef FUNCTIONMANAGER_HPP_
#define FUNCTIONMANAGER_HPP_

#include "function.hpp"

#include <optional>
#include <string>
#include <vector>

class FunctionManager {
public:
  struct FunctionGroup {
    std::string name;
    std::vector<Function> functions;
  };

  struct FunctionId {
    std::size_t group{};
    std::size_t index{};
  };

  FunctionManager() = default;
  ~FunctionManager();

  FunctionManager(FunctionManager const &) = delete;
  FunctionManager &operator=(FunctionManager const &) = delete;
  FunctionManager(FunctionManager &&) = delete;
  FunctionManager &operator=(FunctionManager &&) = delete;

  void loadFromDirectory(std::string_view directory);
  void addUserDefined(Function const &function);

  [[nodiscard]] std::optional<FunctionId> getId(std::string_view name) const;
  [[nodiscard]] std::optional<Function> getFunction(FunctionId id) const;
  [[nodiscard]] std::vector<FunctionGroup> const &getGroups() const noexcept;

private:
  static constexpr std::string_view kUserDefinedGroupName = "User-defined";

  std::vector<FunctionGroup> m_groups;

  void onCreate();
  void onDestroy();
};

#endif