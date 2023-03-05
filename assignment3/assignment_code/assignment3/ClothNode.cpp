#include "ClothNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"

namespace GLOO {
	std::unique_ptr<PositionArray> CalcNormals(const PositionArray& pos, const IndexArray& tris) {
		std::unique_ptr<PositionArray> normals = make_unique<PositionArray>();

		// Init normals to 0
		for (int i = 0; i < pos.size(); i++) {
			normals->push_back(glm::vec3(0.f));
		}

		// Add face normals to adjacent vertices, weighted by triangle area
		for (int i = 0; i < tris.size(); i += 3) {
			int aI = tris[i],
				bI = tris[i + 1],
				cI = tris[i + 2];
			glm::vec3
				a = pos[aI],
				b = pos[bI],
				c = pos[cI],
				edge1 = b - a,
				edge2 = c - a,
				faceNormal = glm::cross(edge1, edge2);
			float area = glm::length(faceNormal) / 2.f; //Parallelogram area / 2 = tri area
			faceNormal = glm::normalize(faceNormal) * area; // Weight face normal on vertex by area

			(*normals)[aI] += faceNormal;
			(*normals)[bI] += faceNormal;
			(*normals)[cI] += faceNormal;
		}

		// Normalize normals
		for (int i = 0; i < pos.size(); i++) {
			(*normals)[i] = glm::normalize((*normals)[i]);
		}

		return std::move(normals);
	}

	ClothNode::ClothNode(std::unique_ptr<ClothIntegrator> integrator, float integrationStep,
		int width, int height,
		float dragCoeff, float gravity, float particleSpacing, float stiffness)
		: particleSystem_(dragCoeff, gravity) {

		integrator_ = std::move(integrator);
		integrationStep_ = integrationStep;
		gridWidth_ = width;
		gridHeight_ = height;
		
		std::shared_ptr<VertexObject> mesh = std::make_shared<VertexObject>();
		mesh_ = mesh.get();
		
		// Add particles/positions/velocities
		const float particleMass = 2. * (particleSpacing*particleSpacing);
		for (int y = gridHeight_ - 1; y >= 0; y--) {
			for (int x = 0; x < gridWidth_; x++) {
				glm::vec3 pos(static_cast<float>(x), static_cast<float>(y), 0);
				pos *= particleSpacing;
				particleState_.positions.push_back(pos);
				particleState_.velocities.push_back(glm::vec3());
				
				particleSystem_.AddParticle(particleMass, false);
			}
		}

		// Fix top left/top right
		particleSystem_.FixParticle(0, true);
		particleSystem_.FixParticle(gridWidth_-1, true);

		const float
			diagonalDist = particleSpacing * glm::root_two<float>(),
			doubleDist = 2 * particleSpacing;
		for (int x = 0; x < gridWidth_; x++) { for (int y = 0; y < gridHeight_; y++) {
			int i = IndexOf(x, y);
			{
			// Structural springs
			int right = x + 1;
			if (right < gridWidth_) {
				int rightIndex = IndexOf(right, y);
				particleSystem_.AddSpring(i, rightIndex, stiffness, particleSpacing);
			}

			int down = y - 1;
			if (down >= 0) {
				int downIndex = IndexOf(x, down);
				particleSystem_.AddSpring(i, downIndex, stiffness, particleSpacing);

				// Shear springs
				if (x + 1 < gridWidth_) {
					int downRightIndex = IndexOf(x + 1, down);
					particleSystem_.AddSpring(i, downRightIndex, stiffness, diagonalDist);
				}
				if (x - 1 >= 0) {
					int downLeftIndex = IndexOf(x - 1, down);
					particleSystem_.AddSpring(i, downLeftIndex, stiffness, diagonalDist);
				}
			}
			}

			// Flex springs
			{
			int down2 = y - 2;
			if (down2 >= 0)
				particleSystem_.AddSpring(i, IndexOf(x, down2), stiffness, doubleDist);
			int right2 = x + 2;
			if(right2 < gridWidth_)
				particleSystem_.AddSpring(i, IndexOf(right2, y), stiffness, doubleDist);
			}
		}}


		std::unique_ptr<IndexArray> indicesPTR = make_unique<IndexArray>();
		auto& indices = *indicesPTR.get();
		
		// Line rendering
		/*
		renderer.SetDrawMode(DrawMode::Lines);
		for (int x = 0; x < gridWidth_; x++) { for (int y = 0; y < gridHeight_; y++) {
			int i = IndexOf(x, y);

			int right = x + 1;
			if (right < gridWidth_) {
				int rightIndex = IndexOf(right, y);
				indices.push_back(i);
				indices.push_back(rightIndex);
			}

			int down = y - 1;
			if (down >= 0) {
				int downIndex = IndexOf(x, down);
				indices.push_back(i);
				indices.push_back(downIndex);
			}
		}}*/

		
		// Triangulate : CCW winding -> 0, 2, 1,  1, 2, 3
		// 0-1
		// |/|
		// 2-3
		
		for (int x = 0; x < gridWidth_ - 1; x++) {
			for (int y = 0; y < gridHeight_ - 1; y++) {
				int p0 = IndexOf(x, y),
					p1 = IndexOf(x + 1, y),
					p2 = IndexOf(x, y + 1),
					p3 = IndexOf(x + 1, y + 1);
				int ps[6] = { p0, p2, p1, p1, p2, p3 };
				for (int i = 0;i < 6;i++)
					indices.push_back(ps[i]);
			}
		}

		const auto& tris = indices;
		UpdateVertexPositions(tris);
		mesh_->UpdateIndices(std::move(indicesPTR));
		RenderingComponent& renderer = CreateComponent<RenderingComponent>(std::move(mesh));
		auto material = std::make_shared<Material>(Material::GetDefault());
		auto shader = std::make_shared<PhongShader>();
		CreateComponent<MaterialComponent>(material);
		CreateComponent<ShadingComponent>(shader);

		// Sphere for interactions
		auto sphereNode = make_unique<SceneNode>();
		// Use  -0.03 to avoid clipping
		sphereNode->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(sphereRadius_ - 0.03, 64, 64));
		sphereNode->CreateComponent<MaterialComponent>(material);
		sphereNode->CreateComponent<ShadingComponent>(shader);

		sphereNode->GetTransform().SetPosition(spherePos_);
		sphereNode_ = sphereNode.get();
		AddChild(std::move(sphereNode));
	}

	void ClothNode::Update(double dt) {
		remainingStep_ += dt;

		//printf("Remaining step: %f\nDelta time: %f\n\n", remainingStep_, delta_time);
		while (remainingStep_ > integrationStep_) {
			particleState_ = integrator_->Integrate(particleSystem_, particleState_, 0, integrationStep_);
			remainingStep_ -= integrationStep_;
			
			
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
		}
		
		auto tris = mesh_->GetIndices();
		UpdateVertexPositions(tris);
		sphereNode_->GetTransform().SetPosition(spherePos_);
	}

	void ClothNode::UpdateVertexPositions(const IndexArray& tris) {
		std::unique_ptr<PositionArray> vertices = make_unique<PositionArray>();
		for (int i = 0; i < particleState_.positions.size(); i++) {
			glm::vec3 pos = particleState_.positions[i];
			//printf("%f, %f, %f\n", pos.x, pos.y, pos.z);
			vertices->push_back(pos);
		}
		mesh_->UpdatePositions(std::move(vertices));
		auto& verts = mesh_->GetPositions();
		mesh_->UpdateNormals(std::move(CalcNormals(verts, tris)));
	}

	int ClothNode::IndexOf(int x, int y){
		return gridWidth_ * y + x;
	}
}