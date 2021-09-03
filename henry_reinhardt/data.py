import io
import pprint
import re
import base64
import numpy as np
import pandas as pd
import operator

item = operator.itemgetter

columns = ('grade', 'yield')


def data_frame_to_dash_table(df : pd.DataFrame, prefix='col-'):
    return [{f'{prefix}{c}': record[c] for c in df.columns}  for _, record in df.iterrows()]


def first(it):
    for e in it:
        yield e
        return


def dash_table_to_data_frame(data: list, prefix='col-'):
    matcher = re.compile(rf'{prefix}([\w_-]+)')

    def clean_col_name(name):
        return next(iter(matcher.findall(name)))

    return pd.DataFrame(data=[{clean_col_name(k): v for k, v in record.items()} for record in data])


def read_spreadsheet(contents, filename, date):
    content_type, content_string = contents.split(',')
    decoded = base64.b64decode(content_string)
    if '.csv' in filename:
        # Assume that the user uploaded a CSV file
        df = pd.read_csv(
            io.StringIO(decoded.decode('utf-8')))
    elif '.xls' in filename:
        # Assume that the user uploaded an excel file
        df = pd.read_excel(io.BytesIO(decoded), header=None, names=['grade', 'yield'])
    elif '.ods' in filename:
        df = pd.read_excel(io.BytesIO(decoded), header=None, names=['grade', 'yield'], engine='odf')
    else:
        raise IOError
    return df


def validate_input_table(df: pd.DataFrame, tol=0.1):
    for c in columns:
        if c not in df.columns:
            return f'missing column "{c}"'
    if df.isnull().sum().sum():
        return 'there are NaN values in your table'
    for i, grade, ymass in df.itertuples():
        if not (0 <= grade <= 100):
            return f'grade in row {i} must be less than 100%. ({grade})'
        if not (0 <= ymass <= 100):
            return f'mass in row {i} must be less than 100%. ({ymass})'
    df = df.sort_values(by='grade')
    if not df.grade.is_monotonic:
        return f'the grades are not a monotonic series'
    if not df['yield'].is_monotonic:
        return f'the yields are not a monotonic series'

    for i, *p1 in df.itertuples():
        for j, *p2 in df.itertuples():
            if i == j: continue
            d = np.linalg.norm(np.array(p1)-np.array(p2))
            if d < tol:
                return f'Point {tuple(p1)} and {tuple(p2)} are too close'
    return None


def points_to_store(points):
    return {i: (x, y, t.value) for i, (x, y, t) in enumerate(points)}


def points_from_store(data):
    from henry_reinhardt.core import Intersection
    return [ (x, y, Intersection(tstr)) for _, (x, y, tstr) in sorted(data.items(), key=item(0))]


def export_matplotlib(d, points, spline=None, median_grade=True, float_sink=True):
    from henry_reinhardt.core import make_step_function_data, make_point, interpolate, calculate_median_grade, calculate_float_sink_curve, transpose
    spline = spline or interpolate(points)
    mgx, mgy = calculate_median_grade(points, spline=spline)
    fs1, fs2 = calculate_float_sink_curve(points, spline=spline, median_grade=mgx)
    firstp, *pts, lastp = map(make_point, points)
    xaxis = np.linspace(firstp.x, lastp.x)
    yaxis = spline(xaxis)
    return f"""
import numpy as np
import matplotlib.pyplot as plt

colors = dict(
    blue='#1f77b4', 
    orange='#ff7f0e', 
    green='#2ca02c', 
    red='#d62728',
    purple='#9467bd',
    brown='#8c564b',
    pink='#e377c2', 
    gray='#7f7f7f',
    yellow='#bcbd22',
    cyan='#17becf'
)

figsize=(7,5)
plt.figure(figsize=(figsize))

step_function = {make_step_function_data(d)}
spline = ({xaxis.tolist()}, {yaxis.tolist()})
float_sink_1 = {tuple(transpose(fs1))}
float_sink_2 = {tuple(transpose(fs2))}

plt.title('Henry-Reinhardt Schaubild')
plt.plot(*step_function, label='Treppenfunktion', color=colors.get('blue'), alpha=0.5)
plt.plot(*spline, label='Grundverwachsungskurve', color=colors.get('orange'))
plt.plot(*float_sink_1, label='Schwimm/Sinkkurve', color=colors.get('green'), marker='x', linestyle='solid')
plt.plot(*float_sink_2, label='Schwimm/Sinkkurve', color=colors.get('green'), marker='x', linestyle='solid')
plt.axvline({mgx}, color=colors.get('gray'), linestyle='dashed', alpha=0.75)
plt.xlabel('Gehalt [%]')
plt.ylabel('Masseausbringen [%]')
plt.ylim(100, 0)
plt.xlim(0, 100)
plt.legend(loc='upper right')
plt.savefig('henry-reinhardt.pdf')

"""