English Manual? [Click Here](README.md)

## 简介
这是一个C++版本的磁力链蜘蛛，它可以帮助你从DHT网络里全自动的抓取最新的BT种子信息。经实测，它的抓取速度为每天3000-9000个种子。

## 开发及运行环境
* 系统: Linux
* 数据库: MySQL >= 5.5.5
* 编译器: gcc, gcc-c++
* 依赖库: 
   1. OpenSSL
   2. [Boost >= 1.59](https://www.boost.org/users/history/)
   3. [libtorrent = 1.1.x](https://github.com/arvidn/libtorrent/releases/tag/libtorrent-1_1_10)

## 配置MySQL
1. 创建dbooster_torrents数据库
2. 创建dbooster用户，其密码为UxtmU6w3wNjYUMvV
3. 设置dbooster用户的登录信息为127.0.0.1
4. 设置dbooster用户只能对dbooster_torrents读写
5. 将database目录里的dbooster_torrents.sql导入到dbooster_torrents库


## 编译步骤
### 编译依赖库
* 一般Linux系统都自带OpenSSL，无需额外编译。比如CentOS里运行如下指令即可安装：`yum install openssl-devel`

* 下载并编译BOOST

```
$ wget https://sourceforge.net/projects/boost/files/boost/1.59.0/boost_1_59_0.tar.gz/download
$ tar -xzf boost_1_59_0.tar.gz
$ cd boost_1_59_0
$ ./bootstrap.sh --prefix=/usr/local/boost
$ ./b2 install --prefix=/usr/local/boost
```

* 下载并编译libtorrent

```
$ wget https://github.com/arvidn/libtorrent/releases/download/libtorrent-1_1_10/libtorrent-rasterbar-1.1.10.tar.gz
$ tar -xzf libtorrent-rasterbar-1.1.10.tar.gz
$ cd libtorrent-rasterbar-1.1.10
$ ./configure --prefix=/usr/local/libtorrent
$ make && make install
```

* 编译本项目

```
首先要进入本工程的dhtspider目录
$ cd /path/to/project/dhtspider

然后直接编译: 
$ make
```

## 运行

* 编译完成spider后即可运行

  前台运行: `$ ./dhtspy`

  后台运行: `$ nohup ./dhtspy &`

* 注意:
  1. 前台运行时可以在屏幕上看到日志输出，协助判断问题
  2. 后台运行时，日志输出到nohup文件内
  3. 所有采集到的种子信息都会存储到MySQL数据库内
  4. 部分网络环境不允许BT协议（比如阿里云），无法采集到种子，建议在海外允许BT协议的机房使用