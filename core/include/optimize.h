//
// Created by Dominik Gehringer on 15.10.24.
//

#ifndef OPTIMIZE_H
#define OPTIMIZE_H
#include "spline.h"

namespace hr::core {
    template<class T>
    point_list<T> extract_points(point_bound_list<T> const &p_and_b) {
        point_list<T> result(p_and_b.size());
        std::transform(
            p_and_b.begin(),
            p_and_b.end(),
            result.begin(),
            [](auto &&p) { return std::get<Grade>(p); }
        );
        return result;
    }

    template<class T>
    point_bound_list<T> defaults_and_bounds(point_list<T> const &points, T starting_grade, T final_grade) {
        point_bound_list<T> points_and_bounds;
        points_and_bounds.reserve(points.size() * 2 - 2);
        for (auto i = 1; i < points.size(); ++i) {
            auto [g1, y1] = points[i - 1];
            auto [g2, y2] = points[i];
            point<T> vi{g1, 0.5 * (y1 + y2)};
            if (i > 1) points_and_bounds.push_back(std::make_tuple(vi, point<T>{y1, y2}, Vertical));
            T hi_grade;
            if (i == 1) hi_grade = starting_grade;
            else if (i == points.size() - 1) hi_grade = final_grade;
            else hi_grade = 0.5 * (g1 + g2);;
            points_and_bounds.push_back(std::make_tuple(point<T>{hi_grade, y2}, point<T>{g1, g2}, Horizontal));
        }

        return points_and_bounds;
    }


    template<class T, Guarantee ... Guarantees>
    std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous), std::nullopt_t,
        std::optional<interpolation_error> > validate_step_function(step_function<T> const &step_function) {
        auto [first_grade, points] = step_function;

        if constexpr (!is_guaranteed<Guarantees...>(SufficientLength)) {
            if (points.empty()) return interpolation_error{SufficientLength, "no points in step function"};
        }
        if constexpr (is_guaranteed<Guarantees...>(InBounds)) {
            if (!in_unit_interval(first_grade))
                return interpolation_error{
                    InBounds, "first grade not in unit interval"
                };
            for (auto point: points) {
                if (!in_unit_interval(point.first) || !in_unit_interval(point.second))
                    return interpolation_error{
                        InBounds, "point not in unit interval"
                    };
            };
        }
        if constexpr (is_guaranteed<Guarantees...>(Monotonous)) {
            point_list<T> all_points(points);
            all_points.emplace(all_points.begin(), std::make_pair(first_grade, 0));
            for (auto i = 0; i < all_points.size() - 1; ++i) {
                auto [g1, y1] = all_points[i];
                auto [g2, y2] = all_points[i + 1];
                if (g2 <= g1)
                    return std::make_optional(core::interpolation_error{
                        Monotonous, "grades are not monotonous"
                    });
                if (y2 <= y1)
                    return std::make_optional(core::interpolation_error{
                        Monotonous, "yields are not monotonous"
                    });
            }
        }
        return std::nullopt;
    }


    template<class T, Guarantee ... Guarantees>
    std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous), point_bound_list<T>,
        Result<
            point_bound_list<T> > > points_and_bounds(step_function<T> const &func, T starting_grade, T final_grade) {
        if constexpr (!all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous)) {
            auto result = validate_step_function<T, Guarantees...>(func);
            if (result.has_value()) return result.value();
        }
        auto [first_grade, points] = func;
        point_list<T> all_points(points);
        all_points.reserve(points.size() + 3);
        all_points.emplace(all_points.begin(), std::make_pair(first_grade, 0));
        all_points.emplace(all_points.begin(), std::make_pair(0, 0));
        all_points.emplace_back(std::make_pair(1, 1));
        return defaults_and_bounds(all_points, starting_grade, final_grade);
    }

    template<class T, Guarantee ... Guarantees>
    std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous), point_list<T>,
        Result<point_list<T> > > compute_break_points(step_function<T> const & step_function, T starting_grade, T final_grade) {
        if constexpr (!all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous)) {
            auto p_and_b = points_and_bounds<T, Guarantees...>(step_function, starting_grade, final_grade);
            return extract_points(std::get<point_bound_list<T> >(p_and_b));
        } else {
            return extract_points(points_and_bounds(step_function, starting_grade, final_grade));
        }
    }

    template<class T>
    point_list<T> compute_break_points(point_bound_list<T> const& p_and_b) {
        return extract_points(p_and_b);
    }
}

#endif //OPTIMIZE_H
