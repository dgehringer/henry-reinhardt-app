//
// Created by Dominik Gehringer on 15.10.24.
//

#ifndef HR_H
#define HR_H
#include "helpers.h"
#include "optimize.h"
#include "pchip.h"
#include "spline.h"

namespace hr {
    using namespace hr::core;

    template<class T>
    point_list<T> extract_points(point_bound_list<T> const &p_and_b) {
        point_list<T> result(p_and_b.size());
        std::transform(
            p_and_b.begin(),
            p_and_b.end(),
            result.begin(),
            [](auto &&p) { return std::get<0>(p); }
        );
        return result;
    }

    template<class T>
    class Intergrowth {
    public:
        using vec_t = typename cubic_spline<T>::vec_t;
        cubic_spline<T> initial;


        explicit Intergrowth(point_bound_list<T> const &p_and_b) : _points_and_bounds(p_and_b),
                                                          initial(pchip_spline<T, SufficientLength, Monotonous>(
                                                              extract_points(p_and_b))) {
        }

    private:
        const point_bound_list<T> _points_and_bounds;
    };
}

#endif //HR_H
