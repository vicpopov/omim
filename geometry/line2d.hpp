#pragma once

#include "geometry/point2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"

#include <string>

namespace m2
{
struct IntersectionResult
{
  enum class Type
  {
    Zero,
    One,
    Infinity
  };

  explicit IntersectionResult(Type type) : m_type(type) { ASSERT_NOT_EQUAL(m_type, Type::One, ()); }
  explicit IntersectionResult(PointD const & point) : m_point(point), m_type(Type::One) {}

  PointD m_point;
  Type m_type;
};

struct Line2D
{
  Line2D() = default;

  explicit Line2D(Segment2D const & segment) : m_point(segment.m_u), m_direction(segment.Dir()) {}

  Line2D(PointD const & point, PointD const & direction) : m_point(point), m_direction(direction) {}

  PointD m_point;
  PointD m_direction;
};

IntersectionResult Intersect(Line2D const & lhs, Line2D const & rhs, double eps);

std::string DebugPrint(Line2D const & line);
std::string DebugPrint(IntersectionResult::Type type);
std::string DebugPrint(IntersectionResult const & result);
}  // namespace m2
