#include "Plane.hpp"

namespace GLOO {
Plane::Plane(const glm::vec3& normal, float d) {
	normal_ = normal;
	position_offset_ = d;
}

bool Plane::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
	float parrallelism = glm::dot(normal_, ray.GetDirection());
	if (parrallelism == 0)
		return false;
	float time = (position_offset_ - glm::dot(normal_, ray.GetOrigin()) )
		/ parrallelism;

	// Hit
	if (time >= t_min && time < record.time) {
		record.normal = normal_;
		record.time = time;
		return true;
	}

	return false;
}
}  // namespace GLOO
