import io
import pickle
import functools
import dns.resolver
import dash_core_components as dcc
import dash_bootstrap_components as dbc

from henry_reinhardt.auth import log_access, get_connection, DEFAULT_DB_NAME

from henry_reinhardt.core import plot_henry_reinhardt, all_points, get_bounds, make_bounds, empty_figure, minimize, \
    prepare_export_data, calculate_residual_areas, transpose
from henry_reinhardt.data import validate_input_table, read_spreadsheet, dash_table_to_data_frame, \
    data_frame_to_dash_table, points_to_store, points_from_store, export_matplotlib, export_gnuplot, export_file
from henry_reinhardt.layout import build_main_card, make_heading, make_residual_badges
from dash_extensions.enrich import Output, DashProxy, Input, MultiplexerTransform, State


external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

FONT_AWESOME = "https://use.fontawesome.com/releases/v5.7.2/css/all.css"
# create an app


def build_dash_base_app():
    app = DashProxy(
        __name__,
        external_stylesheets=[dbc.themes.BOOTSTRAP, FONT_AWESOME],
        prevent_initial_callbacks=True,
        transforms=[MultiplexerTransform()],
        meta_tags=[
            {"name": "viewport", "content": "width=device-width, initial-scale=1"},
        ]
    )
    return app


columns = ('grade', 'yield')

states = {
    True: [True, False],
    False: [False, True],
    None: [False, False]
}


def send_file(f):

    def _inner(*args, **kwargs):
        result = f(*args, **kwargs)
        assert 'filename' in result
        assert 'content' in result
        return dcc.send_bytes(result['content'], result['filename'])

    return _inner


def export_format(n_clicks, data, points, filename, exporter):
    if n_clicks:
        df = dash_table_to_data_frame(data)
        d = df.values.tolist()
        final_points = points_from_store(points)
        return dict(
            filename=filename,
            content=exporter(*prepare_export_data(d, points=final_points))
        )


def make_badge(msg, color):
    return dbc.Badge(msg, color=color, href='#', className="mr-1")


def define_main_callbacks(dash_app):

    @dash_app.callback([Output('upload-success-badge-container', 'children'),
                   Output('table-input-points', 'data')],
                  [Input('datatable-upload', 'contents'),
                   State('datatable-upload', 'filename'),
                   State('datatable-upload', 'last_modified')])
    def update_output(content, name, dates):
        if name:
            try:
                df = read_spreadsheet(content, name, dates)
            except Exception:
                msg, color = f'Failed to read "{name}"', 'danger'
                data = []
            else:
                df = df.sort_values(by='grade')
                msg = validate_input_table(df)
                if msg is not None:
                    data = []
                    color = 'warning'
                else:
                    data = data_frame_to_dash_table(df)
                    color = 'success'
                    msg = 'All fine'

            badge = dbc.Badge(msg, color=color, href='#', className="mr-1")
            return badge, data
        return (), ()

    @dash_app.callback(
        Output('input-add-point-grade', 'valid'),
        Output('input-add-point-grade', 'invalid'),
        Output('input-add-point-yield', 'valid'),
        Output('input-add-point-yield', 'invalid'),
        Output('button-add-point', 'disabled'),
        Output('container-add-point-feedback', 'children'),
        Input('input-add-point-grade', 'value'),
        Input('input-add-point-yield', 'value'),
        State('table-input-points', 'data')
    )
    def can_add_point(grade, ymass, table_data):

        grade_valid = grade is not None
        ymass_valid = ymass is not None

        if ymass_valid and not grade_valid:
            badge = make_badge('Grade must be between 0 and 100%', 'primary')
            ymass_state = states.get(ymass_valid)
            grade_state = states.get(grade)
            button_enabled = False
        elif not ymass_valid and grade_valid:
            badge = make_badge('Yield mass be between 0 and 100%', 'primary')
            ymass_state = states.get(ymass)
            grade_state = states.get(grade_valid)
            button_enabled = False
        elif not ymass_valid and not grade_valid:
            badge = []
            grade_state = ymass_state = states.get(None)
            button_enabled = False
        elif ymass_valid and grade_valid:
            # implicit bool values may be also '' empty strings
            if not bool(grade) or not bool(ymass):
                grade_state = states.get(bool(grade))
                ymass_state = states.get(bool(ymass))
                button_enabled = False
                badge = []
            else:
                preview_data = table_data.copy()
                preview_data.append({
                    'col-grade': grade,
                    'col-yield': ymass
                })
                preview_data = list(sorted(preview_data, key=lambda d: d['col-yield']))
                df = dash_table_to_data_frame(preview_data)
                msg = validate_input_table(df)
                button_enabled = msg is None
                grade_state = ymass_state = states.get(True)
                badge = make_badge(msg or 'All fine', 'light' if msg is None else 'primary')
        else:
            raise RuntimeError
        return tuple(grade_state + ymass_state + [not button_enabled] + [badge])

    @dash_app.callback(
        Output('table-input-points', 'data'),
        Output('input-add-point-grade', 'value'),
        Output('input-add-point-yield', 'value'),
        Output('input-add-point-grade', 'valid'),
        Output('input-add-point-grade', 'invalid'),
        Output('input-add-point-yield', 'valid'),
        Output('input-add-point-yield', 'invalid'),
        Output('button-add-point', 'disabled'),
        Output('container-add-point-feedback', 'children'),
        [
            Input('button-add-point', 'n_clicks'),
            State('table-input-points', 'data'),
            State('input-add-point-grade', 'value'),
            State('input-add-point-yield', 'value')
        ]
    )
    def add_point(n_clicks, table_data, grade, ymass):
        if n_clicks > 0:
            table_data.append({
                'col-grade': grade,
                'col-yield': ymass
            })
            table_data = list(sorted(table_data, key=lambda d: d['col-yield']))
        return table_data, '', '', False, False, False, False, True, []

    @dash_app.callback(
        Output('addon-ymass-first-prepend', 'children'),
        Output('addon-ymass-first-append', 'children'),
        Output('addon-grade-last-prepend', 'children'),
        Output('addon-grade-last-append', 'children'),
        Output('input-ymass-first', 'min'),
        Output('input-ymass-first', 'max'),
        Output('input-ymass-first', 'disabled'),
        Output('input-grade-last', 'min'),
        Output('input-grade-last', 'max'),
        Output('input-grade-last', 'disabled'),
        Output('button-show-export-methods', 'disabled'),
        Input('table-input-points', 'data')
    )
    def update_input_points(data):
        default_values = (), (), (), (), None, None, True, None, None, True, True
        if data is None:
            return default_values
        df = dash_table_to_data_frame(data)
        if validate_input_table(df):
            return default_values
        d = df.values.tolist()
        (ll, lu), (rl, ru) = get_bounds(d)
        return f'{ll:.1f} ≤', f'≤ {lu:.1f} %', f'{rl:.1f} ≤ ', f'≤ {ru:.1f} %', ll, lu, False, rl, ru, False, True

    @dash_app.callback(
        Output('output-residual-areas', 'children'),
        Output('figure-henry', 'figure'),
        Output('input-ymass-first', 'valid'),
        Output('input-ymass-first', 'invalid'),
        Output('input-grade-last', 'valid'),
        Output('input-grade-last', 'invalid'),
        Output('button-plot-preview', 'disabled'),
        Output('button-minimize', 'disabled'),
        Output('button-show-export-methods', 'disabled'),
        Input('input-ymass-first', 'value'),
        Input('input-grade-last', 'value'),
        State('table-input-points', 'data')
    )
    def update_points(ymass, grade, data):
        grade_valid = grade is not None
        ymass_valid = ymass is not None

        button_enabled = grade_valid and ymass_valid
        if button_enabled:
            df = dash_table_to_data_frame(data)
            input_values = df.values.tolist()
            points = all_points(input_values, make_bounds(ymass, grade))
            residual_areas = calculate_residual_areas(points)
            figure = plot_henry_reinhardt(input_values, points=points)
            ra_childs = show_residuals(residual_areas)
        else:
            figure = empty_figure()
            ra_childs = []
        return tuple([ra_childs, figure] + states.get(ymass_valid) + states.get(grade_valid) +
                     [not button_enabled, not button_enabled, True])

    def show_residuals(residual_areas):
        return [make_heading('Residuals', 'fas fa-ruler'), make_residual_badges(residual_areas)]

    @dash_app.callback(
        Output('figure-henry', 'figure'),
        Output('output-residual-areas', 'children'),
        Output('button-show-export-methods', 'disabled'),
        Input('button-plot-preview', 'n_clicks'),
        State('input-ymass-first', 'value'),
        State('input-grade-last', 'value'),
        State('table-input-points', 'data')
    )
    def create_preview(n_clicks, ymass, grade, data):
        if n_clicks:
            df = dash_table_to_data_frame(data)
            input_values = df.values.tolist()
            points = all_points(input_values, make_bounds(ymass, grade))
            residual_areas = calculate_residual_areas(points)

            return \
                plot_henry_reinhardt(input_values, points=points), \
                show_residuals(residual_areas), \
                True
        return empty_figure(), [], True

    @dash_app.callback(
        Output('minimize-loading-output', 'children'),
        Output('figure-henry', 'figure'),
        Output('output-residual-areas', 'children'),
        Output('button-show-export-methods', 'disabled'),
        Output('memory-minimization', 'data'),
        Input('button-minimize', 'n_clicks'),
        State('input-ymass-first', 'value'),
        State('input-grade-last', 'value'),
        State('select-algorithm', 'value'),
        State('table-input-points', 'data')
    )
    def minimize_(n_clicks, ymass, grade, algo, data):
        if n_clicks:
            df = dash_table_to_data_frame(data)
            input_values = df.values.tolist()
            final_points = minimize(input_values, ymass, grade, algo=algo)
            residual_areas = calculate_residual_areas(final_points)
            return '', plot_henry_reinhardt(input_values, points=final_points), show_residuals(residual_areas), False, \
                   points_to_store(final_points)
        return '', empty_figure(), [], True, {}

    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('button-export-python-script', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_script(n_clicks, data, points):
        return export_format(n_clicks, data, points, 'henry-reinhardt-chart.py', export_matplotlib)

    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('button-export-gnuplot-script', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_gnuplot(n_clicks, data, points):
        return export_format(n_clicks, data, points, 'henry-reinhardt-chart.gnuplot', export_gnuplot)

    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('button-export-spline', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_gnuplot(n_clicks, data, points):
        final_points = points_from_store(points)
        grades, yields, *_ = transpose(final_points)

        def export_spline(*_):
            with io.StringIO() as buf:
                buf.write('from scipy.interpolate import PchipInterpolator\n')
                buf.write(f'grades = {grades}\n')
                buf.write(f'yields = {yields}\n')
                buf.write('spline = PchipInterpolator(grades, yields)\n')
                buf.write('coefficients = spline.c')
                return buf.getvalue()

        return export_format(n_clicks, data, points, 'intergrowth.py', export_spline)

    @dash_app.callback(
        Input('button-spreadsheet-info-toast', 'n_clicks'),
        Output('spreadsheet-info-toast', 'is_open')
    )
    def show_spreadsheet_info_toast(n_click):
        return bool(n_click)


def define_export_callbacks(dash_app):

    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('item-export-image-svg', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_svg(n_clicks, data, points):
        export_svg = functools.partial(export_file, export_format='svg')
        return export_format(n_clicks, data, points, 'henry-reinhardt-chart.svg', export_svg)

    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('item-export-image-pdf', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_pdf(n_clicks, data, points):
        export_pdf = functools.partial(export_file, export_format='pdf')
        return send_file(export_format)(n_clicks, data, points, 'henry-reinhardt-chart.pdf', export_pdf)


    @dash_app.callback(
        Output('download-chart-export', 'data'),
        Input('item-export-image-png', 'n_clicks'),
        State('table-input-points', 'data'),
        State('memory-minimization', 'data')
    )
    def download_png(n_clicks, data, points):
        export_png = functools.partial(export_file, export_format='png')
        return send_file(export_format)(n_clicks, data, points, 'henry-reinhardt-chart.png', export_png)


def define_modal_callbacks(dash_app: DashProxy):
    @dash_app.callback(
        Output('input-email', 'valid'),
        Output('input-email', 'invalid'),
        Input('input-email', 'value')
    )
    def validate_email(email):
        from pyisemail import is_email
        if email:
            is_valid = is_email(email)
        else:
            is_valid = False
        return states.get(is_valid)

    @dash_app.callback(
        Output('button-check-form', 'disabled'),
        Input('input-name', 'value'),
        Input('input-institution', 'value'),
        Input('input-email', 'valid')
    )
    def check_form_input(name, institution, email_valid):
        enable_button = bool(name) and bool(institution) and email_valid
        return not enable_button

    @dash_app.callback(
        Output('input-email', 'valid'),
        Output('input-email', 'invalid'),
        Output('modal-terms-of-use', 'is_open'),
        Input('button-check-form', 'n_clicks'),
        State('input-name', 'value'),
        State('input-institution', 'value'),
        State('input-email', 'value')
    )
    def check_form(n_clicks, name, institution, email):
        from pyisemail import is_email
        if n_clicks:
            try:
                is_really_email = is_email(email, check_dns=True)
            except dns.resolver.NoNameservers:
                is_really_email = False

            if is_really_email:
                # now everything is fine and the modal dialog is going to close
                log_access(name, institution, email)
            return states.get(is_really_email) + [not is_really_email]


def build_app(modal=False):
    app = build_dash_base_app()
    define_main_callbacks(app)
    define_export_callbacks(app)
    app.title = 'Henry-Reinhardt Chart Creator'
    app.layout = build_main_card(modal)

    if modal:
        define_modal_callbacks(app)
    return app


app = build_app(modal=True)

server = app.server

if __name__ == '__main__':
    build_app(modal=False).run_server(debug=True)
