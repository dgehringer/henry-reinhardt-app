import enum
import collections
import functools

import numpy as np
from itertools import cycle
from more_itertools import windowed, roundrobin
from scipy.interpolate import PchipInterpolator
from scipy.optimize import minimize as minimize_scipy
from plotly.graph_objs import Figure, Scatter

algorithms = ['L-BFGS-B', 'TNC', 'SLSQP']


hovertemplate = '<b>Grade:</b> %{x:.2f} %<br>' \
                '<b>Yield:</b> %{y:.2f} %<br>'


def empty_figure():
    offset = 3
    fig = Figure()
    fig.update_xaxes(range=[-offset / 3, 100])
    fig.update_yaxes(range=[100 + offset, 0 - offset])
    fig.update_layout(xaxis_title='Grade [%]', yaxis_title='Yield Mass [%]')
    return fig


class Intersection(enum.Enum):
    vertical = 'vertical'
    horizontal = 'horizontal'
    bound = 'bound'


IntersectionPoint = collections.namedtuple('IntersectionPoint', ['x', 'y', 'type'])

make_point = lambda pp: IntersectionPoint(*pp)


def transpose(l): return zip(*l)


def repeat(l: list, times: int = 1, factory: type = list):
    return factory(el for el in l for _ in range(times))


def first(it):
    return next(iter(it))


def last(it, default=StopIteration):
    item = default
    for item in it:
        pass
    if isinstance(item, Exception):
        raise item
    else:
        return item


def get_vertical_intersections(d):
    x, y = map(list, transpose(d))
    y = np.array([0.0] + y)
    return x, ((y[1:] + y[:-1]) / 2.0).tolist()


def get_horizontal_intersections(d):
    x, y = map(list, zip(*d))
    x = np.array(x)
    return ((x[1:] + x[:-1]) / 2.0).tolist(), y[:-1]


def intersections(d):
    point_type = cycle((Intersection.vertical, Intersection.horizontal))
    v, h = map(lambda f: transpose(f(d)), [get_vertical_intersections, get_horizontal_intersections])
    with_type = zip(roundrobin(v, h), point_type)

    def reorder(x):
        (a, b), c = x
        return a, b, c

    return list(map(reorder, with_type))


def initial_bounding_points(d):
    _, y, *_ = d[0]
    left_bound = (0.0, y / 8.0, Intersection.bound)
    x, *_ = d[-1]
    right_bound = ((100.0 + x) / 2.0, 100, Intersection.bound)
    return left_bound, right_bound


def make_bounds(ymass, grade):
    return (0.0, ymass, Intersection.bound), (grade, 100.0, Intersection.bound)


def all_points(d, bounds=None):
    points = intersections(d)
    left_bound, right_bound = bounds or initial_bounding_points(d)
    points.insert(0, left_bound)
    points.append(right_bound)
    return points


def interpolate(points, interpolator=PchipInterpolator):
    x, y, _ = transpose(points)
    spline = interpolator(x, y)
    return spline


def make_step_function_data(d):
    x, y = transpose(d)
    x_ = [0] + repeat(x, 2) + [100]
    y_ = [0, 0] + repeat(y, 2)
    return x_, y_


def plot_henry_reinhardt(d, points=None, spline=None, median_grade=True, float_sink=True):
    x_,  y_ = make_step_function_data(d)
    colormap = {
        Intersection.bound: 'orange',
        Intersection.vertical: 'red',
        Intersection.horizontal: 'green'
    }
    marker_size = {
        Intersection.bound: 12,
        Intersection.horizontal: 8,
        Intersection.vertical: 8
    }
    fig = Figure()
    fig.add_trace(Scatter(x=x_, y=y_, mode='lines', name='step function', opacity=0.5))

    xval, *_ = transpose(points)
    xaxis = np.linspace(np.amin(xval), np.amax(xval), num=200)

    if spline is None:
        spline = interpolate(points)
        fig.add_trace(Scatter(x=xaxis, y=spline(xaxis), name='interpolation', hovertemplate=hovertemplate))

        der_spline = spline.derivative()
        max_val = np.amax(der_spline(xaxis))
        fig.add_trace(Scatter(x=xaxis, y=der_spline(xaxis)/max_val*100, name='derivative', hovertemplate=hovertemplate, visible='legendonly'))

    if median_grade:
        mgx, mgy = calculate_median_grade(points, spline=spline)
        fig.add_trace(Scatter(x=[mgx], y=[mgy], mode='markers', name='median grade',
                              marker=dict(size=[8], color=['black'])))
        fig.add_vline(mgx, line_color='black', line_dash='dash', line_width=0.5)

    if float_sink:
        fs1, fs2 = calculate_float_sink_curve(points, spline=spline, median_grade=mgx)
        fsx1, fsy1 = transpose(fs1)
        fig.add_trace(Scatter(x=fsx1, y=fsy1, marker=dict(color='green'), line=dict(color='green'), mode='lines+markers',
                              hovertemplate=hovertemplate, name='float/sink'))
        fsx2, fsy2 = transpose(fs2)
        fig.add_trace(Scatter(x=fsx2, y=fsy2, marker=dict(color='green'), line=dict(color='green'), mode='lines+markers',
                              hovertemplate=hovertemplate, opacity=0.5, name='float/sink'))

    if points is not None:
        xx, yy, intersection = transpose(points)
        colors = list(map(colormap.get, intersection))
        sizes = list(map(marker_size.get, intersection))
        fig.add_trace(Scatter(x=xx, y=yy, marker=dict(size=sizes, color=colors), mode='markers', name='bounds',
                              hovertemplate=hovertemplate))

    offset = 3
    fig.update_xaxes(range=[-offset / 3, 100])
    fig.update_yaxes(range=[100 + offset, 0 - offset])
    fig.update_layout(xaxis_title='Grade [%]', yaxis_title='Yield Mass [%]', hovermode='closest')
    return fig


def get_bounds(d):
    _, y = next(iter(d))
    x, _ = last(iter(d))
    return (0.0, y), (x, 100)


def all_bounds(p, loffset=0.01, roffset=0.01):
    def bounds_():
        for (plx, ply, _), (_, _, pt), (prx, pry, _) in windowed(p, 3):
            if pt == Intersection.vertical:
                yield ply * (1.0 + loffset), pry * (1.0 - roffset)
            else:
                yield plx * (1.0 + loffset), prx * (1.0 - roffset)

    return list(bounds_())


def calculate_areas(p, spline=None):
    spline = spline or interpolate(p)
    integrated = spline.antiderivative()
    areas = []
    for i, (pl, pm, pr) in enumerate(windowed(map(make_point, p), 3)):
        if pm.type != Intersection.vertical:
            continue
        right_lever = pr.x - pm.x
        left_lever = pm.x - pl.x
        left_area = integrated(pm.x) - integrated(pl.x)
        right_area = integrated(pr.x) - integrated(pm.x)
        if pl.type == Intersection.horizontal:
            left_area = (left_area - left_lever * pl.y)
        elif pl.type == Intersection.vertical:
            left_area = (left_lever * pm.y - left_area)
        elif pl.type == Intersection.bound:
            pass

        if pr.type == Intersection.horizontal:
            right_area = (right_lever * pr.y - right_area)
        elif pr.type == Intersection.vertical:
            right_area = (right_area - pm.y * right_lever)
        elif pr.type == Intersection.bound:
            right_area = (right_lever * pr.y - right_area)

        areas.append((i, left_area, right_area))
    return areas


def prepare_export_data(d, points, spline=None):
    spline = spline or interpolate(points)
    mgx, _ = calculate_median_grade(points, spline=spline)
    fs1, fs2 = calculate_float_sink_curve(points, spline=spline, median_grade=mgx)
    firstp, *pts, lastp = map(make_point, points)
    xaxis = np.linspace(firstp.x, lastp.x, num=200)
    yaxis = spline(xaxis)
    return make_step_function_data(d), xaxis.tolist(), yaxis.tolist(), fs1, fs2, mgx


def reduce_points(p):
    return list(pmy if pt == Intersection.vertical else pmx for _, (pmx, pmy, pt), _ in windowed(p, 3))


def expand_points(x, p):
    pp = p.copy()
    for index, (_, (pmx, pmy, pt), _), val in zip(range(1, len(p) - 1), windowed(p, 3), x):
        assert pt in {Intersection.vertical, Intersection.horizontal}
        pp[index] = (pmx if pt == Intersection.vertical else val, val if pt == Intersection.vertical else pmy, pt)
    return pp


def cost_function(p, spline=None, weights=None):
    return sum(abs(l - r) for _, l, r in calculate_areas(p, spline=spline))


def callback(*args, **kwargs):
    pass


def function(x, p):
    points = expand_points(x, p)
    return cost_function(points)


def calculate_float_sink_curve(points, spline=None, algo='L-BFGS-B', median_grade=0.0):
    firstp, *pts, lastp = map(make_point, points)
    needed_points = list(
        filter(
            lambda p: p.type in {Intersection.bound, Intersection.horizontal},
            pts
        )
    )
    median_grade_for_class = functools.partial(calculate_median_grade, points, spline=spline, algo=algo)
    float_sink_points = needed_points + [lastp]
    float_sink_curve_1 = [median_grade_for_class(firstp=firstp, lastp=p) for p in float_sink_points]
    float_sink_curve_2 = [median_grade_for_class(firstp=p, lastp=lastp) for p in float_sink_points]

    def rearrange(fs):
        return [(eqx, step_y) for (eqx, _), (_, step_y, _) in zip(fs, float_sink_points)]

    return [(firstp.x, firstp.y)] + rearrange(float_sink_curve_1) ,\
            [(firstp.x+median_grade, firstp.y)] + rearrange(float_sink_curve_2)


def calculate_median_grade(points, firstp=None, lastp=None, spline=None, algo='L-BFGS-B'):
    spline = spline or interpolate(points)
    firstp_, *_, lastp_ = map(make_point, points)
    firstp = firstp or firstp_
    lastp = lastp or lastp_

    x0 = lastp.x / 2.0

    def cost(x):
        x = first(x)
        y = spline(x)
        p = [
            firstp,
            (x, y, Intersection.vertical),
            lastp
        ]
        return cost_function(p, spline=spline)

    result = minimize_scipy(cost, np.array([x0]), bounds=[(0.0, lastp.x)], method=algo)
    return first(result.x), spline(first(result.x))


def minimize(input_values, ymass, grade, algo='L-BFGS-B'):
    points = all_points(input_values, make_bounds(ymass, grade))
    x0 = reduce_points(points)
    bounds = all_bounds(points)
    result = minimize_scipy(function, np.array(x0), args=(points,), bounds=bounds, callback=callback, method=algo)
    final_points = expand_points(result.x, points)
    return final_points


def calculate_residual_areas(points):
    grades, _, _ = transpose(points)
    windows = list(windowed(map(make_point, points), 3))

    results = []
    for index, la, ra in calculate_areas(points):
        _, pm, _ = windows[index]
        results.append((pm.x, la, ra))

    return results
