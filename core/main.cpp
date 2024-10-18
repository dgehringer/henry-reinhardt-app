
#include <iostream>
#include <ranges>
#include <vector>

#include "hr.h"
#include "optimize.h"

namespace views = std::ranges;

using namespace hr;
using namespace hr::core;

template <class T> std::string to_list(std::vector<T> const &values) {
  if (values.empty()) return "[]";
  std::stringstream ss;
  ss << "[";
  for (auto i = 0; i < values.size() - 1; ++i) ss << fmt::format("{}, ", values[i]);
  ss << fmt::format("{}]", values.back());
  return ss.str();
}


template <class T> point_list<T> step_function_points(step_function<T> const &step_function) {
  auto error = validate_step_function<T>(step_function);
  assert(!error.has_value());
  const auto [first_grade, points] = step_function;
  T zero{0.0};
  point_list<T> result{
      {zero, zero}, {first_grade, zero}, {first_grade, std::get<Yield>(points.front())}};
  for (auto i = 0; i < points.size() - 1; ++i) {
    auto [g1, y1] = points[i];
    auto [g2, y2] = points[i + 1];
    result.push_back({g1, y1});
    result.push_back({g1, y2});
  }
  result.push_back(points.back());
  result.push_back({std::get<Grade>(points.back()), T{1}});
  result.push_back({T{1}, T{1}});
  return result;
}

template <class T>
std::string scatter(point_list<T> const &list, std::string &&color = "r", double alpha = 1.0) {
  auto [x, y] = transpose(list);
  return fmt::format("plt.scatter({}, {}, color=\"{}\", alpha={})\n", to_list(x), to_list(y), color,
                     alpha);
}

template <class T> std::string plot_spline(cubic_spline<T> const &spline, std::size_t npoints = 100,
                                           std::string &&color = "k", double alpha = 1.0) {
  auto bp = spline.x;
  const auto [start, end] = std::minmax_element(bp.begin(), bp.end());

  auto x = linspace<T>(*start, *end, npoints);
  return fmt::format("plt.plot({}, {}, color=\"{}\", alpha={})\n{}", to_list(x),
                     to_list(result_value(spline(x))), color, alpha,
                     scatter(spline.break_points(), std::move(color), alpha));
}

template <class T> std::string plot_step_function(step_function<T> const &step_function) {
  const auto [stepg, stepy] = transpose(step_function_points(step_function));
  return fmt::format("plt.plot({}, {}, color=\"b\")\n", to_list(stepg), to_list(stepy));
}

template <class T>
void plot_hr(step_function<T> const &step_function, T starting_grade, T final_grade) {
  step_function_points(step_function);
  const auto [splineg, spliney] = transpose(
      result_value(compute_break_points<T>(step_function, starting_grade, final_grade)));

  auto intergrowth = result_value(
      Intergrowth<T>::from_step_function(step_function, starting_grade, final_grade));

  intergrowth.optimized();

  std::stringstream ss;
  ss << "from matplotlib import pyplot as plt\n";
  ss << plot_step_function(step_function);
  ss << plot_spline(intergrowth.initial, 20, "k", 0.25);
  ss << plot_spline(intergrowth.optimized(), 20, "k");
  ss << "plt.xlim(0, 1)\n";
  ss << "plt.ylim(0, 1)\n";
  ss << "plt.show()\n";

  std::cout << ss.str();
}

int main() {
  const step_function<double> step_function{0.2, {{0.4, 0.3}, {0.5, 0.5}, {0.8, 0.8}}};
  const double starting_grade{0.125}, final_grade{0.925};
  plot_hr(step_function, starting_grade, final_grade);
  return 0;
}
