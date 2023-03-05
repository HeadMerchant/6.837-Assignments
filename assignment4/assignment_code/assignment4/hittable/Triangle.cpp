#include "Triangle.hpp"

#include <iostream>
#include <stdexcept>

#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Plane.hpp"

namespace GLOO {
Triangle::Triangle(const glm::vec3& p0,
                   const glm::vec3& p1,
                   const glm::vec3& p2,
                   const glm::vec3& n0,
                   const glm::vec3& n1,
                   const glm::vec3& n2) {
    positions_.push_back(p0);
    positions_.push_back(p1);
    positions_.push_back(p2);

    normals_.push_back(n0);
    normals_.push_back(n1);
    normals_.push_back(n2);
}

Triangle::Triangle(const std::vector<glm::vec3>& positions,
                   const std::vector<glm::vec3>& normals)
    : positions_(positions), normals_(normals) {
    // This should be empty here; don't fill it in
}

bool Triangle::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  // TODO: Implement ray-triangle intersection.
    const glm::vec3
        posA = GetPosition(0),
        posB = GetPosition(1),
        posC = GetPosition(2),
        rayOrigin = ray.GetOrigin(),
        rayDirection = ray.GetDirection();

    //M*<beta, gamma, time> = b
    glm::mat3x3 M(
        posA - posB,
        posA - posC,
        rayDirection
    );
    glm::vec3 b(
        posA - rayOrigin
    );

    glm::vec3 beta_gamma_time = glm::inverse(M)*b;
    const float
        beta = beta_gamma_time.x,
        gamma = beta_gamma_time.y,
        time = beta_gamma_time.z;
    const float alpha = 1.f - beta - gamma;
    
    // Missed plane
    if (time < t_min || time > record.time)
        return false;

    // Hit plane, missed triangle
    if (alpha < 0 || beta < 0 || gamma < 0)
        return false;

    // Hits plane and is inside triangle
    record.time = time;
    record.normal = glm::normalize(
        (GetNormal(0) * alpha)
        + (GetNormal(1) * beta)
        + (GetNormal(2) * gamma)
    );
    return true;
}
}  // namespace GLOO
