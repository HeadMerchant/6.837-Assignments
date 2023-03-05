#ifndef CLOTH_NODE_H
#define CLOTH_NODE_H
#include "ParticleSystemBase.hpp"
#include "ParticleState.hpp"
#include "gloo/SceneNode.hpp"
#include "PendulumSystem.hpp"
#include "IntegratorBase.hpp"
#include "gloo/VertexObject.hpp"

namespace GLOO {
	typedef IntegratorBase<PendulumSystem, ParticleState> ClothIntegrator;
	std::unique_ptr<PositionArray> CalcNormals(const PositionArray& pos, const IndexArray& tris);
	class ClothNode :
		public SceneNode
	{
	public:
		ClothNode(std::unique_ptr<ClothIntegrator> integrator, float integrationStep,
			int width, int height,
			float dragCoeff, float gravity, float particleSeparation, float stiffness);
		void Update(double delta_time) override;
		void UpdateVertexPositions(const IndexArray& tris);
		int IndexOf(int x, int y);
		/* Indices example:
		* 0 1 2 3
		* 4 5 6 7
		* 8 9 10 11 */

	private:
		PendulumSystem particleSystem_;
		std::unique_ptr<ClothIntegrator> integrator_;
		float integrationStep_;
		float remainingStep_ = 0;
		ParticleState particleState_;
		int gridWidth_, gridHeight_;
		float currentTime;
		VertexObject* mesh_;
		
		
		float timer_ = glm::half_pi<float>()-.1;
		glm::vec3 spherePos_ = glm::vec3(1, 0, 0);
		float sphereRadius_ = 1.2;
		float spherePathAmplitude_ = 4.;
		SceneNode* sphereNode_;
	};

}

#endif

