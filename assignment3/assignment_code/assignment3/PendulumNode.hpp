#ifndef PENDULUM_NODE_H
#define PENDULUM_NODE_H
#include "ParticleSystemBase.hpp"
#include "ParticleState.hpp"
#include "gloo/SceneNode.hpp"
#include "PendulumSystem.hpp"
#include "IntegratorBase.hpp"

namespace GLOO {
	typedef IntegratorBase<PendulumSystem, ParticleState> PendulumIntegrator;
	class PendulumNode :
		public SceneNode
	{
		public:
			PendulumNode(std::unique_ptr<PendulumIntegrator> integrator, float integrationStep_,
				float dragCoeff, float gravity, int numParticles, float particleSeparation, float stiffness);
			void Update(double delta_time) override;

		private:
			PendulumSystem particleSystem_;
			std::vector<SceneNode*> particles_;
			std::unique_ptr<PendulumIntegrator> integrator_;
			float integrationStep_;
			float remainingStep_;
			ParticleState particleState_;
	};
}

#endif

