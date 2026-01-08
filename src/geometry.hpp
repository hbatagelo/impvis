/**
 * @file geometry.hpp
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

#ifndef GEOMETRY_HPP_
#define GEOMETRY_HPP_

#include "camera.hpp"

#include <abcgOpenGLExternal.hpp>

namespace geometry {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

void createCylinder(std::vector<Vertex> &vertices, std::vector<GLuint> &indices,
                    glm::vec3 start, glm::vec3 end, float radius,
                    unsigned int segments = 16, bool closeTop = false,
                    bool closeBottom = true);

void createCone(std::vector<Vertex> &vertices, std::vector<GLuint> &indices,
                glm::vec3 base, glm::vec3 tip, float radius,
                unsigned int segments = 16, bool closeBase = true);

[[nodiscard]] float computeScreenSpaceRadius(Camera const &camera,
                                             float targetScreenRadius);

} // namespace geometry

#endif