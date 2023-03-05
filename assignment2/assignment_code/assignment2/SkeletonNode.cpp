#include "SkeletonNode.hpp"

#include "gloo/utils.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/MeshLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/shaders/SimpleShader.hpp"

namespace GLOO {
SkeletonNode::SkeletonNode(const std::string& filename)
    : SceneNode(), draw_mode_(DrawMode::Skeleton) {
  LoadAllFiles(filename);
  DecorateTree();
  ToggleDrawMode();
  // Force initial update.
  OnJointChanged();
}

void SetActiveRecursive(bool newState, SceneNode* node) {
    node->SetActive(newState);
    for (int i = 0; i < node->GetChildrenCount(); i++) {
        SetActiveRecursive(newState, &node->GetChild(i));
    }
}

void SkeletonNode::ToggleDrawMode() {
  draw_mode_ =
      draw_mode_ == DrawMode::Skeleton ? DrawMode::SSD : DrawMode::Skeleton;
  // TODO: implement here toggling between skeleton mode and SSD mode.
  // The current mode is draw_mode_;
  // Hint: you may find SceneNode::SetActive convenient here as
  // inactive nodes will not be picked up by the renderer.
  switch (draw_mode_) {
  case DrawMode::Skeleton:
      SetActiveRecursive(true, jointNodes[0]);
      ssdMesh->SetActive(false);
      break;
  case DrawMode::SSD:
      ssdMesh->SetActive(true);
      SetActiveRecursive(false, jointNodes[0]);
      break;
  }
}

void SkeletonNode::DecorateTree() {
  // TODO: set up addtional nodes, add necessary components here.
  // You should create one set of nodes/components for skeleton mode
  // (spheres for joints and cylinders for bones), and another set for
  // SSD mode (you could just use a single node with a RenderingComponent
  // that is linked to a VertexObject with the mesh information. Then you
  // only need to update the VertexObject - updating vertex positions and
  // recalculating the normals, etc.).

  // The code snippet below shows how to add a sphere node to a joint.
  // Suppose you have created member variables shader_ of type
  // std::shared_ptr<PhongShader>, and sphere_mesh_ of type
  // std::shared_ptr<VertexObject>.
  // Here sphere_nodes_ptrs_ is a std::vector<SceneNode*> that stores the
  // pointer so the sphere nodes can be accessed later to change their
  // positions. joint_ptr is assumed to be one of the joint node you created
  // from LoadSkeletonFile (e.g. you've stored a std::vector<SceneNode*> of
  // joint nodes as a member variable and joint_ptr is one of the elements).
  //
  // auto sphere_node = make_unique<SceneNode>();
  // sphere_node->CreateComponent<ShadingComponent>(shader_);
  // sphere_node->CreateComponent<RenderingComponent>(sphere_mesh_);
  // sphere_nodes_ptrs_.push_back(sphere_node.get());
  // joint_ptr->AddChild(std::move(sphere_node));
}

void SkeletonNode::Update(double delta_time) {
  // Prevent multiple toggle.
  static bool prev_released = true;
  if (InputManager::GetInstance().IsKeyPressed('S')) {
    if (prev_released) {
      ToggleDrawMode();
    }
    prev_released = false;
  } else if (InputManager::GetInstance().IsKeyReleased('S')) {
    prev_released = true;
  }
}

void SkeletonNode::OnJointChanged() {
  // TODO: this method is called whenever the values of UI sliders change.
  // The new Euler angles (represented as EulerAngle struct) can be retrieved
  // from linked_angles_ (a std::vector of EulerAngle*).
  // The indices of linked_angles_ align with the order of the joints in .skel
  // files. For instance, *linked_angles_[0] corresponds to the first line of
  // the .skel file.
    //printf("Linked angles is size %d", linked_angles_.size());

    if (linked_angles_.size() == 0)
        return;
    int numVerts = srcVerts.size();
    auto newVerts = make_unique<PositionArray>(numVerts);
    //std::fill(newVerts->begin(), newVerts->begin() + numVerts, glm::vec3());
    
    for (int i = 0; i < linked_angles_.size(); i++) {
        EulerAngle& angle = *linked_angles_[i];
        glm::quat rotation(glm::vec3(angle.rx, angle.ry, angle.rz));
        Transform& jointTransform = jointNodes[i]->GetTransform();
        jointTransform.SetRotation(rotation);

        //Ignore root transform
        if (i == 0) {
            newVerts->assign(numVerts, glm::vec3());
            continue;
        }

        auto T_Matrix = (jointTransform.GetLocalToWorldMatrix()),
            B_Inverse = toJointBeforeAnimation[i];
        for (int j = 0; j < numVerts; j++) {
            glm::vec4 pos(srcVerts[j], 1.f);
            pos = T_Matrix * B_Inverse * pos;
            float weight = attachmentWeights[j][i-1];
            //printf("pos: %d\n", i);
            (*newVerts)[j] += glm::vec3(pos) * weight;
            //(*newVerts)[j] = pos;
        }
        toWorldAfterAnimation[i] = (jointTransform.GetLocalToWorldMatrix());
    }

    //printf("pos: %d\n", i);
    //printf("New vertices: %d", newVerts->size());
    if (newVerts->size() > 0) {
        auto vtxObj = ssdMesh->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr();
        vtxObj->UpdatePositions(std::move(newVerts));
        CalcNormals(vtxObj);
    }

}

void SkeletonNode::LinkRotationControl(const std::vector<EulerAngle*>& angles) {
  linked_angles_ = angles;
}

void SkeletonNode::LoadSkeletonFile(const std::string& path) {
    std::fstream skeletonFile(path);
    std::vector<int> parentIndices;
    std::vector<glm::vec3> jointPositions;
    for (std::string line; std::getline(skeletonFile, line);) {
        std::stringstream buffer;
        buffer << line;
        glm::vec3 pos;
        buffer >> pos.x;
        buffer >> pos.y;
        buffer >> pos.z;

        int parent_index = 0;
        buffer >> parent_index;
        parentIndices.push_back(parent_index);
        jointPositions.push_back(pos);
    }
    
    static std::shared_ptr<Material> jointBoneMaterial = std::make_shared<Material>(Material::GetDefault());
    static std::shared_ptr<PhongShader> jointBoneShader = std::make_shared<PhongShader>();
    static std::shared_ptr<VertexObject> jointMesh = PrimitiveFactory::CreateSphere(0.02f, 32, 32);
    static std::shared_ptr<VertexObject> boneMesh = PrimitiveFactory::CreateCylinder(0.005f, 1.f, 16);

    for (int i = 0; i < jointPositions.size(); i++) {
        int parentIndex = parentIndices[i];
        SceneNode* parent = (parentIndex == -1) ? this : (jointNodes[parentIndex]);

        std::unique_ptr<SceneNode> joint = make_unique<SceneNode>();
        Transform& jointTransform = joint->GetTransform();
        glm::vec3 position = jointPositions[i];
        jointTransform.SetPosition(position);

        //Spheres
        auto jointVisual = make_unique<SceneNode>();
        jointVisual->CreateComponent<RenderingComponent>(jointMesh);
        jointVisual->CreateComponent<ShadingComponent>(jointBoneShader);
        jointVisual->CreateComponent<MaterialComponent>(jointBoneMaterial);

        joint->AddChild(std::move(jointVisual));
        
        if (parentIndex != -1) {
            //Joint rotation
            
            

            //Bone cylinders
            auto boneVisual = make_unique<SceneNode>();
            boneVisual->CreateComponent<RenderingComponent>(boneMesh);
            boneVisual->CreateComponent<ShadingComponent>(jointBoneShader);
            boneVisual->CreateComponent<MaterialComponent>(jointBoneMaterial);

            float boneLength = glm::length(position);
            Transform& boneTransform = boneVisual->GetTransform();
            glm::vec3 boneScale = boneTransform.GetScale();
            boneScale.y = boneLength;
            boneTransform.SetScale(boneScale);
            glm::vec3 lookDir = -glm::normalize(position);
            glm::quat rotation = glm::quatLookAt(lookDir, glm::vec3(0.f, 1.f, 0.f));
            
            //Rotate 90 degrees on x axis (+y -> +z)
            //boneTransform.SetRotation(glm::vec3(1.f, 0.f, 0.f), glm::half_pi<float>());
            glm::quat yToZRotation(glm::vec3(glm::half_pi<float>(), 0.f, 0.f));
            boneTransform.SetRotation(rotation * yToZRotation);
            //glm::quat yToZRotation();

            parent->AddChild(std::move(boneVisual));
        }

        SceneNode* jointNode = joint.get();
        // Put joint in joints list
        jointNodes.push_back(jointNode);
        parent->AddChild(std::move(joint));
        
        auto mat = jointNode->GetTransform().GetLocalToWorldMatrix();
        toJointBeforeAnimation.push_back(glm::inverse(mat));
        toWorldAfterAnimation.push_back(mat);
    }

}

void SkeletonNode::CalcNormals(VertexObject* mesh) {
    auto pos = mesh->GetPositions();
    auto tris = mesh->GetIndices();

    std::unique_ptr<PositionArray> normals = make_unique<PositionArray>();
    
    // Init normals to 0
    for (int i = 0; i < pos.size(); i++) {
        normals->push_back(glm::vec3(0.f));
    }

    // Add face normals to adjacent vertices, weighted by triangle area
    for (int i = 0; i < tris.size(); i += 3) {
        int aI = tris[i],
            bI = tris[i+1],
            cI = tris[i+2];
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
    mesh->UpdateNormals(std::move(normals));
}

void SkeletonNode::LoadMeshFile(const std::string& filename) {
    std::shared_ptr<VertexObject> vtx_obj =
        MeshLoader::Import(filename).vertex_obj;
        //PrimitiveFactory::CreateCylinder(0.005f, 1.f, 16);

    //Store initial positions
    srcVerts = vtx_obj->GetPositions();
    CalcNormals(vtx_obj.get());

    std::unique_ptr<SceneNode> meshNode = make_unique<SceneNode>();
    meshNode->CreateComponent<RenderingComponent>(vtx_obj);

    std::shared_ptr<Material> meshMaterial = std::make_shared<Material>(Material::GetDefault());
    std::shared_ptr<ShaderProgram> meshShader = std::make_shared<PhongShader>();


    meshNode->CreateComponent<MaterialComponent>(meshMaterial);
    meshNode->CreateComponent<ShadingComponent>(meshShader);
      
    ssdMesh = meshNode.get();
    AddChild(std::move(meshNode));
}

void SkeletonNode::LoadAttachmentWeights(const std::string& path) {
    std::fstream skeletonFile(path);
    
    for (std::string line; std::getline(skeletonFile, line);) {
        std::vector<float> matrixRow;
        std::stringstream rowBuffer;
        rowBuffer << line;
        float weight;
        while (rowBuffer >> weight)
        {
            matrixRow.push_back(weight);
            //printf("%f, ", weight);
        }
        //printf("\n");

        attachmentWeights.push_back(matrixRow);
    }
}

void SkeletonNode::LoadAllFiles(const std::string& prefix) {
  std::string prefix_full = GetAssetDir() + prefix;
  LoadSkeletonFile(prefix_full + ".skel");
  LoadMeshFile(prefix + ".obj");
  LoadAttachmentWeights(prefix_full + ".attach");
}
}  // namespace GLOO
