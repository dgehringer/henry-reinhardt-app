
import io
import re
import base64
import operator
import functools
import numpy as np
import pandas as pd


item = operator.itemgetter

columns = ('grade', 'yield')


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


def data_frame_to_dash_table(df: pd.DataFrame, prefix='col-'):
    return [{f'{prefix}{c}': record[c] for c in df.columns} for _, record in df.iterrows()]


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
        if grade <= 1e-5:
            return f'"grade" can not be strictly 0. Please use e.g 0.001'
    df = df.sort_values(by='grade')
    if not df.grade.is_monotonic:
        return f'the grades are not a monotonic series'
    if not df['yield'].is_monotonic:
        return f'the yields are not a monotonic series'

    for i, *p1 in df.itertuples():
        for j, *p2 in df.itertuples():
            if i == j:
                continue
            d = np.linalg.norm(np.array(p1)-np.array(p2))
            if d < tol:
                return f'Point {tuple(p1)} and {tuple(p2)} are too close'
    return None


def points_to_store(points):
    return {i: (x, y, t.value) for i, (x, y, t) in enumerate(points)}


def points_from_store(data):
    from henry_reinhardt.core import Intersection
    points = [(x, y, Intersection(tstr)) for _, (x, y, tstr) in sorted(data.items(), key=item(0))]
    return list(sorted(points, key=item(0)))


def export_gnuplot(step, xaxis, yaxis, fs1, fs2, mgx):
    from henry_reinhardt.core import transpose

    def color(c, opacity=1.0):
        opacity = hex(int(opacity * 255))[-2:]
        return f'#{opacity}{colors.get(c)[-6:]}'.upper()

    buf = io.StringIO()
    write = functools.partial(print, file=buf)

    def write_points(x, y):
        for x_, y_ in zip(x, y):
            write(f'{x_:.5f} {y_:.5f}')
        write('e')

    write('set title "Henry-Reinhardt Schaubild"')
    write('set xlabel "Gehalt [%]"')
    write('set ylabel "Masseausbringen [%]"')
    write('set xrange [0:100]')
    write('set yrange [100:0]')
    write(f'set arrow from {mgx},0 to {mgx},100 nohead lc rgb "{color("gray", 0.75)}" dt 2')
    write(f'plot '
          f'"-" with lines lc rgb "{color("blue", 0.5)}"  title "Treppenfunktion", '
          f'"-" with lines lc rgb "{colors.get("orange")}" title "Grundverwachsungskurve", '
          f'"-" with linespoints lc rgb "{colors.get("green")}" pt 2 title "Schwimm/Sinkkurve", '
          f'"-" with linespoints lc rgb "{colors.get("green")}" pt 2 title "Schwimm/Sinkkurve"')
    write_points(*step)
    write_points(xaxis, yaxis)
    write_points(*transpose(fs1))
    write_points(*transpose(fs2))

    script = buf.getvalue()
    buf.close()
    return script


def export_file(step, xaxis, yaxis, fs1, fs2, mgx, export_format='pdf'):
    BufferType = io.StringIO if export_format == 'svg' else io.BytesIO
    with BufferType() as buf:
        buffer_var_name = 'buffer'
        script = export_matplotlib(step, xaxis, yaxis, fs1, fs2, mgx, export_format=export_format, export_var_name=buffer_var_name)
        exec(script, {buffer_var_name: buf})
        return buf.getvalue()


def export_matplotlib(step, xaxis, yaxis, fs1, fs2, mgx, export_prefix='henry-reinhardt', export_format='pdf', export_var_name=None):
    from henry_reinhardt.core import transpose
    savefig_args = f'\'{export_prefix}.{export_format}\'' if export_var_name is None else export_var_name
    return f"""
import numpy as np
import matplotlib.pyplot as plt

colors = {colors}

figsize=(7,5)
plt.figure(figsize=(figsize))

step_function = {step}
spline = ({xaxis}, {yaxis})
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
plt.savefig({savefig_args}, format='{export_format}')

"""
