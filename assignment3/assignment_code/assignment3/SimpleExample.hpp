#ifndef SIMPLE_EXAMPLE_NODE_H_
#define SIMPLE_EXAMPLE_NODE_H_

#include "gloo/SceneNode.hpp"
#include "ParticleState.hpp"
#include "ForwardEulerIntegrator.hpp"
#include "ParticleSystemBase.hpp"

namespace GLOO {
    class Eqn1ParticleSystem : public ParticleSystemBase {
    public:
        //~Eqn1ParticleSystem() override;
        Eqn1ParticleSystem() {}
        ParticleState ComputeTimeDerivative(const ParticleState& state,
            float time) const override;
    };
    typedef IntegratorBase<Eqn1ParticleSystem, ParticleState> SimpleIntegrator;

    class SimpleExample : public SceneNode{
        public:
            void Update(double delta_time) override;
            SimpleExample(std::unique_ptr<SimpleIntegrator> integrator, double integrationStep);

        private:
            //std::vector<SceneNode*> particles_;
            double integrationStep_;
            double remainingStep_;
            Eqn1ParticleSystem particleSystem_;
            ParticleState particleState_;
            SceneNode* particle_;
            std::unique_ptr<SimpleIntegrator> integrator_;
    };
}  // namespace GLOO

#endif
