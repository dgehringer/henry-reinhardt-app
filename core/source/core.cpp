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
using ValueVector = std::vector<double>;
using IntergrowthFunction = hr::Intergrowth<double>;

ValueVector evaluate_fast(Spline const& spline, ValueVector const& values) {
  return spline.template operator()<hr::InBounds, hr::Monotonous>(values);
}

std::optional<ValueVector> evaluate(Spline const& spline, ValueVector const& values) {
  auto result = spline.template operator()(values);
  if (std::holds_alternative<hr::interpolation_error>(result)) {
    return std::nullopt;
  }
  return std::get<ValueVector>(result);
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
  register_vector<double>("ValueVector").function("toArray", &to_js_array<double>);
  register_optional<ValueVector>();

  class_<StepFunction>("StepFunction")
      .constructor<double, PointVector>()
      .property("firstGrade", &StepFunction::first)
      .property("points", &StepFunction::second);

  class_<hr::interpolation_error>("InterpolationError")
      .property("code", &hr::interpolation_error::code)
      .property("message", &hr::interpolation_error::message);

  class_<Spline>("Spline")
      .function("bounds", &Spline::bounds)
      .function("breakPoints", &Spline::break_points)
      .function("evaluateFast", &evaluate_fast)
      .function("evaluate", &evaluate);

  class_<IntergrowthFunction>("Intergrowth")
      .property("initial", &IntergrowthFunction::initial)
      .class_function("fromStepFunction", &from_step_function)
      .function("optimized", &IntergrowthFunction::optimized);

  register_optional<IntergrowthFunction>();

  function("linspace", &hr::linspace<double>);
}