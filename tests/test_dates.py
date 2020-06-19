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
    cursor.execute(f"SELECT * FROM {TABLE}")
    data = cursor.fetchone()
    print(f'Data: {data}')
    cursor.execute(f"DROP TABLE {TABLE}")
    conn.commit()
    cursor.close()
    conn.close()
    return data

def test_date():
    data = create_insert_select('x date, y date, z date', "'2010-01-12', '120314', '13*04*21'")
    assert data[0] == datetime.date(2010, 1, 12)
    assert data[1] == datetime.date(2012, 3, 14)
    assert data[2] == datetime.date(2013, 4, 21)

def test_time():
    data = create_insert_select('i time, u time', "'9:6:3', 151413")
    assert data[0] == datetime.timedelta(0, 32763)
    assert data[1] == datetime.timedelta(0, 54853)

def test_year():
    data = create_insert_select('i year, u year', "1990, '2012'")

    assert data[0] == 1990
    assert data[1] == 2012

def test_datetime():
    data = create_insert_select('i datetime, u datetime', "'2011-03-11', '2012-04-19 13:08:22'")
    assert data[0] == datetime.datetime(2011, 3, 11, 0, 0)
    assert data[1] == datetime.datetime(2012, 4, 19, 13, 8, 22)

def test_timestamp():
    data = create_insert_select('i timestamp', "'2012-04-19 13:08:22'")
    assert data[0] == datetime.datetime(2012, 4, 19, 13, 8, 22)
