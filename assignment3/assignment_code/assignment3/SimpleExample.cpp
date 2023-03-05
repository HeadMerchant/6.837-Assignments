#include "SimpleExample.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"

namespace GLOO {

    SimpleExample::SimpleExample(std::unique_ptr<SimpleIntegrator> integrator, double integrationStep) {
        integrator_ = std::move(integrator);
        integrationStep_ = integrationStep;
        remainingStep_ = 0;

        auto particle = make_unique<SceneNode>();
        particle_ = particle.get();

        particle->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(1., 32, 32));
        particle->CreateComponent<MaterialComponent>(std::make_shared<Material>(Material::GetDefault()));
        particle->CreateComponent<ShadingComponent>(std::make_shared<PhongShader>());
        AddChild(std::move(particle));

        particleState_.positions.push_back(glm::vec3(1, 1, 0));
        particleState_.velocities.push_back(glm::vec3());
    }
    void SimpleExample::Update(double delta_time) {
        remainingStep_ += delta_time;
        while (remainingStep_ > integrationStep_) {
            particleState_ = integrator_->Integrate(particleSystem_, particleState_, 0, integrationStep_);
            remainingStep_ -= integrationStep_;
        }
        particle_->GetTransform().SetPosition(particleState_.positions[0]);
    }

    ParticleState Eqn1ParticleSystem::ComputeTimeDerivative(const ParticleState& state,
        float time) const {
        ParticleState derivative;
        for (int i = 0; i < state.positions.size(); i++) {
            glm::vec3 pos = state.positions[i];
            derivative.positions.push_back(glm::vec3(-pos.y, pos.x, 0.f));
            derivative.velocities.push_back(glm::vec3());
        }
        return derivative;
    }

    /*
    private:
        SceneNode* particle_;
        double integrationStep_;
        ParticleState state_;
        ForwardEulerIntegrator<ParticleSystemBase, ParticleState> integrator_;*/
}  // namespace GLOO