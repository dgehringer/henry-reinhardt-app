
import dash_table
import dash_core_components as dcc
import dash_html_components as html
import dash_bootstrap_components as dbc
from henry_reinhardt.core import algorithms, empty_figure


def build_add_point_form():
    form = [
        dbc.PopoverHeader([
            html.B('add point'),
            dbc.Button(
                html.Span(
                    [html.I(className='fas fa-plus-square'), ' add']
                ),
                id='button-add-point',
                color='primary',
                size='sm',
                disabled=True,
                n_clicks=0,
                style=dict(float='right', marginRight='0px')
            )
        ], style=dict(paddingBottom='20px')),
        dbc.PopoverBody(
            [
                dbc.InputGroup(
                    [
                        dbc.InputGroupAddon('Grade', addon_type='prepend'),
                        dbc.Input(id='input-add-point-grade', min=0.0, max=100.0, type='number'),
                        dbc.InputGroupAddon('%', addon_type='append')
                    ],
                    size='sm'
                ),
                dbc.InputGroup(
                    [
                        dbc.InputGroupAddon('Yield', addon_type='prepend'),
                        dbc.Input(id='input-add-point-yield', min=0.0, max=100.0, type='number'),
                        dbc.InputGroupAddon('%', addon_type='append')
                    ],
                    size='sm',
                    style=dict(marginTop='5px')
                ),
                html.Div(id='container-add-point-feedback')
            ]
        )
    ]

    return html.Div([
        dbc.Button(
            html.I(className='fas fa-plus-square'),
            color='primary',
            size='sm',
            id='button-add-point-hover'
        ),
        dbc.Popover(
            form,
            id='popover-add-point',
            target='button-add-point-hover',
            placement='bottom',
            trigger='hover',
        )
    ], style=dict(textAlign='right', marginTop='10px'))


def build_upload_section():
    return \
    [
        make_heading('Upload data', 'fas fa-file-upload'),
        dcc.Upload(
            id='datatable-upload',
            children=html.Div([
                'Drop or ',
                html.A("Select")
            ]),
            style={
                'width': '80%',
                'height': '60px',
                'lineHeight': '60px',
                'borderWidth': '1px',
                'borderStyle': 'dashed',
                'borderRadius': '5px',
                'textAlign': 'center',
                'margin': '0px 10% 0px'
            },
            multiple=False
        )
    ]


def build_data_table():
    return \
        [
            make_heading('Points', 'fas fa-map-marker-alt'),
            dash_table.DataTable(
                id='table-input-points',
                columns=[
                    dict(id='col-grade', name='Grade [%]', type='numeric', clearable=False),
                    dict(id='col-yield', name='Yield mass [%]', type='numeric', clearable=False)
                ],
                data=[],
                editable=True,
                row_deletable=True,
            ),
            build_add_point_form(),
            html.Span(id='upload-success-badge-container')
        ]


def make_heading(name, icon=None):
    name = name if icon is None else f' {name}'
    childs = [name]
    if icon is not None:
        childs.insert(0, html.I(className=icon))

    return html.H5(childs, style=dict(marginTop='20px', marginBottom='20px'))


def build_settings_section():
    return [
        make_heading('Settings', 'fas fa-wrench'),
        dbc.FormGroup(
            [
                dbc.Label('Algorithm', html_for='select-algorithm', width=12),
                dbc.Col(
                    [
                        dbc.Select(
                            id='select-algorithm',
                            value=next(iter(algorithms)),
                            options=[dict(label=algo, value=algo) for algo in algorithms]
                        ),
                        dbc.FormText('Some text goes here to select the algorithm')
                    ],
                    width=9
                )
            ]
        ),
        dbc.Row(
            [
                dbc.Col(
                    [
                        dbc.FormGroup(
                            [
                                dbc.Label('Grade [%]', html_for='input-grade-last'),
                                dbc.InputGroup(
                                    [
                                        dbc.InputGroupAddon(id='addon-grade-last-prepend',addon_type='prepend'),
                                        dbc.Input(id='input-grade-last', type='number', disabled=True),
                                        dbc.InputGroupAddon(id='addon-grade-last-append', addon_type='append')
                                    ]
                                ),
                                dbc.FormText('Grade in % of the last point')
                            ]
                        )
                    ],
                    width=6
                ),
                dbc.Col(
                    [
                        dbc.FormGroup(
                            [
                                dbc.Label('Yield mass [%]', html_for='input-ymass-first'),
                                dbc.InputGroup(
                                    [
                                        dbc.InputGroupAddon(id='addon-ymass-first-prepend', addon_type='prepend'),
                                        dbc.Input(id='input-ymass-first', type='number', disabled=True),
                                        dbc.InputGroupAddon(id='addon-ymass-first-append', addon_type='append')
                                    ]
                                ),
                                dbc.FormText('Yield mass in % of the first point')
                            ],
                        )
                    ],
                    width=6
                ),
            ],
            form=True
        ),
        dbc.Row(
            [
                dbc.Button(html.Span([html.I(className='fas fa-eye'), ' Preview']), color='primary', id='button-plot-preview', className='mr-1', disabled=True),
                dbc.Button(html.Span([html.I(className='fas fa-cogs'), ' Minimize']), color='primary', id='button-minimize', className='mr-1', disabled=True),
                dbc.Button(html.I(className='fas fa-file-download'), color='primary',  disabled=True, id='button-show-export-methods'),
                dbc.Popover(
                    [
                        dbc.PopoverHeader('Export chart'),
                        dbc.PopoverHeader(
                            [
                                dbc.ButtonGroup(
                                    [
                                        dbc.Button(html.I(className='fab fa-python'), id='button-export-python-script', color='primary'),
                                        dbc.Button(html.I(className='fas fa-chart-line'), id='button-export-gnuplot-script', color='primary')
                                    ]
                                )
                            ]
                        )
                    ],
                    id='popover-export-methods',
                    target='button-show-export-methods',
                    placement='right',
                    trigger='hover',
                ),
                dcc.Download(id='download-chart-export'),
                dcc.Store(id='memory-minimization'),
                dbc.Spinner(html.Div(id='minimize-loading-output'), fullscreen=True)
            ]
        )
    ]


def make_residual_badges(residual_areas):

    def make_residual_badge(grade, left, right):
        deviation = abs(left-right)
        average = (left + right)/2.0
        deviation_percentage = deviation/average*100
        if deviation_percentage <= 1.0:
            color = 'success'
        elif deviation <= 5.0:
            color = 'warning'
        else:
            color = 'danger'
        return dbc.Button(
            [
                f'{grade:.1f} %',
                dbc.Badge(f'{deviation:.2e}', color='light', className='ml-1', )
            ],
            color=color,
            active=True,
            className='mr-1',
            size='sm'
        )

    return \
        html.Center(
            html.Span(
                [make_residual_badge(*ra) for ra in residual_areas]
            )
        )


def build_card_body():
    return dbc.CardBody([
        dbc.Row([
            dbc.Col(
                build_upload_section() + \
                build_data_table() + \
                build_settings_section()
            , width=4),
            dbc.Col([
                make_heading('Chart', 'fas fa-chart-line'),
                dcc.Graph(id='figure-henry', figure=empty_figure(), config=dict(
                    doubleClick='reset+autosize'
                )),
                html.Div(id='output-residual-areas')
            ], width=8)
        ]),
    ])


def build_main_card():
    card = dbc.Card(
        [
            dbc.CardHeader(
                html.H3('Henry-Reinhardt chart creator')
            ),
            dbc.CardBody(html.P(build_card_body(), id="card-content", className="card-text")),
            dbc.CardFooter(
                html.Center(
                [
                    '© 2021 ',
                    html.A('Dominik Gehringer', href='http://dominik.gehringers.at'),
                    html.Span(
                        [
                            dbc.Button(html.I(className='fab fa-github-square'), color='link', href='https://github.com/dgehringer'),
                            dbc.Button(html.I(className='fab fab fa-linkedin'), color='link', href='http://linkedin.com/in/dominik-gehringer-b90230215'),
                            dbc.Button(html.I(className='fas fa-envelope-square'), color='link', href='mailto:dominik.gehringers.at')
                        ]
                    )
                ]
            ))
        ],
        style=dict(margin='auto', width='95%')
    )
    return html.Div(card, style=dict(marginTop='50px'))
