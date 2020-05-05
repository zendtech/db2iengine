# ibmdb2i
MariaDB IBM i DB2 storage engine. 

version
```
1.0.3 (in development)
```

copy source
```
> mkdir -p storage/ibmdb2i/special
> unzip maria_ibmdb2i-1.0.3.zip
Note:
Use storage/ibmdb2i, not storage/ibmidb2i (Dave)
```

Fix-up for gcc 4.8.3
```
> cd storage/ibmdb2i/special
> chmod +x zzcpy2gccfixed.sh
> ./zzcpy2gccfixed.sh
```
Fixup for gcc 6.3 from IBM yum repo
cd storage/ibmdb2i/special
cp /usr/include/unistd.h
cp /QIBM/include/qmyse.h


mariadb build (subset myisam only)
```
> cd /usr/src/zenddbi
> chmod +x build-tony.sh
> ./build-tony.sh (tony subset)
-- or --
> ./build-zenddbi.sh (zend)
Note:
add -malign-power -malign-natural (Dave) 
Our power engine will run better. 
Also fewer gcc odd boundary over runs.
```


start
```
> cd /usr/local/mariadb
> ./bin/mysqld_safe& 
```

setup
```
$ bin/mysql -u root -p
Enter password: 
Welcome to the MariaDB monitor.  Commands end with ; or \g.
MariaDB [(none)]> create database ranger;
MariaDB [(none)]> CREATE USER 'ranger'@'localhost' IDENTIFIED BY 'ranger';
MariaDB [(none)]> GRANT ALL PRIVILEGES ON * . * TO 'ranger'@'localhost';
MariaDB [(none)]> FLUSH PRIVILEGES;
MariaDB [(none)]> select host, user, password from mysql.user;
```

test
```
> bin/mysql -u ranger -p
> use ranger

MariaDB [ranger]> CREATE TABLE itiny (i tinyint, u tinyint unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into itiny (i,u) values(127,255);
MariaDB [ranger]> select * from itiny; 

MariaDB [ranger]> CREATE TABLE ismall (i smallint, u smallint unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into itiny (i,u) values(32767,65535);
MariaDB [ranger]> select * from ismall; 

MariaDB [ranger]> CREATE TABLE imedium (i mediumint, u mediumint unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into imedium (i,u) values(-8388608,16777215);
MariaDB [ranger]> select * from imedium;

MariaDB [ranger]> CREATE TABLE ibig (i bigint, u bigint unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into ibig (i,u) values(9223372036854775807,18446744073709551615);
MariaDB [ranger]> select * from ibig;

MariaDB [ranger]> CREATE TABLE ipack (i decimal(10,2), u decimal(10,2) unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into ibig (i,u) values(12345678.91,12345678.91);
MariaDB [ranger]> select * from ipack;

MariaDB [ranger]> CREATE TABLE inum (i numeric(10,2), u numeric(10,2) unsigned) engine=ibmdb2i;
MariaDB [ranger]> insert into inum (i,u) values(-99999999.99,-99999999.99);
MariaDB [ranger]> select * from inum;

MariaDB [ranger]> CREATE TABLE ifloat (i float) engine=ibmdb2i;
MariaDB [ranger]> insert into ifloat (i) values(-9999.99);
MariaDB [ranger]> insert into ifloat (i) values(99999.99);
MariaDB [ranger]> select * from ifloat;

MariaDB [ranger]> CREATE TABLE idouble (i double) engine=ibmdb2i;
MariaDB [ranger]> insert into idouble (i) values(-9999.99);
MariaDB [ranger]> insert into idouble (i) values(87769999.99);
MariaDB [ranger]> select * from idouble;

MariaDB [ranger]> CREATE TABLE ibit (i bit(8)) engine=ibmdb2i;
MariaDB [ranger]> insert into ibit (i) values(b'11111111');
MariaDB [ranger]> insert into ibit (i) values(b'01010101');
MariaDB [ranger]> select i+0, HEX(i), OCT(i), BIN(i) from ibit;

MariaDB [ranger]> CREATE TABLE ichar10 (i char(10)) engine=ibmdb2i;
MariaDB [ranger]> insert into ichar10 (i) values('MariaDB');
MariaDB [ranger]> select * from ichar10;

MariaDB [ranger]> CREATE TABLE ivarchar (i varchar(10)) engine=ibmdb2i;
MariaDB [ranger]> insert into ivarchar (i) values('Maria   ');
MariaDB [ranger]> select * from ivarchar;

MariaDB [ranger]> CREATE TABLE itinytext (i tinytext) engine=ibmdb2i;
MariaDB [ranger]> insert into itinytext (i) values('A');
MariaDB [ranger]> insert into itinytext (i) values('AB');
MariaDB [ranger]> select * from itinytext;

MariaDB [ranger]> CREATE TABLE itext (i text) engine=ibmdb2i;
MariaDB [ranger]> insert into itext (i) values('ABC');
MariaDB [ranger]> insert into itext (i) values('12345678901234567890123456789012345678901234567890');
MariaDB [ranger]> select * from itext;

MariaDB [ranger]> CREATE TABLE imediumtext (i mediumtext) engine=ibmdb2i;
MariaDB [ranger]> insert into imediumtext (i) values('ABC');
MariaDB [ranger]> insert into imediumtext (i) values('12345678901234567890123456789012345678901234567890');
MariaDB [ranger]> select * from imediumtext;

MariaDB [ranger]> CREATE TABLE ilongtext (i longtext) engine=ibmdb2i;
MariaDB [ranger]> insert into ilongtext (i) values('ABC');
MariaDB [ranger]> insert into ilongtext (i) values('12345678901234567890123456789012345678901234567890');
MariaDB [ranger]> select * from ilongtext;

MariaDB [ranger]> CREATE TABLE itinyblob (i tinyblob) engine=ibmdb2i
MariaDB [ranger]> insert into itinyblob (i) values(x'DEADBEEF');
MariaDB [ranger]> insert into itinyblob (i) values('ABC');
MariaDB [ranger]> select HEX(i) from itinyblob;

MariaDB [ranger]> CREATE TABLE imediumblob (i mediumblob) engine=ibmdb2i;
MariaDB [ranger]> insert into imediumblob (i) values(x'DEADBEEF');
MariaDB [ranger]> insert into imediumblob (i) values(x'BADC0FFEE0DDF00D');
MariaDB [ranger]> insert into imediumblob (i) values('ABC');
MariaDB [ranger]> select HEX(i) from imediumblob;

MariaDB [ranger]> CREATE TABLE ilongblob (i longblob) engine=ibmdb2i
MariaDB [ranger]> insert into ilongblob (i) values('ABC');
MariaDB [ranger]> insert into ilongblob (i) values(x'BADC0FFEE0DDF00D');
MariaDB [ranger]> insert into ilongblob (i) values(x'DEADBEEF');
MariaDB [ranger]> select HEX(i) from ilongblob

MariaDB [ranger]> CREATE TABLE iblob (i blob) engine=ibmdb2i;
MariaDB [ranger]> insert into iblob (i) values('ABC');
MariaDB [ranger]> insert into iblob (i) values(b'1111111111111');
MariaDB [ranger]> insert into iblob (i) values(x'DEADBEEF');
MariaDB [ranger]> select HEX(i) from iblob;

MariaDB [ranger]> CREATE TABLE ibool (i boolean) engine=ibmdb2i;
MariaDB [ranger]> insert into ibool (i) values(TRUE); 
MariaDB [ranger]> insert into ibool (i) values(FALSE);
MariaDB [ranger]> select * from ibool;

MariaDB [ranger]> CREATE TABLE ibin (i binary(10)) engine=ibmdb2i;
MariaDB [ranger]> insert into ibin (i) values('12345678901');
MariaDB [ranger]> insert into ibin (i) values(xDEADBEEF);
MariaDB [ranger]> insert into ibin (i) values(x'BADC0FFEE0DDF00D');
MariaDB [ranger]> select HEX(i) from ibin;

MariaDB [ranger]> CREATE TABLE ivarbin (i varbinary(10)) engine=ibmdb2i;
MariaDB [ranger]> insert into ivarbin (i) values(x'BADC0FFEE0DDF00D');
MariaDB [ranger]> select HEX(i) from ivarbin;

MariaDB [ranger]> CREATE TABLE ienum (i enum('bob','sally','jodi','tony','sue')) engine=ibmdb2i;
MariaDB [ranger]> insert into ienum (i) values('tony');
MariaDB [ranger]> insert into ienum (i) values('sue');
MariaDB [ranger]> select * from ienum;

MariaDB [ranger]> CREATE TABLE idate (i date) engine=ibmdb2i;
MariaDB [ranger]> insert into idate (i) values("2010-01-12");
MariaDB [ranger]> insert into idate (i) values('120314');
MariaDB [ranger]> insert into idate (i) values('13*04*21');
MariaDB [ranger]> select * from idate;

MariaDB [ranger]> CREATE TABLE itime (i time) engine=ibmdb2i;
MariaDB [ranger]> insert into itime (i) values('9:6:3');
MariaDB [ranger]> insert into itime (i) values(151413);
MariaDB [ranger]> select * from itime;

MariaDB [ranger]> CREATE TABLE iyear (i year) engine=ibmdb2i
MariaDB [ranger]> insert into iyear (i) values(1990);
MariaDB [ranger]> insert into iyear (i) values('2012')
MariaDB [ranger]> select * from iyear;

MariaDB [ranger]> CREATE TABLE idatetime (i datetime) engine=ibmdb2i;
MariaDB [ranger]> insert into idatetime (i) values("2011-03-11");
MariaDB [ranger]> insert into idatetime (i) values("2012-04-19 13:08:22");
MariaDB [ranger]> select * from idatetime;

MariaDB [ranger]> CREATE TABLE istamp (i timestamp) engine=ibmdb2i;
MariaDB [ranger]> insert into istamp (i) values("2012-04-19 13:08:22");
MariaDB [ranger]> select * from istamp;

```

