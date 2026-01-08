/**
 * @file swapchain.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "swapchain.hpp"

SwapChain::SwapChain(
    std::vector<RenderTarget::AttachmentSpec> const &attachments)
    : m_targets(std::array<RenderTarget, 2>{RenderTarget{attachments},
                                            RenderTarget{attachments}}) {}

void SwapChain::resize(glm::ivec2 size) {
  if (m_targets[0].getSize() == size) {
    return;
  }

  m_targets[0].resize(size);
  m_targets[1].resize(size);
}

void SwapChain::swap() noexcept { m_backIndex = 1 - m_backIndex; }

RenderTarget const &SwapChain::back() const noexcept {
  return m_targets[m_backIndex];
}

RenderTarget const &SwapChain::front() const noexcept {
  return m_targets[1 - m_backIndex];
}
