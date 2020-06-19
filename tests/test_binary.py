import mysql.connector as mariadb
import datetime
from os import getenv

TABLE = getenv('DB2I_TABLE') or 'test'
DB = getenv('DB2I_DB') or 'db2itest'
HOST = getenv('DB2I_HOST') or '127.0.0.1'
USER = getenv('DB2I_USER') or 'root'
PASS = getenv('DB2I_PASS') or ''

def create_insert_select(fields, values):
    conn = mariadb.connect(user=USER, password=PASS, host=HOST, database=DB)
    cursor = conn.cursor()
    cursor.execute(f"DROP TABLE IF EXISTS {TABLE}")
    conn.commit()
    cursor.execute(f"CREATE TABLE {TABLE} ({fields}) engine=ibmdb2i")
    cursor.execute(f"INSERT INTO {TABLE} VALUES({values})")
    conn.commit()
    cursor.execute(f"SELECT HEX(x), HEX(y), HEX (z) FROM {TABLE}")
    data = cursor.fetchone()
    print(f'Data: {data}')
    cursor.execute(f"DROP TABLE {TABLE}")
    conn.commit()
    cursor.close()
    conn.close()
    return data

def test_tinyblob():
    data = create_insert_select('x tinyblob, y tinyblob, z tinyblob', "x'DEADBEEF', 'ABC', 'CBA'")
    assert data[0] == 'DEADBEEF'
    assert data[1] == '414243'
    assert data[2] == '434241'

def test_mediumblob():
    data = create_insert_select('x mediumblob, y mediumblob, z mediumblob',
     "x'DEADBEEF', x'BADC0FFEE0DDF00D', 'ABC'")
    assert data[0] == 'DEADBEEF'
    assert data[1] == 'BADC0FFEE0DDF00D'
    assert data[2] == 414243 # returned as number rather than string

def test_longblob():
    data = create_insert_select('x longblob, y longblob, z longblob',
     "'ABC', x'BADC0FFEE0DDF00D', x'DEADBEEF'")
    assert data[0] == 414243 # returned as number rather than string
    assert data[1] == 'BADC0FFEE0DDF00D'
    assert data[2] == 'DEADBEEF'

def test_blob():
    data = create_insert_select('x blob, y blob, z blob',
     "'ABC', b'1111111111111', x'DEADBEEF'")
    assert data[0] == 414243 # returned as number rather than string
    assert data[1] == '1FFF'
    assert data[2] == 'DEADBEEF'

def test_binary():
    data = create_insert_select('x binary(10), y binary(10), z binary(10)',
     "'1234567890', x'DEADBEEF', x'BADC0FFEE0DDF00D'")
    assert data[0] == '31323334353637383930'
    assert data[1] == 'DEADBEEF000000000000'
    assert data[2] == 'BADC0FFEE0DDF00D0000'

def test_varbinary():
    data = create_insert_select('x varbinary(10), y varbinary(10), z varbinary(10)',
    "'1234567890', x'DEADBEEF', x'BADC0FFEE0DDF00D'")
    assert data[0] == '31323334353637383930'
    assert data[1] == 'DEADBEEF'
    assert data[2] == 'BADC0FFEE0DDF00D'
