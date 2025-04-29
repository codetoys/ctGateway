
-- 注释到行尾（需要一个空格） #也是注释到行尾 
/*跨行注释*/

/*
开发工具
mysql-workbench-community-8.0.38-winx64.msi
数据库服务器（完全安装）
mysql-9.0.1-winx64.msi
默认选项
root/root
user1/user1

查询表和列
select table_name,column_name, is_nullable,column_type,column_comment from information_schema.columnS where table_schema = 'gateway'
    order by table_name,column_name;
*/

CREATE DATABASE  IF NOT EXISTS `gateway` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `gateway`;
-- MySQL dump 10.13  Distrib 8.0.38, for Win64 (x86_64)
--
-- Host: 127.0.0.1    Database: gateway
-- ------------------------------------------------------
-- Server version	9.0.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `gw_config`
--

DROP TABLE IF EXISTS `gw_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `gw_config` (
  `gw_sn` varchar(30) NOT NULL COMMENT '网关序列号',
  `gw_model` varchar(30) DEFAULT NULL COMMENT '型号',
  `gw_iccid` varchar(30) DEFAULT NULL COMMENT 'ICCID（如果有）',
  `sw_version` varchar(45) DEFAULT NULL COMMENT '软件版本',
  `confirmed_data_sequence` int DEFAULT NULL COMMENT '已确认的数据序列',
  `last_data_sequence` int DEFAULT NULL COMMENT '最新数据序列',
  `use_gm` int DEFAULT NULL COMMENT '是否使用国密',
  `key_seq` int DEFAULT NULL COMMENT '密钥标识（设置值）',
  PRIMARY KEY (`gw_sn`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='网关配置（平台侧）';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gw_config`
--

LOCK TABLES `gw_config` WRITE;
/*!40000 ALTER TABLE `gw_config` DISABLE KEYS */;
INSERT INTO `gw_config` VALUES ('GW-5G-VM-DELL','GW-5G',NULL,NULL,NULL,NULL,NULL,NULL);
/*!40000 ALTER TABLE `gw_config` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `gw_key`
--

DROP TABLE IF EXISTS `gw_key`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `gw_key` (
  `gw_sn` varchar(30) NOT NULL COMMENT '网关序列号',
  `gw_key_seq` int NOT NULL COMMENT 'key标识',
  `gw_key` varchar(128) DEFAULT NULL COMMENT 'key',
  PRIMARY KEY (`gw_sn`,`gw_key_seq`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='网关key信息';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gw_key`
--

LOCK TABLES `gw_key` WRITE;
/*!40000 ALTER TABLE `gw_key` DISABLE KEYS */;
INSERT INTO `gw_key` VALUES ('GW-5G-VM-DELL',0,'123'),('GW-5G-VM-DELL',1,'12345'),('GW-5G-VM-DELL',2,'1234567');
/*!40000 ALTER TABLE `gw_key` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `gw_message_list`
--

DROP TABLE IF EXISTS `gw_message_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `gw_message_list` (
  `id` int NOT NULL AUTO_INCREMENT,
  `gw_sn` varchar(30) DEFAULT NULL,
  `func` varchar(30) DEFAULT NULL,
  `ts_str` varchar(30) DEFAULT NULL,
  `insert_time` datetime DEFAULT CURRENT_TIMESTAMP,
  `topic` varchar(255) DEFAULT NULL,
  `message` mediumtext COMMENT '消息体，超长部分被丢弃',
  `uid` varchar(45) DEFAULT NULL COMMENT '消息标识',
  `respond_for_uid` varchar(45) DEFAULT NULL COMMENT '如果是应答，针对的uid',
  `data_seq` int DEFAULT NULL,
  `summary` varchar(255) DEFAULT NULL COMMENT '摘要，超过长度的部分被丢弃',
  PRIMARY KEY (`id`),
  KEY `i_sn` (`gw_sn`,`insert_time`)
) ENGINE=InnoDB AUTO_INCREMENT=22 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='网关消息列表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gw_message_list`
--

LOCK TABLES `gw_message_list` WRITE;
/*!40000 ALTER TABLE `gw_message_list` DISABLE KEYS */;
/*!40000 ALTER TABLE `gw_message_list` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `gw_report_info`
--

DROP TABLE IF EXISTS `gw_report_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `gw_report_info` (
  `gw_sn` varchar(30) NOT NULL COMMENT '网关序列号',
  `gw_model` varchar(30) DEFAULT NULL COMMENT '型号',
  `gw_iccid` varchar(30) DEFAULT NULL COMMENT 'ICCID（如果有）',
  `sw_version` varchar(45) DEFAULT NULL COMMENT '软件版本',
  `confirmed_data_sequence` int DEFAULT NULL COMMENT '已确认的数据序列',
  `last_data_sequence` int DEFAULT NULL COMMENT '最新数据序列',
  `start_time` varchar(30) DEFAULT NULL COMMENT '启动时间（程序）',
  `last_time` varchar(30) DEFAULT NULL COMMENT '上一次上报信息的时间（包含上报数据）',
  `last_data_time` varchar(30) DEFAULT NULL COMMENT '上一次上报数据的时间',
  `use_gm` int DEFAULT NULL COMMENT '是否使用国密',
  `key_seq` int DEFAULT NULL COMMENT '密钥标识（设置值）',
  `current_key_seq` int DEFAULT NULL COMMENT '当前密钥标识（使用值）',
  PRIMARY KEY (`gw_sn`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='网关列表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `gw_report_info`
--

LOCK TABLES `gw_report_info` WRITE;
/*!40000 ALTER TABLE `gw_report_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `gw_report_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_gw`
--

DROP TABLE IF EXISTS `tenant_gw`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tenant_gw` (
  `tenant_name` varchar(64) NOT NULL COMMENT '租户名称',
  `gw_sn` varchar(30) NOT NULL COMMENT '网关序列号',
  `gw_north_config_name` varchar(64) DEFAULT NULL COMMENT '网关北向配置',
  `gw_source_config_name` varchar(64) DEFAULT NULL COMMENT '网关南向配置',
  `gw_hardware_config_name` varchar(64) DEFAULT NULL COMMENT '网关硬件配置',
  PRIMARY KEY (`tenant_name`,`gw_sn`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='租户的网关';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_gw`
--

LOCK TABLES `tenant_gw` WRITE;
/*!40000 ALTER TABLE `tenant_gw` DISABLE KEYS */;
/*!40000 ALTER TABLE `tenant_gw` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_hardware_config`
--

DROP TABLE IF EXISTS `tenant_hardware_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tenant_hardware_config` (
  `tenant_name` varchar(64) NOT NULL COMMENT '租户名称',
  `hardware_config_name` varchar(64) NOT NULL,
  `hardware_cinfig_data` varchar(16000) DEFAULT NULL,
  PRIMARY KEY (`tenant_name`,`hardware_config_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='硬件配置';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_hardware_config`
--

LOCK TABLES `tenant_hardware_config` WRITE;
/*!40000 ALTER TABLE `tenant_hardware_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `tenant_hardware_config` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_info`
--

DROP TABLE IF EXISTS `tenant_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tenant_info` (
  `tenant_name` varchar(64) NOT NULL COMMENT '租户名称',
  `tenant_contact` varchar(45) DEFAULT NULL COMMENT '联系人',
  `tenant_phone_number` varchar(45) DEFAULT NULL COMMENT '电话',
  PRIMARY KEY (`tenant_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='租户信息';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_info`
--

LOCK TABLES `tenant_info` WRITE;
/*!40000 ALTER TABLE `tenant_info` DISABLE KEYS */;
INSERT INTO `tenant_info` VALUES ('default_tenant','联系人','电话');
/*!40000 ALTER TABLE `tenant_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_north_config`
--

DROP TABLE IF EXISTS `tenant_north_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tenant_north_config` (
  `tenant_name` varchar(64) NOT NULL COMMENT '租户名称',
  `north_config_name` varchar(32) NOT NULL COMMENT '配置名称',
  `north_type` varchar(45) DEFAULT NULL COMMENT 'MQTT/MQTTS',
  `host` varchar(45) DEFAULT NULL COMMENT '地址',
  `port` int DEFAULT NULL COMMENT '端口',
  `user` varchar(45) DEFAULT NULL COMMENT '用户名',
  `passowrd` varchar(45) DEFAULT NULL COMMENT '密码',
  PRIMARY KEY (`tenant_name`,`north_config_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='租户的北向配置';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_north_config`
--

LOCK TABLES `tenant_north_config` WRITE;
/*!40000 ALTER TABLE `tenant_north_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `tenant_north_config` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `tenant_south_config`
--

DROP TABLE IF EXISTS `tenant_south_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `tenant_south_config` (
  `tenant_name` varchar(64) NOT NULL COMMENT '租户名称',
  `south_config_name` varchar(64) NOT NULL,
  `south_config_data` mediumtext COMMENT '配置数据',
  PRIMARY KEY (`tenant_name`,`south_config_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='南向配置（数采）';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tenant_south_config`
--

LOCK TABLES `tenant_south_config` WRITE;
/*!40000 ALTER TABLE `tenant_south_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `tenant_south_config` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-04-29 12:03:05
