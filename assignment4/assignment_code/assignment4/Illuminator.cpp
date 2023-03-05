#include "Illuminator.hpp"

#include <limits>
#include <stdexcept>

#include <glm/geometric.hpp>

#include "gloo/lights/DirectionalLight.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/SceneNode.hpp"

namespace GLOO {
void Illuminator::GetIllumination(const LightComponent& light_component,
                                  const glm::vec3& hit_pos,
                                  glm::vec3& dir_to_light,
                                  glm::vec3& intensity,
                                  float& dist_to_light) {
  // Calculation will be done in world space.

  auto light_ptr = light_component.GetLightPtr();
  auto lightType = light_ptr->GetType();
  if (lightType == LightType::Directional) {
    auto directional_light_ptr = static_cast<DirectionalLight*>(light_ptr);
    dir_to_light = -directional_light_ptr->GetDirection();
    intensity = directional_light_ptr->GetDiffuseColor();
    dist_to_light = std::numeric_limits<float>::max();
  } else if (lightType == LightType::Point) {
      auto point_light_ptr = static_cast<PointLight*>(light_ptr);
      glm::vec3 lightPos = light_component.GetNodePtr()->GetTransform().GetPosition();
      glm::vec3 lightDisplacement = lightPos - hit_pos;
      dir_to_light = glm::normalize(lightDisplacement);
      dist_to_light = glm::length(lightDisplacement);
      intensity = point_light_ptr->GetDiffuseColor()
          / (point_light_ptr->GetAttenuation() * pow(dist_to_light, 2.f));
  } else {  // TODO: Implement point light.
      printf("Mission failed");
    throw std::runtime_error(
        "Unrecognized light type when computing "
        "illumination");
  }

}
}  // namespace GLOO
