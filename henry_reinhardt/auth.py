

import atexit
import collections.abc
import functools
import sqlite3

DEFAULT_DB_NAME = 'auth.sqlite'
DEFAULT_TABLE_NAME = 'access'
__conn = None


def ensure_iterable(o, exclude=(bytes, str, bytearray), factory=tuple):
    is_iterable = isinstance(o, collections.abc.Iterable) and not isinstance(o, exclude)
    return o if is_iterable else factory([o])


def _create_table(table_name=DEFAULT_TABLE_NAME):
    return f'''
CREATE TABLE IF NOT EXISTS {table_name} (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    institution TEXT NOT NULL,
    email TEXT NOT NULL,
    time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
'''


def _save_access_record(name, institution, email, table_name=DEFAULT_TABLE_NAME):
    sql = f'INSERT INTO {table_name}(name, institution, email) VALUES (?, ?, ?)'
    return sql, (name, institution, email)


def get_connection(db=DEFAULT_DB_NAME):
    global __conn
    if __conn is None:
        __conn = sqlite3.connect(db, check_same_thread=False)
        atexit.register(__conn.close)
    return __conn


def with_connection(f, connection=None, **conn_kwargs):
    connection = connection or get_connection(**conn_kwargs)

    @functools.wraps(f)
    def _inner(*args, **kwargs):
        execution_args = f(*args, **kwargs)
        with connection:
            return connection.execute(*ensure_iterable(execution_args))

    return _inner


def log_access(name, institution, email, db=DEFAULT_DB_NAME, table_name=DEFAULT_TABLE_NAME):
    execute = functools.partial(with_connection, db=db)
    execute(_create_table)(table_name=table_name)
    execute(_save_access_record)(name, institution, email, table_name=table_name)
