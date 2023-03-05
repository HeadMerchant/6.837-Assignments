#pragma once
#ifndef RK4_INTEGRATOR_H_
#define RK4_INTEGRATOR_H_

#include "IntegratorBase.hpp"

namespace GLOO {
    template <class TSystem, class TState>
    class RK4Integrator : public IntegratorBase<TSystem, TState> {
    public:
        TState Integrate(const TSystem& system,
            const TState& state,
            float start_time,
            float dt) const override {

            TState k1 = system.ComputeTimeDerivative(state, start_time);
            TState k2 = system.ComputeTimeDerivative(state + .5*dt*k1, start_time+(.5*dt));
            TState k3 = system.ComputeTimeDerivative(state + .5*dt*k2, start_time+(.5*dt));
            TState k4 = system.ComputeTimeDerivative(state + dt*k3, start_time+dt);
            return state + (1./6. * dt) * (k1 + 2*k2 + 2*k3 + k4);
        }
    };
}  // namespace GLOO

#endif
