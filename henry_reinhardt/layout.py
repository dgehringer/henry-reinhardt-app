
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
    z_index = 1200
    return \
    [
        make_heading('Upload data', 'fas fa-file-upload'),
        dbc.Toast(
            [
                html.H6('Upload spreadsheet file'),
                html.Center(
                    html.Img(src='assets/spreadsheet_example.png', width='75%')
                ),
                dcc.Markdown(
                    '''
                    Please adapt the file such that it looks like the image above

                     - No column headers
                     - First column is grade
                     - Second column is yield mass
                     
                    Supported formats are:
                    ''',
                    style=dict(marginTop='15px')
                ),

                html.Center(
                    dbc.ButtonGroup(
                        [
                            dbc.Button([html.I(className='fas fa-file-excel'), html.Br(), 'Excel'], color='link'),
                            dbc.Button([html.I(className='fas fa-table'), html.Br(), 'OpenOffice'], color='link'),
                            dbc.Button([html.I(className='fas fa-file-csv'), html.Br(), 'CSV'], color='link')
                        ]
                    )
                )
            ],
            header='Upload spreadsheet info',
            icon='primary',
            id='spreadsheet-info-toast',
            is_open=False,
            style=dict(position='fixed', top=75, right=75, width=350, zIndex=z_index),
            dismissable=True
        ),
        dbc.Button(html.I(className='fas fa-question-circle'),
                   style=dict(
                       float='right',
                       borderRadius='20px',
                   ),
                   color='primary',
                   size='sm',
                   n_clicks=0,
                   id='button-spreadsheet-info-toast'
        ),
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


def build_modal_dialog():
    return dbc.Modal(
        [
            dbc.ModalHeader(html.Center('Terms of use')),
            dbc.ModalBody(
                [
                    html.P('Just a few things before we start ...'),
                    dbc.FormGroup(
                        [
                            dbc.Label([html.I(className='fas fa-user'), ' Name'], html_for='input-name'),
                            dbc.Input(id='input-name', type='text', placeholder='Your name ...', value='')
                        ]
                    ),
                    dbc.FormGroup(
                        [
                            dbc.Label([html.I(className='fas fa-envelope'), ' E-Mail'], html_for='input-email'),
                            dbc.Input(id='input-email', type='email', placeholder='Your E-Mail address ...'),
                            dbc.FormText('I promise you won\'t get any spam. This is just for analytics', color='secondary'),
                            dbc.FormFeedback('Sorry but this email does not look valid', valid=False)
                        ]
                    ),
                    dbc.FormGroup(
                        [
                            dbc.Label([html.I(className='fas fa-university'), ' Institution/ ', html.I(className='fas fa-industry'), ' Company'], html_for='input-institution'),
                            dbc.Input(id='input-institution', type='text', placeholder='Your institution/company ...', value='')
                        ],
                    ),
                    dcc.Markdown("""
                    ... and finally:
                    
                    - The project is distributed under [GPLv3](https://github.com/dgehringer/henry-reinhardt-app/blob/main/LICENSE) license
                    - Feature requests and bug reports go [here](https://github.com/dgehringer/henry-reinhardt-app/issues)
                    - &#128077;: leave a &#127775; at the [repo](https://github.com/dgehringer/henry-reinhardt-app) go tell your friends
                    - &#128078;: do not tell anyone about it
                    """)
                ]
            ),
            dbc.ModalFooter(
                [
                    dbc.Button('That\'s OK', color='primary', id='button-check-form', disabled=True),
                    html.Div(style=dict(width='37%')),
                    html.Center(make_social_links())
                ]
            )
        ],
        keyboard=False,
        centered=True,
        is_open=True,
        backdrop='static',
        id='modal-terms-of-use'
    )


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
                        dbc.FormText('Choose an algorithms for minimizing the residual areas', color='secondary')
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
                                dbc.FormText('Grade in % of the last point', color='secondary')
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
                                dbc.FormText('Yield mass in % of the first point', color='secondary')
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
                                        dbc.Button(html.I(className='fas fa-chart-line'), id='button-export-gnuplot-script', color='primary'),
                                        dbc.Button(html.I(className='fas fa-image'), id='button-export-image', color='primary'),
                                        dbc.Tooltip('Export a Python script', placement='bottom', target='button-export-python-script'),
                                        dbc.Tooltip('Export a gnuplot script', placement='bottom', target='button-export-gnuplot-script'),
                                        dbc.Tooltip('Export a figure', placement='right', target='button-export-image'),
                                        dbc.Popover(
                                            [
                                                dbc.PopoverBody(
                                                    [
                                                        dbc.ListGroup(
                                                            [
                                                                dbc.ListGroupItem(html.Span([html.I(className='fas fa-file-pdf'), ' PDF']), id='item-export-image-pdf', n_clicks=0, action=True),
                                                                dbc.ListGroupItem(html.Span([html.I(className='fas fa-bezier-curve'), ' SVG']), id='item-export-image-svg', n_clicks=0, action=True),
                                                                dbc.ListGroupItem(html.Span([html.I(className='fas fa-file-image'), ' PNG']), id='item-export-image-png', n_clicks=0, action=True),
                                                            ]
                                                        )
                                                    ]
                                                )
                                            ],
                                            id='popover-export-file-format',
                                            target='button-export-image',
                                            placement='top',
                                            trigger='hover'
                                        )
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


def make_social_links():
    return html.Span(
        [
            dbc.Button(html.I(className='fas fa-globe-europe'), color='link', href='http://dominik.gehringers.at'),
            dbc.Button(html.I(className='fab fa-github-square'), color='link', href='https://github.com/dgehringer'),
            dbc.Button(html.I(className='fab fab fa-linkedin'), color='link',
                       href='http://linkedin.com/in/dominik-gehringer-b90230215'),
            dbc.Button(html.I(className='fas fa-envelope-square'), color='link', href='mailto:dominik.gehringers.at')
        ]
    )


def build_main_card(modal_dialog=True):
    card = dbc.Card(
        [
            dbc.CardHeader(
                html.H3('Henry-Reinhardt Chart Creator')
            ),
            dbc.CardBody(html.P(build_card_body(), id="card-content", className="card-text")),
            dbc.CardFooter(
                html.Center(
                [
                    '© 2021 ',
                    html.A('Dominik Gehringer', href='http://dominik.gehringers.at'),
                    make_social_links()
                ]
            ))
        ],
        style=dict(margin='auto', width='95%')
    )
    children = [card]
    if modal_dialog:
        children.append(build_modal_dialog())
    return html.Div(
        children,
        style=dict(marginTop='50px')
    )
