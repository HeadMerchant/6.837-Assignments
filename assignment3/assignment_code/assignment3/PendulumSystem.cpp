#include "PendulumSystem.hpp"

namespace GLOO {
	
	PendulumSystem::PendulumSystem(float dragCoeff, float gravityCoeff) {
		dragCoeff_ = dragCoeff;
		gravityCoeff_ = gravityCoeff;
	}

	ParticleState PendulumSystem::ComputeTimeDerivative(const ParticleState& state, float time) const{
		std::vector<glm::vec3> 
			velocities(state.velocities),
			forces;

		
		const glm::vec3 gravity = glm::vec3(0, -gravityCoeff_, 0);
		for (int i = 0; i < particles_.size(); i++){
			//printf(particles_[i].isFixed ? "true\n" : "false\n");
			glm::vec3 dragAndGravity = -dragCoeff_ * state.velocities[i];
			dragAndGravity += particles_[i].mass * gravity;
			forces.push_back(dragAndGravity);
		}
		
		// Spring forces
		for (int i = 0; i < springs_.size(); i++) {
			Spring spring = springs_[i];
			int p1 = spring.particle1Index,
				p2 = spring.particle2Index;
			glm::vec3
				pos1 = state.positions[p1],
				pos2 = state.positions[p2];

			glm::vec3 deltaPos = pos2 - pos1;
			float currentLength = glm::length(deltaPos);
			float deltaLength = currentLength - spring.restingLength;
			
			glm::vec3 springForce = -spring.stiffness * deltaLength * (deltaPos / currentLength);
			forces[p1] -= springForce;
			forces[p2] += springForce;
		}

		auto& accelerations = forces;
		
		// force -> acceleration; fix particles
		for (int i = 0; i < particles_.size(); i++) {
			auto particle = particles_[i];
			//printf("Mass: %f\n", particle.mass);
			if (particle.isFixed) {
				velocities[i] = glm::vec3();
				accelerations[i] = glm::vec3();
			}
			else {
				accelerations[i] = forces[i] / particle.mass;
			}
			/*accelerations[i] = particle.isFixed ?
				glm::vec3() : forces[i] / particle.mass;*/
		}

		ParticleState derivative{ velocities, accelerations };
		return derivative;
	}

	void PendulumSystem::AddSpring(int particle1Index, int particle2Index, float stiffness, float restingLength) {
		Spring spring{ particle1Index, particle2Index, stiffness, restingLength };
		springs_.push_back(spring);
	}

	int PendulumSystem::AddParticle(double mass, bool isFixed) {
		int index = particles_.size();
		FixedParticle particle;
		particle.mass = mass;
		particle.isFixed = isFixed;
		particles_.push_back(particle);

		return index;
	}

	void PendulumSystem::FixParticle(int particleIndex, bool isFixed) {
		particles_[particleIndex].isFixed = isFixed;
	}
}