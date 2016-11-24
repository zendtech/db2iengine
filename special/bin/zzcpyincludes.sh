#!/QOpenSys/usr/bin/ksh
ls /QIBM/include/*lgu* > ./list2
for i in $(< ./list2)
do
  echo "PREPARATION COPY"
  system -v "CPY OBJ('$i') TODIR('/usr/include/') TOCCSID(*STDASCII) DTAFMT(*TEXT) REPLACE(*YES)"                                      
done
ls /QIBM/include/*my* > ./list2
for i in $(< ./list2)
do
  echo "PREPARATION COPY"
  system -v "CPY OBJ('$i') TODIR('.') TOCCSID(*STDASCII) DTAFMT(*TEXT) REPLACE(*YES)"                                      
done
