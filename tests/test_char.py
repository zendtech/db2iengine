import mysql.connector as mariadb
from os import getenv

TABLE = getenv('DB2I_TABLE') or 'test'
DB = getenv('DB2I_DB') or 'db2itest'
HOST = getenv('DB2I_HOST') or '127.0.0.1'
USER = getenv('DB2I_USER') or 'root'
PASS = getenv('DB2I_PASS') or ''
EXPECTED = getenv('DB2I_EXPECTED') or 'Hello World'

def create_insert_select(charset, collation="general_ci"):
    conn = mariadb.connect(user=USER, password=PASS, host=HOST, database=DB)
    cursor = conn.cursor()
    cursor.execute(f"DROP TABLE IF EXISTS {TABLE}")
    conn.commit()
    cursor.execute(f"CREATE TABLE {TABLE} (i char(15)) engine=ibmdb2i CHARACTER SET '{charset}' COLLATE '{charset}_{collation}'")
    cursor.execute(f"INSERT INTO {TABLE} VALUES('{EXPECTED}')")
    conn.commit()
    cursor.execute(f"SELECT i FROM {TABLE}")
    data = cursor.fetchone()
    print(f'Data is:{data}')
    cursor.execute(f"DROP TABLE {TABLE}")
    conn.commit()
    cursor.close()
    conn.close()
    return data

def test_utf8():
    data = create_insert_select('utf8')
    assert data[0] == EXPECTED

def test_utf8mb4():
    data = create_insert_select('utf8mb4')
    assert data[0] == EXPECTED

def test_utf8mb3():
    data = create_insert_select('utf8mb3')
    assert data[0] == EXPECTED

def test_utf16():
    data = create_insert_select('utf16')
    assert data[0] == EXPECTED

def test_ucs2():
    data = create_insert_select('ucs2')
    assert data[0] == EXPECTED

def test_ascii():
    data = create_insert_select('ascii')
    assert data[0] == EXPECTED

def test_latin1():
    data = create_insert_select('latin1')
    assert data[0] == EXPECTED

def test_latin2():
    data = create_insert_select('latin2')
    assert data[0] == EXPECTED

def test_greek():
    data = create_insert_select('greek')
    assert data[0] == EXPECTED

def test_hebrew():
    data = create_insert_select('hebrew')
    assert data[0] == EXPECTED

def test_latin5():
    data = create_insert_select('latin5', 'turkish_ci')
    assert data[0] == EXPECTED

def test_tis620():
    data = create_insert_select('tis620', 'thai_ci')
    assert data[0] == EXPECTED
