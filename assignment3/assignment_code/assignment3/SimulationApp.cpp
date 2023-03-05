#include "SimulationApp.hpp"

#include "glm/gtx/string_cast.hpp"

#include "gloo/shaders/PhongShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/CameraComponent.hpp"
#include "gloo/components/LightComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/MeshLoader.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/lights/AmbientLight.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/debug/AxisNode.hpp"

#include "IntegratorFactory.hpp"
#include "SimpleExample.hpp"
#include "PendulumNode.hpp"
#include "PendulumSystem.hpp"
#include "ClothNode.hpp"
//#include "GelatinNode.hpp"

namespace GLOO {
SimulationApp::SimulationApp(const std::string& app_name,
                             glm::ivec2 window_size,
                             IntegratorType integrator_type,
                             float integration_step)
    : Application(app_name, window_size),
      integrator_type_(integrator_type),
      integration_step_(integration_step) {
  // TODO: remove the following two lines and use integrator type and step to
  // create integrators; the lines below exist only to suppress compiler
  // warnings.
  UNUSED(integrator_type_);
  UNUSED(integration_step_);
}

void SimulationApp::SetupScene() {
  SceneNode& root = scene_->GetRootNode();

  auto camera_node = make_unique<ArcBallCameraNode>(45.f, 0.75f, 5.0f);
  camera_node->GetTransform().SetPosition(glm::vec3(0, 0, -10));
  scene_->ActivateCamera(camera_node->GetComponentPtr<CameraComponent>());
  root.AddChild(std::move(camera_node));

  root.AddChild(make_unique<AxisNode>('A'));

  auto ambient_light = std::make_shared<AmbientLight>();
  ambient_light->SetAmbientColor(glm::vec3(0.2f));
  root.CreateComponent<LightComponent>(ambient_light);

  auto point_light = std::make_shared<PointLight>();
  point_light->SetDiffuseColor(glm::vec3(0.8f, 0.8f, 0.8f));
  point_light->SetSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));
  point_light->SetAttenuation(glm::vec3(1.0f, 0.09f, 0.032f));
  auto point_light_node = make_unique<SceneNode>();
  point_light_node->CreateComponent<LightComponent>(point_light);
  point_light_node->GetTransform().SetPosition(glm::vec3(0.0f, 2.0f, 4.f));
  root.AddChild(std::move(point_light_node));


  {
      auto integrator = IntegratorFactory::CreateIntegrator<Eqn1ParticleSystem, ParticleState>(integrator_type_);
      auto simpleExample = make_unique<SimpleExample>(std::move(integrator), integration_step_);
      simpleExample.get()->GetTransform().SetPosition(glm::vec3(-5, 0, 0));
      root.AddChild(std::move(simpleExample));
  }

  {
      float
          // close to minimum drag for euler to not blow up at time step 0.005
          // 0.2 does well for RK4
          drag = integrator_type_ == IntegratorType::Euler ? .65 : .1,
          grav = 9.8,
          restLength = 1.,
          stiffness = 50.;
      int numParticles = 4;

      auto pendulumIntegrator = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type_);
      auto pendulumNode = make_unique<PendulumNode>(std::move(pendulumIntegrator), integration_step_,
          drag, grav, numParticles, restLength, stiffness);
      pendulumNode.get()->GetTransform().SetPosition(glm::vec3(3, 0, 0));
      root.AddChild(std::move(pendulumNode));
  }
  {
      int clothSize = 5;
      int clothResolution = 20;
      float
          drag = 0.4,
          grav = 9.8,
          restLength = static_cast<float>(clothSize)/clothResolution,
          stiffness = 100.;
      auto integrator = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type_);
      auto cloth = make_unique<ClothNode>(std::move(integrator), integration_step_,
          clothResolution, clothResolution,
          drag, grav, restLength, stiffness);
      cloth->GetTransform().SetPosition(glm::vec3(-.5 * clothSize, 0, 0));
      root.AddChild(std::move(cloth));
  }
  /*
  {
      int gelSize = 5;
      int gelResolution = 3;
      float
          drag = 0.4,
          grav = 9.8,
          restLength = static_cast<float>(gelSize) / gelResolution,
          stiffness = 100.;
      auto integrator = IntegratorFactory::CreateIntegrator<PendulumSystem, ParticleState>(integrator_type_);
      printf("Gonna make node\n");
      auto gel = make_unique<GelatinNode>(std::move(integrator), integration_step_,
          gelResolution, gelResolution, gelResolution,
          drag, grav, restLength, stiffness);
      printf("Set its position\n");
      gel->GetTransform().SetPosition(glm::vec3(-.5 * gelSize, 0, 0));
      printf("Add to scene\n");
      root.AddChild(std::move(gel));
  }*/
}
}  // namespace GLOO
