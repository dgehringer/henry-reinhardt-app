//
// Created by Dominik Gehringer on 15.10.24.
//

#ifndef HR_H
#define HR_H
#include "helpers.h"
#include "optimize.h"
#include "pchip.h"
#include "pchip.h"
#include "pchip.h"
#include "spline.h"

namespace hr {
    using namespace hr::core;


    template<class T>
    class Intergrowth {
    public:
        using vec_t = typename cubic_spline<T>::vec_t;
        const cubic_spline<T> initial;


        template<Guarantee ... Guarantees>
        static Result<Intergrowth> from_step_function(step_function<T> const &step_function, T starting_grade,
                                                      T final_grade) {
            const Result<point_bound_list<T> > p_and_b = points_and_bounds<T, Guarantees...>(
                step_function, starting_grade, final_grade);
            if (!result_ok(p_and_b)) return result_error(p_and_b);
            auto break_points = compute_break_points<T>(result_value(p_and_b));
            auto initial_spline = core::pchip_spline<T, Monotonous, SufficientLength>(std::move(break_points));

            return Intergrowth(std::move(initial_spline), std::move(result_value(p_and_b)), std::move(break_points));
        }

        void optimize() {
            optimization_helper<T>{}.optimize(_break_points, _points_and_bounds);
        }

    private:
        const point_list<T> _break_points;
        const point_bound_list<T> _points_and_bounds;
        std::optional<cubic_spline<T> > _optimized;

        explicit Intergrowth(cubic_spline<T> &&initial, point_bound_list<T> &&points_and_bounds,
                             point_list<T> &&break_points) : initial(initial), _points_and_bounds(points_and_bounds),
                                                             _break_points(break_points),
                                                             _optimized(std::nullopt) {
        }


    };
}

#endif //HR_H
