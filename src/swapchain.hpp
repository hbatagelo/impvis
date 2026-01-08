/**
 * @file swapchain.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef SWAPCHAIN_HPP_
#define SWAPCHAIN_HPP_

#include "rendertarget.hpp"

#include <array>

class SwapChain {
public:
  explicit SwapChain(
      std::vector<RenderTarget::AttachmentSpec> const &attachments);
  ~SwapChain() = default;

  SwapChain(SwapChain const &) = delete;
  SwapChain &operator=(SwapChain const &) = delete;
  SwapChain(SwapChain &&) = delete;
  SwapChain &operator=(SwapChain &&) = delete;

  void resize(glm::ivec2 size);
  void swap() noexcept;
  [[nodiscard]] RenderTarget const &back() const noexcept;
  [[nodiscard]] RenderTarget const &front() const noexcept;

private:
  std::array<RenderTarget, 2> m_targets;
  std::size_t m_backIndex{0};
};

#endif // SWAPCHAIN_HPP_