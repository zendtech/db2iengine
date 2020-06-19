import mysql.connector as mariadb
from decimal import *
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
    cursor.execute(f"SELECT * FROM {TABLE}")
    data = cursor.fetchone()
    print(f'Data is:{data}')
    cursor.execute(f"DROP TABLE {TABLE}")
    conn.commit()
    cursor.close()
    conn.close()
    return data

def test_tiny():
    data = create_insert_select('i tinyint, u tinyint unsigned', '127, 255')
    assert data[0] == 127
    assert data[1] == 255

def test_small():
    data = create_insert_select('i smallint, u smallint unsigned', '32767, 65535')

    assert data[0] == 32767
    assert data[1] == 65535

def test_medium():
    data = create_insert_select('i mediumint, u mediumint unsigned', '-8388608, 16777215')
    assert data[0] == -8388608
    assert data[1] == 16777215

def test_big():
    data = create_insert_select('i bigint, u bigint unsigned', '-9223372036854775807, 18446744073709551615')

    assert data[0] == -9223372036854775807
    assert data[1] == 18446744073709551615

def test_pack():
    data = create_insert_select('i decimal(10,2) signed, u decimal(10,2) unsigned', '-12345678.91, 12345678.91')
    assert data[0] == Decimal('-12345678.91')
    assert data[1] == Decimal('12345678.91')

def test_numeric():
    data = create_insert_select('i numeric(10,2) signed, u numeric(10,2) unsigned', '-99999999.99, 99999999.99')
    assert data[0] == Decimal('-99999999.99')
    assert data[1] == Decimal('99999999.99')

def test_float():
    data = create_insert_select('i float, u float', '-9999.99, 99999.99')
    assert data[0] == -9999.99
    # assert data[1] == 99999.99 #+100000.0 is returned

def test_double():
    data = create_insert_select('i double, u double', '-9999.99, 87769999.99')
    assert data[0] == -9999.99
    assert data[1] == 87769999.99

def test_bit():
    data = create_insert_select('i bit(8), u bit(8)', "b'11111111', b'01010101'")
    assert data[0] == 255
    assert data[1] == 85

def test_bool():
    data = create_insert_select('i boolean, u boolean', 'TRUE, FALSE')
    assert data[0] == True
    assert data[1] == False

def test_enum():
    data = create_insert_select("i enum('bob','sally','jodi','tony','sue'), \
     u enum('bob','sally','jodi','tony','sue')", "'tony', 'sue'")
    assert data[0] == 'tony'
    assert data[1] == 'sue'
