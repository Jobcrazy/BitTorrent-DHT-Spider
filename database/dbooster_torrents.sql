SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- 数据库: `dbooster_torrents`
--
CREATE DATABASE `dbooster_torrents` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `dbooster_torrents`;

-- --------------------------------------------------------

--
-- 表的结构 `db_daycounter`
--

CREATE TABLE IF NOT EXISTS `db_daycounter` (
  `ymd` date NOT NULL,
  `today` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`ymd`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- 表的结构 `db_files`
--

CREATE TABLE IF NOT EXISTS `db_files` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `tid` int(10) unsigned NOT NULL,
  `fname` varchar(1024) NOT NULL DEFAULT '',
  `fpath` varchar(1024) NOT NULL,
  `fsize` bigint(20) unsigned DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `fmid_i` (`tid`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- 表的结构 `db_sphcounter`
--

CREATE TABLE IF NOT EXISTS `db_sphcounter` (
  `counter_id` int(11) NOT NULL,
  `max_doc_id` bigint(20) NOT NULL,
  PRIMARY KEY (`counter_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- 表的结构 `db_torrents`
--

CREATE TABLE IF NOT EXISTS `db_torrents` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `hash` char(40) DEFAULT NULL,
  `name` varchar(1024) DEFAULT NULL,
  `path` varchar(1024) DEFAULT NULL,
  `length` bigint(20) unsigned DEFAULT '0',
  `ctime` bigint(20) unsigned DEFAULT '0',
  `click` bigint(20) unsigned DEFAULT '0',
  `lastac` bigint(20) unsigned DEFAULT NULL,
  `date` date NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `infohash` (`hash`),
  KEY `date` (`date`),
  KEY `ctime` (`ctime`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;
