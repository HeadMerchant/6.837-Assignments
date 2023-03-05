#ifndef GELATIN_NODE_H
#define GELATIN_NODE_H
#include "ClothNode.hpp"
#include "ParticleSystemBase.hpp"
#include "ParticleState.hpp"
#include "gloo/SceneNode.hpp"
#include "PendulumSystem.hpp"
#include "IntegratorBase.hpp"
#include "gloo/VertexObject.hpp"

namespace GLOO {
	std::unique_ptr<PositionArray> CalcNormals(const PositionArray& pos, const IndexArray& tris);
	class GelatinNode :
		public SceneNode
	{
	public:
		GelatinNode(std::unique_ptr<ClothIntegrator> integrator, float integrationStep,
			int width, int height, int depth,
			float dragCoeff, float gravity, float particleSeparation, float stiffness);
		void Update(double dt) override;
		void UpdateVertexPositions(const IndexArray& tris);
		int IndexOf(int x, int y, int z);

	private:
		PendulumSystem particleSystem_;
		std::unique_ptr<ClothIntegrator> integrator_;
		float integrationStep_;
		float remainingStep_ = 0;
		ParticleState particleState_;
		int gridWidth_, gridHeight_, gridDepth_;
		float currentTime;
		VertexObject* mesh_;
		
		/*
		float timer_ = 0;
		glm::vec3 spherePos_ = glm::vec3(1, 0, 0);
		float sphereRadius_ = 1.2;
		float spherePathAmplitude_ = 4.;
		SceneNode* sphereNode_;
		*/
	};

}

#endif

