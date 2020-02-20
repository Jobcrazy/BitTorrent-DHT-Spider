all:
	gcc -O2 -o dhtspy  -lstdc++ -lmysqlclient P2SP.cpp -I/usr/local/libtorrent/include/ -I/usr/local/boost/include/ -L/usr/local/libtorrent/lib/ -I/usr/local/mysql/include/ -L/usr/local/mysql/lib/ -ltorrent-rasterbar  -liconv -L/usr/local/boost/lib/ -lboost_system -Wl,-rpath,/usr/local/libtorrent/lib/,-rpath,/usr/local/boost/lib/,-rpath,/usr/local/mysql/lib/,-rpath,/usr/local/lib/

log:
	gcc -O2 -o dhtspy  -lstdc++ -lmysqlclient P2SP.cpp -I/usr/local/libtorrent/include/ -I/usr/local/boost/include/ -L/usr/local/libtorrent/lib/ -I/usr/local/mysql/include/ -L/usr/local/mysql/lib/ -ltorrent-rasterbar  -liconv -L/usr/local/boost/lib/ -lboost_system -Wl,-rpath,/usr/local/libtorrent/lib/,-rpath,/usr/local/boost/lib/,-rpath,/usr/local/mysql/lib/,-rpath,/usr/local/lib/ -DLOG

debug:
	gcc -g -o dhtspy  -lstdc++ -lmysqlclient P2SP.cpp -I/usr/local/libtorrent/include/ -I/usr/local/boost/include/ -L/usr/local/libtorrent/lib/ -I/usr/local/mysql/include/ -L/usr/local/mysql/lib/ -ltorrent-rasterbar  -liconv -L/usr/local/boost/lib/ -lboost_system -Wl,-rpath,/usr/local/libtorrent/lib/,-rpath,/usr/local/boost/lib/,-rpath,/usr/local/mysql/lib/,-rpath,/usr/local/lib/ 
