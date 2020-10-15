# ibmdb2i Tests

## Setup

```bash
$ pip3 install -r requirements.txt

$ mysqld_safe &

$ mysql 

MariaDB [(none)]> create database db2itest;
MariaDB [(none)]> install plugin ibmdb2i soname "ha_ibmdb2i.so";
MariaDB [(none)]> quit
```

## Run tests

```bash
$ pytest
```

## Configure

- `DB2I_DB` - The database to use. Defaults to use `db2itest`.

- `DB2I_HOST` - The host to connect to. Defaults to use `localhost`.

- `DB2I_USER`- The user to connect as. Defaults to `root`.

- `DB2I_PASS` - The user's password. Defaults to `''`.

