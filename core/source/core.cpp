//
// Created by Dominik Gehringer on 17.10.24.
//
#include <emscripten/bind.h>

#include "helpers.h"
#include "hr.h"

using namespace emscripten;
using Point = hr::point<double>;
using PointVector = std::vector<Point>;
using Spline = hr::cubic_spline<double>;
using StepFunction = hr::step_function<double>;
using Vector = std::vector<double>;
using Matrix = std::vector<Vector>;
using OptimizationStep = hr::optimization_step<double>;
using IntergrowthFunction = hr::Intergrowth<double>;

Vector evaluate_fast(Spline const& spline, Vector const& values) {
  return spline.template operator()<hr::InBounds, hr::Monotonous>(values);
}

std::optional<Vector> evaluate(Spline const& spline, Vector const& values) {
  auto result = spline.template operator()(values);
  if (std::holds_alternative<hr::interpolation_error>(result)) {
    return std::nullopt;
  }
  return std::get<Vector>(result);
}

Matrix spline_coeffs(Spline const& spline) {
  Matrix matrix;
  matrix.reserve(spline.c.rows());
  for (int i = 0; i < spline.c.rows(); ++i) {
    const auto& row = spline.c.row(i);
    matrix.emplace_back(row.begin(), row.end());
  }
  return matrix;
}

template <class T> auto to_js_array(std::vector<T> const& vector) {
  return val::array(vector.cbegin(), vector.cend());
}

std::optional<IntergrowthFunction> from_step_function(StepFunction const& step,
                                                      double starting_grade, double final_grade) {
  auto result = IntergrowthFunction::from_step_function<>(step, starting_grade, final_grade);
  if (std::holds_alternative<hr::interpolation_error>(result)) {
    return std::nullopt;
  }
  return std::get<IntergrowthFunction>(result);
}

EMSCRIPTEN_BINDINGS(core) {
  value_array<Point>("Point").element(&Point::first).element(&Point::second);

  register_vector<Point>("PointVector").function("toArray", &to_js_array<Point>);
  register_vector<double>("Vector").function("toArray", &to_js_array<double>);
  register_vector<Vector>("Matrix").function("toArray", &to_js_array<Vector>);
  register_vector<OptimizationStep>("OptimizationStepVector").function("toArray", &to_js_array<OptimizationStep>);
  register_optional<Vector>();

  class_<hr::interpolation_error>("InterpolationError")
      .property("code", &hr::interpolation_error::code)
      .property("message", &hr::interpolation_error::message);

  class_<OptimizationStep>("OptimizationStep")
      .property("index", &OptimizationStep::index)
      .property("solution", &OptimizationStep::solution)
      .property("residuals", &OptimizationStep::residuals);

  class_<Spline>("Spline")
      .function("bounds", &Spline::bounds)
      .function("breakPoints", &Spline::break_points)
      .function("evaluateFast", &evaluate_fast)
      .function("evaluate", &evaluate)
      .function("coeffs", &spline_coeffs);

  class_<IntergrowthFunction>("Intergrowth")
      .property("initial", &IntergrowthFunction::initial)
      .class_function("fromStepFunction", &from_step_function)
      .function("optimized", &IntergrowthFunction::optimized)
      .function("history", &IntergrowthFunction::history);

  register_optional<IntergrowthFunction>();

  function("linspace", &hr::linspace<double>);
}