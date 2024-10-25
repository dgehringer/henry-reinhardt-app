//
// Created by Dominik Gehringer on 15.10.24.
//

#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include <cppoptlib/solver/lbfgsbsolver.h>
#include <numeric>

#include "pchip.h"
#include "spline.h"

namespace hr::core {
  template <class T> using vector_t = typename cubic_spline<T>::vec_t;

  template <class T> point_list<T> extract_points(point_bound_list<T> const &p_and_b) {
    point_list<T> result(p_and_b.size());
    std::transform(p_and_b.begin(), p_and_b.end(), result.begin(),
                   [](auto &&p) { return std::get<Grade>(p); });
    return result;
  }

  template <class T> point_bound_list<T> defaults_and_bounds(point_list<T> const &points,
                                                             T starting_grade, T final_grade) {
    point_bound_list<T> points_and_bounds;
    points_and_bounds.reserve(points.size() * 2 - 2);
    for (auto i = 1; i < points.size()-1; ++i) {
      auto [g1, y1] = points[i - 1];
      auto [g2, y2] = points[i];
      point<T> vi{g1, 0.5 * (y1 + y2)};
      if (i > 1) points_and_bounds.push_back(std::make_tuple(vi, point<T>{y1, y2}, Vertical));
      T hi_grade;
      if (i == 1)
        hi_grade = starting_grade;
      else if (i == points.size() - 2)
        hi_grade = final_grade;
      else
        hi_grade = 0.5 * (g1 + g2);
      points_and_bounds.push_back(
          std::make_tuple(point<T>{hi_grade, y2}, point<T>{g1, g2}, Horizontal));
    }
    return points_and_bounds;
  }

  template <class T, Guarantee... Guarantees>
  std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous),
                     std::nullopt_t, std::optional<interpolation_error> >
  validate_step_function(step_function<T> const &step_function) {
    if constexpr (!is_guaranteed<Guarantees...>(SufficientLength)) {
      if (step_function.size() < 3)
        return interpolation_error{SufficientLength, "no points in step function"};
    }
    if constexpr (is_guaranteed<Guarantees...>(InBounds)) {
      for (auto point : step_function) {
        if (!in_unit_interval(point.first) || !in_unit_interval(point.second))
          return interpolation_error{InBounds, "point not in unit interval"};
      };
    }
    if constexpr (is_guaranteed<Guarantees...>(Monotonous)) {
      point_list<T> all_points(step_function);
      for (auto i = 0; i < all_points.size() - 1; ++i) {
        auto [g1, y1] = all_points[i];
        auto [g2, y2] = all_points[i + 1];
        if (g2 <= g1)
          return std::make_optional(
              core::interpolation_error{Monotonous, "grades are not monotonous"});
        if (y2 <= y1)
          return std::make_optional(
              core::interpolation_error{Monotonous, "yields are not monotonous"});
      }
    }
    return std::nullopt;
  }

  template <class T, Guarantee... Guarantees>
  std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous),
                     point_bound_list<T>, Result<point_bound_list<T> > >
  points_and_bounds(step_function<T> const &func, T starting_grade, T final_grade) {
    if constexpr (!all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous)) {
      auto result = validate_step_function<T, Guarantees...>(func);
      if (result.has_value()) return result.value();
    }
    point_list<T> all_points(func);
    all_points.reserve(func.size() + 2);
    all_points.emplace(all_points.begin(), std::make_pair(0, 0));
    all_points.emplace_back(std::make_pair(1, 1));
    return defaults_and_bounds(all_points, starting_grade, final_grade);
  }

  template <class T, Guarantee... Guarantees>
  std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous),
                     point_list<T>, Result<point_list<T> > >
  compute_break_points(step_function<T> const &step_function, T starting_grade, T final_grade) {
    if constexpr (!all_guaranteed<Guarantees...>(SufficientLength, InBounds, Monotonous)) {
      auto p_and_b
          = points_and_bounds<T, Guarantees...>(step_function, starting_grade, final_grade);
      return extract_points(std::get<point_bound_list<T> >(p_and_b));
    } else {
      return extract_points(points_and_bounds(step_function, starting_grade, final_grade));
    }
  }

  template <class T> point_list<T> compute_break_points(point_bound_list<T> const &p_and_b) {
    return extract_points(p_and_b);
  }

  inline DOF other(DOF value) { return value == Horizontal ? Vertical : Horizontal; }

  template <class T> class ObjectiveFunction : public cppoptlib::BoundedProblem<T> {
  public:
    using Superclass = cppoptlib::BoundedProblem<T>;

    ObjectiveFunction(const point_bound_list<T> &p_and_b, const vector_t<T> lb, vector_t<T> ub)
        : Superclass(lb, ub), _points_and_bounds(p_and_b) {}

    T value(const vector_t<T> &x) {
      const auto fx = objective_values(assemble_spline(x));
      return fx.array().abs().sum();
    }

    cubic_spline<T> assemble_spline(vector_t<T> const &input) const {
      assert(input.size() == _points_and_bounds.size() - 2);
      point_list<T> break_points{std::get<0>(_points_and_bounds.front())};
      break_points.reserve(_points_and_bounds.size());
      for (auto i = 1; i < _points_and_bounds.size() - 1; ++i) {
        const auto [p, _, dof] = _points_and_bounds[i];
        T value{input(i - 1)};
        break_points.push_back(dof == Horizontal ? point<T>{value, std::get<Yield>(p)}
                                                 : point<T>{std::get<Grade>(p), value});
      }
      break_points.push_back(std::get<0>(_points_and_bounds.back()));
      return pchip_spline<T, Monotonous, SufficientLength>(std::move(break_points));
    }

  private:
    point_bound_list<T> _points_and_bounds;

    vector_t<T> objective_values(cubic_spline<T> const &spline) const {
      const quartic_spline<T> antiderivative = spline.antiderivative();

      const auto integrate = [&](point<T> const &a, point<T> const &b) {
        return spline.template integrate<InBounds>(std::get<Grade>(a), std::get<Grade>(b),
                                                   antiderivative);
      };

      std::vector<T> diffs;
      for (auto i = 1; i < _points_and_bounds.size() - 1; ++i) {
        auto [p, lp, dofp] = _points_and_bounds[i - 1];
        auto [q, lq, dofq] = _points_and_bounds[i];
        auto [r, lr, dofr] = _points_and_bounds[i + 1];
        if (dofq == Horizontal) continue;
        assert(dofq == other(dofp));
        assert(dofq == other(dofr));
        auto lever_left{std::get<Grade>(q) - std::get<Grade>(p)},
            lever_right{std::get<Grade>(r) - std::get<Grade>(q)};
        auto spline_area_left{integrate(p, q)}, spline_area_right{integrate(q, r)};
        // assert(lever_left > 0 && lever_right > 0);
        // assert(spline_area_left > 0 && spline_area_right > 0);
        {
          T area_left, area_right;
          if (dofq == Vertical) {
            area_left = spline_area_left - std::get<Yield>(p) * lever_left;
            area_right = std::get<Yield>(r) * lever_right - spline_area_right;
          } else {
            area_left = std::get<Yield>(q) * lever_left - spline_area_left;
            area_right = spline_area_right - std::get<Yield>(q) * lever_right;
          }
          // assert(area_left > 0 && area_right > 0);
          diffs.push_back(area_right - area_left);
        }
      }
      return vector_t<T>(stl_to_eigen(std::move(diffs)));
    }
  };

  template <class T> cubic_spline<T> optimize(point_bound_list<T> const &p_and_b,
                                              int max_iterations = 1000, T eps = 1.0e-5) {
    auto [x0, lb, ub] = guess_and_bounds(p_and_b);
    ObjectiveFunction<T> objective(p_and_b, lb, ub);
    cppoptlib::LbfgsbSolver<ObjectiveFunction<T> > solver;
    auto criterion{cppoptlib::Criteria<T>::defaults()};
    criterion.gradNorm = eps;
    criterion.iterations = max_iterations;
    solver.setStopCriteria(criterion);
    solver.minimize(objective, x0);
    return objective.assemble_spline(x0);
  }

  template <class T> std::tuple<vector_t<T>, vector_t<T>, vector_t<T> > guess_and_bounds(
      point_bound_list<T> const &p_and_b) {
    auto length = p_and_b.size() - 2;
    vector_t<T> x0(length), lb(length), ub(length);
    for (auto i = 1; i < p_and_b.size() - 1; ++i) {
      const auto [point, limits, dof_type] = p_and_b[i];
      x0(i - 1) = dof_type == Horizontal ? std::get<Grade>(point) : std::get<Yield>(point);
      auto [lower_limit, upper_limit] = limits;
      auto offset = 0.025 * (upper_limit - lower_limit);
      lb(i - 1) = lower_limit + offset;
      ub(i - 1) = upper_limit - offset;
    }
    return std::make_tuple(x0, lb, ub);
  }
}  // namespace hr::core

#endif  // OPTIMIZE_H
