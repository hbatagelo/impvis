/**
 * @file geometry.cpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#include "geometry.hpp"

namespace {

struct Frame {
  glm::vec3 direction;
  glm::vec3 tangent;
  glm::vec3 bitangent;
};

[[nodiscard]] Frame computeFrame(glm::vec3 direction) {
  auto const arbitrary{(std::abs(direction.x) < 0.9f) ? glm::vec3(1, 0, 0)
                                                      : glm::vec3(0, 1, 0)};
  auto const tangent{glm::normalize(glm::cross(direction, arbitrary))};
  auto const bitangent{glm::cross(direction, tangent)};
  return {direction, tangent, bitangent};
}

void addCap(std::vector<geometry::Vertex> &vertices,
            std::vector<GLuint> &indices, glm::vec3 center, Frame const &frame,
            float radius, unsigned int segments, bool flip) {
  auto const centerIndex{gsl::narrow<GLuint>(vertices.size())};
  auto const normal{flip ? -frame.direction : frame.direction};
  vertices.push_back({center, normal});

  auto const offsetForIndex{[&](unsigned int index) -> glm::vec3 {
    auto const angle{(2.0f * glm::pi<float>() * gsl::narrow<float>(index)) /
                     gsl::narrow<float>(segments)};
    return radius * (std::cos(angle) * frame.tangent +
                     std::sin(angle) * frame.bitangent);
  }};

  for (auto const index : iter::range(segments + 1)) {
    auto const offset{offsetForIndex(index)};
    vertices.push_back({center + offset, normal});
  }

  for (auto const index : iter::range(segments)) {
    indices.push_back(centerIndex);
    if (!flip) {
      indices.push_back(centerIndex + index + 1);
      indices.push_back(centerIndex + index + 2);
    } else {
      indices.push_back(centerIndex + index + 2);
      indices.push_back(centerIndex + index + 1);
    }
  }
}

} // namespace

namespace geometry {

void createCylinder(std::vector<Vertex> &vertices, std::vector<GLuint> &indices,
                    glm::vec3 start, glm::vec3 end, float radius,
                    unsigned int segments, bool closeTop, bool closeBottom) {
  auto const baseIndex{gsl::narrow<GLuint>(vertices.size())};

  auto const direction{glm::normalize(end - start)};
  auto const frame{computeFrame(direction)};

  auto const offsetForIndex{[&](unsigned int index) -> glm::vec3 {
    auto const angle{(2.0f * glm::pi<float>() * gsl::narrow<float>(index)) /
                     gsl::narrow<float>(segments)};
    return radius * (std::cos(angle) * frame.tangent +
                     std::sin(angle) * frame.bitangent);
  }};

  for (auto const index : iter::range(segments + 1)) {
    auto const offset{offsetForIndex(index)};
    auto const normal{glm::normalize(offset)};

    vertices.push_back({start + offset, normal});
    vertices.push_back({end + offset, normal});
  }

  for (auto const index : iter::range(segments)) {
    auto const bl{baseIndex + index * 2};
    auto const br{baseIndex + (index + 1) * 2};
    auto const tl{bl + 1};
    auto const tr{br + 1};

    indices.push_back(bl);
    indices.push_back(br);
    indices.push_back(tl);

    indices.push_back(tl);
    indices.push_back(br);
    indices.push_back(tr);
  }

  if (closeBottom) {
    addCap(vertices, indices, start, frame, radius, segments, true);
  }

  if (closeTop) {
    addCap(vertices, indices, end, frame, radius, segments, false);
  }
}

void createCone(std::vector<Vertex> &vertices, std::vector<GLuint> &indices,
                glm::vec3 base, glm::vec3 tip, float radius,
                unsigned int segments, bool closeBase) {
  auto const baseIndex{gsl::narrow<GLuint>(vertices.size())};

  auto const direction{glm::normalize(tip - base)};
  auto const frame{computeFrame(direction)};

  auto const offsetForIndex{[&](unsigned int index) -> glm::vec3 {
    auto const angle{(2.0f * glm::pi<float>() * gsl::narrow<float>(index)) /
                     gsl::narrow<float>(segments)};
    return radius * (std::cos(angle) * frame.tangent +
                     std::sin(angle) * frame.bitangent);
  }};

  vertices.push_back({tip, direction});

  for (auto const index : iter::range(segments + 1)) {
    auto const offset{offsetForIndex(index)};
    auto const smoothNormal{glm::normalize(direction + glm::normalize(offset))};
    vertices.push_back({base + offset, smoothNormal});
  }

  for (auto const index : iter::range(segments)) {
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + index + 1);
    indices.push_back(baseIndex + index + 2);
  }

  if (closeBase) {
    addCap(vertices, indices, base, frame, radius, segments, true);
  }
}

// targetScreenRadius is in normalized device coordinates
// (e.g., 0.01 = 1% of screen)
float computeScreenSpaceRadius(Camera const &camera, float targetScreenRadius) {
  float worldRadius{};
  auto const fovY{camera.getFovY()};

  if (camera.getProjection() == Camera::Projection::Perspective) {
    // Perspective: radius scales with distance and FOV
    auto const distance{camera.getLookAtDistance()};
    auto const fovYRad{glm::radians(fovY)};

    // Height at the distance in world space
    auto const worldHeight{2.0f * distance * std::tan(fovYRad / 2.0f)};

    // Convert target screen radius to world space
    worldRadius = worldHeight * targetScreenRadius;
  } else {
    // Orthographic: FOV represents world height
    auto const worldHeight{fovY * 2.0f};
    auto const orthoScale{0.1f};
    worldRadius = worldHeight * targetScreenRadius * orthoScale;
  }

  // Take into account model scale
  worldRadius /= camera.getModelScale();

  return worldRadius;
}

} // namespace geometry