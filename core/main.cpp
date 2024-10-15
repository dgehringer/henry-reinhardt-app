
#include <iostream>
#include <ranges>
#include <vector>
#include <LBFGSB.h>
#include "hr.h"
#include "optimize.h"

namespace views = std::ranges;


using namespace hr;
using namespace hr::core;


int main() {
  std::cout << "Hello, World!" << std::endl;;
  const auto points = std::get<point_bound_list<double>>(points_and_bounds(step_function<double>{
                                    0.2, {
                                      {0.4, 0.3},
                                      {0.5, 0.5},
                                      {0.8, 0.8}
                                    }
                                  }, 0.125, 0.92));
  Intergrowth<double> inter(points);

  return 0;
}
