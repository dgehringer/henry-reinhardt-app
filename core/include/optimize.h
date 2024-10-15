//
// Created by Dominik Gehringer on 15.10.24.
//

#ifndef OPTIMIZE_H
#define OPTIMIZE_H
#include "spline.h"

namespace hr::core {

    template<class T>
    point_bound_list<T> defaults_and_bounds(point_list<T> const &points, T starting_grade, T final_grade) {
        point_bound_list<T> points_and_bounds;
        points_and_bounds.reserve(points.size() * 2 - 2);
        for (auto i = 1; i < points.size(); ++i) {
            auto [g1, y1] = points[i - 1];
            auto [g2, y2] = points[i];
            point<T> vi{g1, 0.5 * (y1 + y2)};
            if (i > 1) points_and_bounds.push_back(std::make_tuple(vi, point<T>{y1, y2}, DOF::Vertical));
            T hi_grade;
            if (i == 1) hi_grade = starting_grade;
            else if (i == points.size() - 1) hi_grade = final_grade;
            else hi_grade = 0.5 * (g1 + g2);;
            points_and_bounds.push_back(std::make_tuple(point<T>{hi_grade, y2}, point<T>{g1, g2}, DOF::Horizontal));
        }

        return points_and_bounds;
    }

    template<class T>
    std::optional<interpolation_error> is_monotonous(point_list<T> const &points) {
        for (auto i = 0; i < points.size() - 1; ++i) {
            auto [g1, y1] = points[i];
            auto [g2, y2] = points[i + 1];
            if (g2 <= g1)
                return std::make_optional(core::interpolation_error{
                    Monotonous, "grades are not monotonous"
                });
            if (y2 <= y1)
                return std::make_optional(core::interpolation_error{
                    Monotonous, "yields are not monotonous"
                });
        }
        return std::nullopt;
    }

    template<class T>
    Result<point_bound_list<T> > points_and_bounds(step_function<T> const &func, T starting_grade, T final_grade) {
        auto [first_grade, points] = func;
        if (points.empty()) return interpolation_error{core::SufficientLength, "points are empty"};

        point_list<T> all_points(points);

        all_points.reserve(points.size() + 4);
        all_points.emplace(all_points.begin(), std::make_pair(first_grade, 0));

        auto _is_monotonous = is_monotonous(all_points);
        if (_is_monotonous.has_value()) return _is_monotonous.value();

        all_points.emplace(all_points.begin(), std::make_pair(0, 0));
        all_points.emplace_back(std::make_pair(1, 1));

        return defaults_and_bounds(all_points, starting_grade, final_grade);
    }

}

#endif //OPTIMIZE_H
