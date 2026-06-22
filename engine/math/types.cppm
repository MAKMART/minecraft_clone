export module engine.math:types;

import glm;
// TODO: Write your own math library, don't rely on glm
export namespace engine::math {
  using vec2 = glm::vec2;
  using vec3 = glm::vec3;
  using vec4 = glm::vec4;

  using ivec3 = glm::ivec3;
  using ivec4 = glm::ivec4;
  using uvec3 = glm::uvec3;
  using uvec4 = glm::uvec4;

  using dvec2 = glm::dvec2;
  using dvec3 = glm::dvec3;
  using dvec4 = glm::dvec4;

  using bvec4 = glm::bvec4;


  using mat4 = glm::mat4;
  using quat = glm::quat;

  using glm::normalize;
  using glm::cross;
  using glm::dot;
  using glm::clamp;
  using glm::distance;
  using glm::min;
  using glm::max;

  using glm::translate;
  using glm::rotate;
  using glm::scale;
  using glm::perspective;
  using glm::lookAt;
  using glm::greaterThan;
  using glm::mix;

  using glm::value_ptr;

  using glm::operator+;
  using glm::operator-;
  using glm::operator*;
  using glm::operator/;
  using glm::operator%;
  using glm::operator^;
  using glm::operator&;
  using glm::operator|;
  using glm::operator~;
  using glm::operator<<;
  using glm::operator>>;
  using glm::operator==;
  using glm::operator!=;
  using glm::operator&&;
  using glm::operator||;
}
