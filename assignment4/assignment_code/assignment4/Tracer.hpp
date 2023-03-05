#ifndef TRACER_H_
#define TRACER_H_

#include "gloo/Scene.hpp"
#include "gloo/Material.hpp"
#include "gloo/lights/LightBase.hpp"
#include "gloo/components/LightComponent.hpp"

#include "Ray.hpp"
#include "HitRecord.hpp"
#include "TracingComponent.hpp"
#include "CubeMap.hpp"
#include "PerspectiveCamera.hpp"

namespace GLOO {
class Tracer {
 public:
  Tracer(const CameraSpec& camera_spec,
         const glm::ivec2& image_size,
         size_t max_bounces,
         const glm::vec3& background_color,
         const CubeMap* cube_map,
         bool shadows_enabled,
         int super_sampling_scale,
         bool jitter_enabled,
         float fog_density,
         glm::vec3 fog_color)
      : camera_(camera_spec),
        image_size_(image_size),
        max_bounces_(max_bounces),
        background_color_(background_color),
        cube_map_(cube_map),
        shadows_enabled_(shadows_enabled),
        super_sampling_scale_(super_sampling_scale),
        jitter_enabled_(jitter_enabled),
        fog_density_(fog_density),
        fog_color_(fog_color),
        fog_enabled_(fog_density != 0.f),
        scene_ptr_(nullptr) {
  }
  void Render(const Scene& scene, const std::string& output_file);

 private:
  glm::vec3 TraceRay(const Ray& ray, size_t bounces, HitRecord& record) const;
  bool TraceShadow(const float lightDist, const Ray& lightRay) const;

  glm::vec3 GetBackgroundColor(const glm::vec3& direction) const;
  glm::vec3 ApplyFog(const glm::vec3& inColor, float distanceThroughFog, const Ray* ray) const;

  PerspectiveCamera camera_;
  glm::ivec2 image_size_;
  size_t max_bounces_;

  std::vector<TracingComponent*> tracing_components_;
  std::vector<LightComponent*> light_components_;
  glm::vec3 background_color_;
  const CubeMap* cube_map_;
  bool shadows_enabled_;
  int super_sampling_scale_;
  bool jitter_enabled_;

  float fog_density_;
  glm::vec3 fog_color_;
  bool fog_enabled_;

  const Scene* scene_ptr_;
};
}  // namespace GLOO

#endif
