中文版说明书? [请点这里](README_CN.md)

## Introduction
This is a C++ magnet spider, and it can grab bittorrent infomation from the DHT network. According to some user reports, this sipder can grab 3000-9000 torrent files in just one day. 

## Development Environment
* System: Linux
* Database: MySQL >= 5.5.5
* Compiler: gcc, gcc-c++
* Dependencies: 
   1. OpenSSL
   2. [Boost >= 1.59](https://www.boost.org/users/history/)
   3. [libtorrent = 1.1.x](https://github.com/arvidn/libtorrent/releases/tag/libtorrent-1_1_10)

## Config MySQL
1. Create a database named "dbooster_torrents"
2. Create a database user named "dbooster" and set "UxtmU6w3wNjYUMvV" as its password
3. Set the host name of this user as "127.0.0.1"
4. Set the read/write access of database "dbooster_torrents" for this user
5. Import the file "dbooster_torrents.sql" to database "dbooster_torrents"
6. If you want to change the default login information, please edit "P2SP.cpp" 


## Compile
### Compile the dependencies
* OpenSSL: It is often included in a linux system, you need not to compile it. For example, you could install the openssl library via this command: `yum install openssl-devel`

* Download and compile Boost

```
$ wget https://sourceforge.net/projects/boost/files/boost/1.59.0/boost_1_59_0.tar.gz/download
$ tar -xzf boost_1_59_0.tar.gz
$ cd boost_1_59_0
$ ./bootstrap.sh --prefix=/usr/local/boost
$ ./b2 install --prefix=/usr/local/boost
```

* Download and compile libtorrent

```
$ wget https://github.com/arvidn/libtorrent/releases/download/libtorrent-1_1_10/libtorrent-rasterbar-1.1.10.tar.gz
$ tar -xzf libtorrent-rasterbar-1.1.10.tar.gz
$ cd libtorrent-rasterbar-1.1.10
$ ./configure --prefix=/usr/local/libtorrent
$ make && make install
```

* Compile the spider

```
You should go to the dhtspider directory of this project:
$ cd /path/to/project/dhtspider

and then compile: 
$ make
```

## Run

* You could launch the spider after it has been compiled

  Run in the forgeground: `$ ./dhtspy`

  Run in the background: `$ nohup ./dhtspy &`

* Remark:
  1. You can watch the log on the screen if the spider is running in the forgeground, and the log is helpful for you to diagnose if something wrong with the spider
  2. The log will be stored in the file "nohup" if the spider is running in the background
  3. All the collected information will be stored in MySQL database  
  4. BT protocol has been blocked by some networks(such as aliyun), the spider cannot get any torrent in these networks. It is suggested that run this project in some "proper" networks