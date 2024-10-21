
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
  T zero{0.0};
  point_list<T> result{{zero, zero}};
  for (auto i = 0; i < step_function.size() - 1; ++i) {
    auto [g1, y1] = step_function[i];
    auto [g2, y2] = step_function[i + 1];
    result.push_back({g1, y1});
    result.push_back({g1, y2});
  }
  result.push_back(step_function.back());
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
  ss << plot_spline(intergrowth.initial, 100, "k", 0.25);
  ss << plot_spline(intergrowth.optimized(), 100, "k");
  ss << "plt.xlim(0, 1)\n";
  ss << "plt.ylim(1, 0)\n";
  ss << "plt.show()\n";

  std::cout << ss.str();
}

int main() {
  const step_function<double> step_function{
      {0.03431372549019608, 0.0},   {0.08986928104575163, 0.4016},
      {0.2042483660130719, 0.5744}, {0.3627450980392157, 0.63519999999999996},
      {0.4705882352941177, 0.68},   {0.5800653594771242, 0.7184},
      {0.6781045751633987, 0.76},   {0.7679738562091504, 0.808},
      {0.8643790849673202, 0.8784}, {0.8643790849673202, 1.0}};
  const double starting_grade{0.0163}, final_grade{0.8912};
  plot_hr(step_function, starting_grade, final_grade);

  //plot_hr({{0.25, 0.0}, {0.5, 0.5}, {0.75, 0.75}, {0.75, 1.0}}, 0.125, 0.825);
  return 0;
}
