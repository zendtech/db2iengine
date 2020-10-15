import mysql.connector as mariadb
from os import getenv

TABLE = getenv('DB2I_TABLE') or 'test'
DB = getenv('DB2I_DB') or 'db2itest'
HOST = getenv('DB2I_HOST') or '127.0.0.1'
USER = getenv('DB2I_USER') or 'root'
PASS = getenv('DB2I_PASS') or ''
EXPECTED = getenv('DB2I_EXPECTED') or 'Hello World'

def create_insert_select(charset, collation="general_ci", expected=EXPECTED, encoding=None):
    collation = f'{charset}_{collation}'
    is_unicode = True if 'utf' in charset or 'ucs' in charset else False
    use_unicode = True if is_unicode else False
    conn_charset = 'utf8' if is_unicode else charset
    conn_collation = 'utf8_general_ci' if is_unicode else collation

    conn = mariadb.connect(user=USER, password=PASS, host=HOST, database=DB,
        charset=conn_charset, collation=conn_collation, use_unicode=use_unicode)

    cursor = conn.cursor()
    cursor.execute(f"DROP TABLE IF EXISTS {TABLE}")
    conn.commit()
    cursor.execute(f"CREATE TABLE {TABLE} (i char(255)) engine=ibmdb2i CHARACTER SET '{charset}' COLLATE '{collation}'")
    cursor.execute(f"INSERT INTO {TABLE} VALUES(%s)", (expected,))
    conn.commit()
    cursor.execute(f"SELECT i FROM {TABLE}")
    data = cursor.fetchone()
    if encoding:
        data = data[0].decode(encoding)
    else:
        data = data[0]

    print(f'Data is: {data}')
    cursor.execute(f"DROP TABLE {TABLE}")
    conn.commit()
    cursor.close()
    conn.close()
    return data

def test_utf8():
    data = create_insert_select('utf8')
    assert data == EXPECTED

def test_utf8mb4():
    data = create_insert_select('utf8mb4')
    assert data == EXPECTED

def test_utf8mb3():
    data = create_insert_select('utf8mb3')
    assert data == EXPECTED

def test_utf16():
    data = create_insert_select('utf16')
    assert data == EXPECTED

def test_ucs2():
    data = create_insert_select('ucs2')
    assert data == EXPECTED

def test_ascii():
    # https://en.wikipedia.org/wiki/ASCII#Character_set
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F))]).decode('ascii')
    data = create_insert_select('ascii', expected=EXPECTED, encoding='ascii')
    assert data == EXPECTED

def test_latin1():
    # https://en.wikipedia.org/wiki/ISO/IEC_8859-1#Code_page_layout
    #  CCSID 1148 does not include ¤ (00A4) because its replaced by €
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA0, 0XA4))
    + tuple(range(0XA5, 0X100))]).decode('iso8859-1')
    data = create_insert_select('latin1', expected=EXPECTED, encoding='iso8859-1')
    assert data == EXPECTED

def test_latin2():
    # CCSID 1153 does not include ¤ (00A4) because its replaced by €
    # https://en.wikipedia.org/wiki/ISO/IEC_8859-2#Code_page_layout
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA0, 0XA4)) 
    + tuple(range(0XA5, 0X100))]).decode('iso8859-2')
    data = create_insert_select('latin2', expected=EXPECTED, encoding='iso8859-2')
    assert data == EXPECTED

def test_greek():
    # https://en.wikipedia.org/wiki/ISO/IEC_8859-7#Codepage_layout
    # CCSID 4971 does not support ₯ drachma sign (0XA5) and ͺ Greek ypogegrammeni (0XAA)
    # these are available in CCSID 9067 (March 2005) ?
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA0, 0XA5))
     + tuple(range(0XA6, 0XAA)) + tuple(range(0XAB, 0XAE)) + tuple(range(0XAF, 0XD2))
     + tuple(range(0XD4, 0XFF))]).decode('iso8859-7')
    data = create_insert_select('greek', expected=EXPECTED, encoding='iso8859-7')
    assert data == EXPECTED

def test_hebrew():
    # https://en.wikipedia.org/wiki/ISO/IEC_8859-8#Codepage_layout
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA0, 0XA1))
    + tuple(range(0XA2, 0XBF)) + tuple(range(0XDF, 0XFB))]).decode('iso8859-8')
    data = create_insert_select('hebrew', expected=EXPECTED, encoding='iso8859-8')
    assert data == EXPECTED

def test_latin5():
    # https://en.wikipedia.org/wiki/ISO/IEC_8859-9#Codepage_layout
    # CCSID 1155  does not include ¤ (00A4) because its replaced by €
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA0, 0XA4))
    + tuple(range(0XA5, 0X100))]).decode('iso8859-9')
    data = create_insert_select('latin5', 'turkish_ci', expected=EXPECTED, encoding='iso8859-9')
    assert data == EXPECTED

def test_tis620():
    # https://en.wikipedia.org/wiki/Thai_Industrial_Standard_620-2533#Character_set
    # The sole difference is that ISO/IEC 8859-11 allocates non-breaking space to code 0xA0,
    # while TIS-620 leaves it undefined. (In practice, this small distinction is usually ignored.)
    # Here we skip over nbsp 0xA0
    EXPECTED = b''.join([bytes([i]) for i in tuple(range(0X20, 0X7F)) + tuple(range(0XA1, 0XDB))
    + tuple(range(0XDF, 0XFC))]).decode('iso8859-11')
    data = create_insert_select('tis620', 'thai_ci', expected=EXPECTED, encoding='iso8859-11')
    assert data == EXPECTED
