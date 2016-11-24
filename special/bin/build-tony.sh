echo "Building Mariadb"
export M4=/opt/freeware/bin/m4
# Dave add -malign-power -malign-natural (below)
# Our power engine will run better. Also fewer gcc odd boundary over runs.
cmake . -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS_DEBUG="-O0 -g -mminimal-toc -mcpu=power6 -malign-power -malign-natural -Wl,-bnoquiet" \
    -DCMAKE_C_FLAGS_DEBUG="-O0 -g -mminimal-toc -mcpu=power6 -malign-power -malign-natural -Wl,-bnoquiet" \
    -DCMAKE_INSTALL_PREFIX=/usr/local/mariadb \
    -DMYSQL_DATADIR=/usr/local/mariadb/data \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE \
    -DCMAKE_INSTALL_RPATH=/usr/local/mariadb/lib \
    -DWITH_EMBEDDED_SERVER=OFF \
    -DWITH_UNIT_TESTS=OFF \
    -DINSTALL_MYSQLTESTDIR= \
    -DPLUGIN_FEEDBACK=DYNAMIC \
    -DPLUGIN_FILE_KEY_MANAGEMENT=NO \
    -DPLUGIN_FTEXAMPLE=DYNAMIC \
    -DPLUGIN_HANDLERSOCKET=DYNAMIC \
    -DPLUGIN_LOCALES=DYNAMIC \
    -DPLUGIN_METADATA_LOCK_INFO=DYNAMIC \
    -DPLUGIN_MYSQL_CLEAR_PASSWORD=DYNAMIC \
    -DPLUGIN_PERFSCHEMA=NO \
    -DPLUGIN_QA_AUTH_CLIENT=DYNAMIC \
    -DPLUGIN_QA_AUTH_INTERFACE=DYNAMIC \
    -DPLUGIN_QA_AUTH_SERVER=DYNAMIC \
    -DPLUGIN_QUERY_CACHE_INFO=DYNAMIC \
    -DPLUGIN_QUERY_RESPONSE_TIME=DYNAMIC \
    -DPLUGIN_SEMISYNC_MASTER=DYNAMIC \
    -DPLUGIN_SEMISYNC_SLAVE=DYNAMIC \
    -DPLUGIN_SEQUENCE=DYNAMIC \
    -DPLUGIN_SERVER_AUDIT=DYNAMIC \
    -DPLUGIN_SIMPLE_PASSWORD_CHECK=DYNAMIC \
    -DPLUGIN_SPHINX=NO \
    -DPLUGIN_SQL_ERRLOG=DYNAMIC \
    -DPLUGIN_TEST_SQL_DISCOVERY=DYNAMIC \
    -DPLUGIN_WSREP_INFO=DYNAMIC \
    -DPLUGIN_XTRADB=NO \
    -DPLUGIN_IBMDB2I=DYNAMIC \
    -DDISABLE_SHARED=OFF \
    -DWITH_READLINE=OFF \
    -DCURSES_CURSES_H_PATH=/usr/include \
    -DCURSES_CURSES_LIBRARY=/usr/lib/libcurses.a \
    -DCURSES_EXTRA_LIBRARY=CURSES_EXTRA_LIBRARY-NOTFOUND \
    -DCURSES_FORM_LIBRARY=/usr/lib/libform.a \
    -DCURSES_HAVE_CURSES_H=/usr/include/curses.h \
    -DCURSES_INCLUDE_PATH=/usr/include \
    -DCURSES_LIBRARY=/usr/lib/libcurses.a \
    -DCURSES_NCURSES_LIBRARY=/usr/lib/libncurses.a

make install

