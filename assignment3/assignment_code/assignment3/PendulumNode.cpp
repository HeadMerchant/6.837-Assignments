#include "PendulumNode.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"

namespace GLOO {
	
	PendulumNode::PendulumNode(std::unique_ptr<PendulumIntegrator> integrator, float integrationStep, 
		float dragCoeff, float gravity, int numParticles, float particleSeparation, float stiffness)
		: particleSystem_(dragCoeff, gravity) {
		integrator_ = std::move(integrator);
		integrationStep_ = integrationStep;
		remainingStep_ = 0;

		static std::shared_ptr<VertexObject> sphereGeometry =
			PrimitiveFactory::CreateSphere(0.2, 16, 16);
			
		static auto material = std::make_shared<Material>(Material::GetDefault());
		static auto shader = std::make_shared<PhongShader>();
			
		const glm::vec3 particlePos(1, 0, 0);
		for (int i = 0; i < numParticles; i++) {

			// Node setup
			auto particle = make_unique<SceneNode>();
			particle->CreateComponent<RenderingComponent>(sphereGeometry);
			particle->CreateComponent<MaterialComponent>(material);
			particle->CreateComponent<ShadingComponent>(shader);

			// Add particle to scene
			particles_.push_back(particle.get());
			glm::vec3 pos = static_cast<float>(i) * particlePos;
			particle->GetTransform().SetPosition(pos);
			AddChild(std::move(particle));
			
			// State
			particleState_.positions.push_back(pos);
			particleState_.velocities.push_back(glm::vec3());

			// Springs
			particleSystem_.AddParticle(1, i == 0);
			if (i > 0) {
				particleSystem_.AddSpring(i - 1, i, stiffness, particleSeparation);
			}
		}
		
	}

	void PendulumNode::Update(double delta_time) {
		remainingStep_ += delta_time;
		//printf("Remaining step: %f\nDelta time: %f\n\n", remainingStep_, delta_time);
		while (remainingStep_ > integrationStep_) {
			particleState_ = integrator_->Integrate(particleSystem_, particleState_, 0, integrationStep_);
			remainingStep_ -= integrationStep_;
		}
		for (int i = 0; i < particles_.size(); i++) {
			glm::vec3 pos = particleState_.positions[i];
			//printf("%d: %f, %f, %f\n", i, pos.x, pos.y, pos.z);
			SceneNode& p = *(particles_[i]);
			p.GetTransform().SetPosition(pos);
		}
		
	}
}