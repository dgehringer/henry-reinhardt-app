import io
import re
import base64
import numpy as np
import pandas as pd

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
