#include "Tracer.hpp"

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <algorithm>

#include "gloo/Transform.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/lights/AmbientLight.hpp"

#include "gloo/Image.hpp"
#include "Illuminator.hpp"
#include "PerspectiveCamera.hpp"

namespace GLOO {
float rand01() {
    static const float MAX = 1.f/RAND_MAX;
    return rand() * MAX;
}
void Tracer::Render(const Scene& scene,
                    const std::string& output_file) {
  scene_ptr_ = &scene;

  auto& root = scene_ptr_->GetRootNode();
  tracing_components_ = root.GetComponentPtrsInChildren<TracingComponent>();
  light_components_ = root.GetComponentPtrsInChildren<LightComponent>();


  Image image(image_size_.x, image_size_.y);
  // printf("Fog Density: %f\n", fog_density_);
  // Super sampling
  const float
      SAMPLE_OFFSET = 1.f / super_sampling_scale_,
      SAMPLE_WEIGHT = 1.f / (super_sampling_scale_ * super_sampling_scale_);

  for (size_t y = 0; y < image_size_.y; y++) {
    for (size_t x = 0; x < image_size_.x; x++) {
        
        glm::vec3 pixelColor(0);
        for (float j = 0; j < super_sampling_scale_; j++) {
            for (size_t i = 0; i < super_sampling_scale_; i++) {
                const float
                    offSetX = (i + (jitter_enabled_ ? rand01() : 0))
                        * SAMPLE_OFFSET,
                    offSetY = (j + (jitter_enabled_ ? rand01() : 0))
                        * SAMPLE_OFFSET;

                glm::vec2 imageCoords(
                    (x + offSetX) * 1.f / image_size_.x,
                    (y + offSetY) * 1.f / image_size_.y);

                // Remap 0-x, 0-y to -1 - 1
                imageCoords = (2.f * imageCoords) - 1.f;

                auto ray = camera_.GenerateRay(imageCoords);
                HitRecord hitRecord;
                glm::vec3 sampleColor = TraceRay(ray, max_bounces_, hitRecord);
                pixelColor += sampleColor;
            }
        }
        pixelColor *= SAMPLE_WEIGHT;
        image.SetPixel(x, y, pixelColor);
    }
  }

  if (output_file.size())
    image.SavePNG(output_file);
}


glm::vec3 Tracer::TraceRay(const Ray& ray,
                           size_t bounces,
                           HitRecord& record) const {
  // TODO: Compute the color for the cast ray.
    bool rayMiss = true;
    Material* hitMaterial;

    for (int i = 0; i < tracing_components_.size(); i++) {
        auto& tracingComponent = *tracing_components_[i];
        auto& hittable = tracingComponent.GetHittable();
        glm::mat4x4
            localToWorld = tracingComponent.GetNodePtr()->GetTransform().GetLocalToWorldMatrix(),
            worldToLocal = glm::inverse(localToWorld);
        Ray transformedRay = ray;
        transformedRay.ApplyTransform(worldToLocal);

        const float T_MIN = 0.001f;
        if (hittable.Intersect(transformedRay, T_MIN, record)) {
            rayMiss = false;
            record.normal = glm::transpose(glm::inverse(localToWorld)) * glm::vec4(record.normal, 0);
            record.normal = glm::normalize(record.normal);
            hitMaterial = &tracingComponent.GetNodePtr()->GetComponentPtr<MaterialComponent>()->GetMaterial();
        }
    }
    
    // Is intersection invalid?
    if (rayMiss) {
        auto bg = GetBackgroundColor(ray.GetDirection());

        return fog_enabled_ ? ApplyFog(bg, record.time, &ray) : bg;
    }
        

    const auto hitPos = ray.At(record.time);
    const auto rayDir = ray.GetDirection();
    const glm::vec3
        diffuseColor = hitMaterial->GetDiffuseColor(),
        specularColor = hitMaterial->GetSpecularColor();
    const float shininess = hitMaterial->GetShininess();
    const glm::vec3 reflectedEyeRay = glm::reflect(rayDir, record.normal);
    
    glm::vec3 outColor(0);
    const glm::vec3 WHITE(1);

    // Lighting calculations
    for (int i = 0; i < light_components_.size(); i++) {
        const LightComponent& light = *light_components_[i];
        auto lightType = light.GetLightPtr()->GetType();
        if (lightType == LightType::Ambient) {
            const glm::vec3 ambientColor = light.GetLightPtr()->GetDiffuseColor() * diffuseColor;
            outColor += fog_enabled_ ? fog_color_*ambientColor
                : ambientColor;
            outColor += ambientColor;
            continue;
        }

        float lightDist;
        glm::vec3
            lightDir,
            lightIntensity;

        Illuminator::GetIllumination(light, hitPos, lightDir, lightIntensity, lightDist);
        Ray lightRay(hitPos, lightDir);
        if (glm::dot(lightDir, record.normal) < 0
            || (shadows_enabled_ && !TraceShadow(lightDist, lightRay)))
            continue;
        // Diffuse lighting
        glm::vec3 diffuseLighting = glm::clamp(glm::dot(lightDir, record.normal), 0.f, 1.f)
            * lightIntensity * diffuseColor;
        // Specular lighting
        glm::vec3 specularLighting = pow(glm::clamp(glm::dot(reflectedEyeRay, lightDir), 0.f, 1.f), shininess)
            * lightIntensity * specularColor;
        outColor += fog_enabled_
                ? (diffuseLighting + specularLighting) * ApplyFog(WHITE, lightDist, nullptr)
                : diffuseLighting + specularLighting;
    }

    if (bounces > 0) {
        Ray reflectedRay(hitPos, reflectedEyeRay);
        HitRecord reflectionHits;
        outColor += specularColor * TraceRay(reflectedRay, bounces-1, reflectionHits);
    }
    
    // FOG
    if (fog_enabled_) {
        outColor = ApplyFog(outColor, record.time, &ray);
    }

    return outColor;
}

glm::vec3 Tracer::ApplyFog(const glm::vec3& inColor, float distanceThroughFog, const Ray* ray) const {
    float fogMass = 0.f;
    glm::vec3 fogLight(0);
    const float RAYMARCH_STEP_SIZE = .05;
    if (!shadows_enabled_ || ray == nullptr) {
        fogMass = distanceThroughFog * fog_density_;
        fogLight = fog_color_;
    } else if (shadows_enabled_ && RAYMARCH_STEP_SIZE < distanceThroughFog && ray != nullptr) {
        
        const float
            NEAR = 12,
            FAR = 20;
        float time = RAYMARCH_STEP_SIZE;
        fogMass = 0.f;
        int sampleCount = 0;
        
        const int MAX_SAMPLES = FAR / RAYMARCH_STEP_SIZE;
        while (time < distanceThroughFog && sampleCount < MAX_SAMPLES) {
            sampleCount++;
            glm::vec3 sampleIllumination(0);
            const glm::vec3 samplePoint = ray->At(time);
            const float sampleDensity = fog_density_ * RAYMARCH_STEP_SIZE;
            int sampleCount = 0;
            for (int i = 0; i < light_components_.size(); i++) {
                sampleCount++;
                const LightComponent& light = *light_components_[i];
                auto lightType = light.GetLightPtr()->GetType();
                if (lightType == LightType::Ambient) {
                    sampleIllumination += light.GetLightPtr()->GetDiffuseColor();
                    continue;
                }

                float lightDist;
                glm::vec3
                    lightDir,
                    lightIntensity;

                Illuminator::GetIllumination(light, samplePoint, lightDir, lightIntensity, lightDist);
                Ray lightRay(samplePoint, lightDir);
                if (!TraceShadow(lightDist, lightRay)) {
                    //printf("Shadow");
                    continue;
                }
                // printf("No shadow");
                // Diffuse lighting
                sampleIllumination += lightIntensity;
            }
            

            fogLight += sampleIllumination;
            fogMass += sampleDensity;
            time += RAYMARCH_STEP_SIZE;
        }
        float actualTime = sampleCount * RAYMARCH_STEP_SIZE;
        fogLight /= (0.f + sampleCount);
        // fogMass *= (distanceThroughFog / actualTime);
        fogMass = distanceThroughFog * fog_density_;
    }
    // printf("Fog color: %f, %f, %f\n", fogColor.r, fogColor.g, fogColor.b);
    float fogThinness = exp(-fogMass);
    return glm::mix(fogLight * fog_color_, inColor, fogThinness);
}

// True if light reaches
bool Tracer::TraceShadow(const float lightDist, const Ray& lightRay) const {
    HitRecord record;
    for (int i = 0; i < tracing_components_.size(); i++) {
        auto& tracingComponent = *tracing_components_[i];
        auto& hittable = tracingComponent.GetHittable();
        glm::mat4x4
            localToWorld = tracingComponent.GetNodePtr()->GetTransform().GetLocalToWorldMatrix(),
            worldToLocal = glm::inverse(localToWorld);
        Ray transformedRay = lightRay;
        transformedRay.ApplyTransform(worldToLocal);

        const float T_MIN = 0.001f;
        if (hittable.Intersect(transformedRay, T_MIN, record)
            && record.time < lightDist)
            return false;
    }

    return true;
}

glm::vec3 Tracer::GetBackgroundColor(const glm::vec3& direction) const {
  if (cube_map_ != nullptr) {
    return cube_map_->GetTexel(direction);
  } else
    return background_color_;
}
}  // namespace GLOO
