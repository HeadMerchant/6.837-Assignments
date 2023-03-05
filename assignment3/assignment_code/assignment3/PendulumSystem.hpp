#ifndef PENDULUM_SYSTEM_H

#define PENDULUM_SYSTEM_H
#include "ParticleSystemBase.hpp"
#include "ParticleState.hpp"

namespace GLOO {
	struct FixedParticle {
		bool isFixed = false;
		float mass = 1;
	};

	struct Spring {
		int particle1Index;
		int particle2Index;
		float stiffness;
		float restingLength;
	};

	class PendulumSystem :
		public ParticleSystemBase
	{
		public:	
			PendulumSystem(float dragCoeff, float gravityCoeff);
			ParticleState ComputeTimeDerivative(const ParticleState& state,
				float time) const override;
			void AddSpring(int particle1Index, int particle2Index, float stiffness, float restingLength);
			int AddParticle(double mass, bool isFixed);
			void FixParticle(int particleIndex, bool isFixed);
			
		private:
			float dragCoeff_;
			float gravityCoeff_;
			std::vector<FixedParticle> particles_;
			std::vector<Spring> springs_;
	};
};

#endif //PENDULUM_SYSTEM_H