#ifndef INTEGRATOR_FACTORY_H_
#define INTEGRATOR_FACTORY_H_

#include "IntegratorBase.hpp"
#include "TrapezoidalIntegrator.hpp"
#include "RK4Integrator.hpp"

#include <stdexcept>

#include "gloo/utils.hpp"

#include "IntegratorType.hpp"
#include "ForwardEulerIntegrator.hpp"

namespace GLOO {
class IntegratorFactory {
 public:
  template <class TSystem, class TState>
    static std::unique_ptr<IntegratorBase<TSystem, TState>> CreateIntegrator(
        IntegratorType type) {
        switch (type) {
            case IntegratorType::Euler:
                return make_unique<ForwardEulerIntegrator<TSystem, TState>>();
                break;
            case IntegratorType::Trapezoidal:
                return make_unique<TrapezoidalIntegrator<TSystem, TState>>();
                break;
            case IntegratorType::RK4:
                return make_unique<RK4Integrator<TSystem, TState>>();
                break;
            default:
                return nullptr;
                break;
        }

    }
};
}  // namespace GLOO

#endif
