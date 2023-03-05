#include "GelatinNode.hpp"
#include "ClothNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
//#include "gloo/shaders/PhongShader.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"

namespace GLOO {
	typedef SimpleShader GelatinShaderType;
	GelatinNode::GelatinNode(std::unique_ptr<ClothIntegrator> integrator, float integrationStep,
		int width, int height, int depth, 
		float dragCoeff, float gravity, float particleSpacing, float stiffness)
		: particleSystem_(dragCoeff, gravity) {

		integrator_ = std::move(integrator);
		integrationStep_ = integrationStep;
		gridWidth_ = width;
		gridHeight_ = height;
		gridDepth_ = depth;
		
		// Add particles/positions/velocities
		const float particleMass = 2. * pow(particleSpacing, 3.);
		int i = 0;
		for (float z = 0; z < gridDepth_; z++) {
		for (float y = 0; y < gridHeight_; y++) {
		for (float x = 0; x < gridWidth_; x++) {
			glm::vec3 pos(x, y, z);
			pos *= particleSpacing;
			particleState_.positions.push_back(pos);
			particleState_.velocities.push_back(glm::vec3());

			//printf("%d\n", ++i);
			particleSystem_.AddParticle(particleMass, false);
		}}}

		// Bottom four corners
		particleSystem_.FixParticle(0, true);
		particleSystem_.FixParticle(gridWidth_-1, true);
		particleSystem_.FixParticle(IndexOf(0, 0, gridDepth_-1), true);
		particleSystem_.FixParticle(IndexOf(gridWidth_ - 1, 0, gridDepth_ - 1), true);

		std::shared_ptr<VertexObject> mesh = std::make_shared<VertexObject>();
		mesh_ = mesh.get();

		std::unique_ptr<IndexArray> indicesPTR = make_unique<IndexArray>();
		auto& indices = *indicesPTR.get();
		
		const float
			diagonalDist = particleSpacing * glm::root_two<float>(),
			diagonalDist2 = particleSpacing * glm::root_three<float>(),
			doubleDist = 2 * particleSpacing;
		
		for (int x = 0; x < gridWidth_; x++) {
		for (int y = 0; y < gridHeight_; y++) {
		for (int z = 0; z < gridDepth_; z++) {
			int i = IndexOf(x, y, z);
			{
				// Structural springs
				int right = x + 1;
				if (right < gridWidth_) {
					int rightIndex = IndexOf(right, y, z);
					particleSystem_.AddSpring(i, rightIndex, stiffness, particleSpacing);
				}

				int up = y + 1;
				if (up < gridHeight_) {
					int upIndex = IndexOf(x, up, z);
					particleSystem_.AddSpring(i, upIndex, stiffness, particleSpacing);
				}

				int forward = z + 1;
				if (forward < gridDepth_) {
					int forwardIndex = IndexOf(x, y, forward);
					particleSystem_.AddSpring(i, forwardIndex, stiffness, particleSpacing);

					float dist = diagonalDist;
					for (int up = y; up <= y + 1 && up < gridHeight_; up++) {
						int left = x - 1;
						if (left >= 0) {
							particleSystem_.AddSpring(i, IndexOf(left, up, forward), stiffness, dist);
						}
						if (right < gridWidth_) {
							particleSystem_.AddSpring(i, IndexOf(right, up, forward), stiffness, dist);
						}
						dist = diagonalDist2;
					}
				}
			}

			// Flex springs
			
			{
				int right = x + 2;
				if (right < gridWidth_) {
					int rightIndex = IndexOf(right, y, z);
					particleSystem_.AddSpring(i, rightIndex, stiffness, particleSpacing);
				}

				int up = y + 2;
				if (up < gridHeight_) {
					int upIndex = IndexOf(x, up, z);
					particleSystem_.AddSpring(i, upIndex, stiffness, particleSpacing);
				}

				int forward = z + 2;
				if (forward < gridHeight_) {
					int forwardIndex = IndexOf(x, y, forward);
					particleSystem_.AddSpring(i, forwardIndex, stiffness, particleSpacing);
				}
			}
		}}}

		// Line rendering
		for (int x = 0; x < gridWidth_; x++) {
		for (int y = 0; y < gridHeight_; y++) {
		for (int z = 0; z < gridDepth_; z++) {
			int i = IndexOf(x, y, z);

			int right = x + 1;
			if (right < gridWidth_) {
				int rightIndex = IndexOf(right, y, z);
				indices.push_back(i);
				indices.push_back(rightIndex);
			}

			int up = y + 1;
			if (up < gridHeight_) {
				int upIndex = IndexOf(x, up, z);
				indices.push_back(i);
				indices.push_back(upIndex);
			}

			int forward = z + 1;
			if (forward < gridHeight_) {
				int forwardIndex = IndexOf(x, y, forward);
				indices.push_back(i);
				indices.push_back(forwardIndex);
			}
		}}}

		const auto& tris = indices;
		UpdateVertexPositions(tris);
		mesh_->UpdateIndices(std::move(indicesPTR));
		RenderingComponent& renderer = CreateComponent<RenderingComponent>(std::move(mesh));
		renderer.SetDrawMode(DrawMode::Lines);
		auto material = std::make_shared<Material>(glm::vec3(), glm::vec3(.2, .2, .8), glm::vec3(0,0,.8), .6);
		//auto shader = std::make_shared<PhongShader>();
		auto shader = std::make_shared<GelatinShaderType>();
		CreateComponent<MaterialComponent>(material);
		CreateComponent<ShadingComponent>(shader);

		// Sphere for interactions
		/*
		auto sphereNode = make_unique<SceneNode>();
		// Use  -0.03 to avoid clipping
		sphereNode->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(sphereRadius_ - 0.03, 64, 64));
		sphereNode->CreateComponent<MaterialComponent>(material);
		sphereNode->CreateComponent<ShadingComponent>(shader);

		sphereNode->GetTransform().SetPosition(spherePos_);
		sphereNode_ = sphereNode.get();
		AddChild(std::move(sphereNode));
		*/
	}

	void GelatinNode::Update(double dt) {
		remainingStep_ += dt;

		//printf("Remaining step: %f\nDelta time: %f\n\n", remainingStep_, delta_time);
		while (remainingStep_ > integrationStep_) {
			particleState_ = integrator_->Integrate(particleSystem_, particleState_, 0, integrationStep_);
			remainingStep_ -= integrationStep_;
			
		}
		
		auto tris = mesh_->GetIndices();
		UpdateVertexPositions(tris);
		//sphereNode_->GetTransform().SetPosition(spherePos_);
	}

	void GelatinNode::UpdateVertexPositions(const IndexArray& tris) {
		std::unique_ptr<PositionArray> vertices = make_unique<PositionArray>();
		for (int i = 0; i < particleState_.positions.size(); i++) {
			glm::vec3 pos = particleState_.positions[i];
			vertices->push_back(pos);
		}
		mesh_->UpdatePositions(std::move(vertices));
		/*
		auto& verts = mesh_->GetPositions();
		mesh_->UpdateNormals(std::move(CalcNormals(verts, tris)));
		*/
	}

	int GelatinNode::IndexOf(int x, int y, int z){
		// https://stackoverflow.com/a/7367817
		return x + gridHeight_ * (y + gridDepth_ * z);
	}
}

/*
// Sphere
timer_ += integrationStep_;
spherePos_.z = sin(timer_) * spherePathAmplitude_;
// Sphere-particle collisions
for (int i = 0; i < particleState_.positions.size(); i++) {
	auto pos = particleState_.positions[i],
		vel = particleState_.velocities[i];
	auto projectionDir = pos - spherePos_;
	if (glm::length(projectionDir) <= sphereRadius_) {
		auto normal = glm::normalize(projectionDir);
		particleState_.positions[i] = (normal * sphereRadius_) + spherePos_;

		// Friction
		float dot = glm::dot(normal, vel);
		dot = fmax(dot, 0);
		vel += normal * dot;
		particleState_.velocities[i] = vel;
	}
}
*/