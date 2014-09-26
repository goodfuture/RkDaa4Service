#ifndef RKPROTOCOL_H
#define RKPROTOCOL_H

#include "rkcrc.h"
#include "rkcom.h"

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib,"Ws2_32.lib.")
#else
#include <arpa/inet.h>
#endif

#include "rkcom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG_RECV_MESSAGE_RAW_OUTPUT
//#define DEBUG_SEND_MESSAGE_RAW_OUTPUT
//#define DEBUG_MESSAGE_PARSE_OUTPUT

#define RK_PROTOCOL_HEADER			 	0xABCDEF
#define RK_PROTOCOL_CW_SEG_OFFSET		3
#define RK_PROTOCOL_CW_SEG_LEN			1
#define RK_PROTOCOL_PKT_LEN_SEG_OFFSET	(RK_PROTOCOL_CW_SEG_OFFSET + RK_PROTOCOL_CW_SEG_LEN)
#define RK_PROTOCOL_PKT_LEN_SEG_LEN		2
#define RK_PROTOCOL_HEADER_LEN			(RK_PROTOCOL_PKT_LEN_SEG_OFFSET + RK_PROTOCOL_PKT_LEN_SEG_LEN)
#define RK_PROTOCOL_TAIL_LEN			2
#define RK_PROTOCOL_MESSAGE_MAX_LEN		1024

#define RK_FTP_HEADER_SEQ_SEG_LEN		2
#define RK_FTP_HEADER_FLAG_SEG_OFFSET	2
#define RK_FTP_HEADER_FLAG_SEG_LEN		1
#define RK_FTP_HEADER_LEN_SEG_OFFSET	(RK_FTP_HEADER_FLAG_SEG_OFFSET + RK_FTP_HEADER_FLAG_SEG_LEN)
#define RK_FTP_HEADER_LEN_SEG_LEN		2
#define RK_FTP_HEADER_LEN				(RK_FTP_HEADER_LEN_SEG_OFFSET + RK_FTP_HEADER_LEN_SEG_LEN)
#define RK_FTP_MESSAGE_MAX_LEN			(RK_PROTOCOL_MESSAGE_MAX_LEN - RK_FTP_HEADER_LEN - RK_PROTOCOL_TAIL_LEN)

#define CW_USER_LOGIN					0x00
#define CW_GET_PARAM_CONFIG				0x01
#define CW_PUT_PARAM_CONFIG				0x02
#define CW_SYN_REALTIME_DATA			0x03
#define CW_QUERY_HISTORY_DATA			0x04
#define CW_EXPORT_HISTORY_DATA			0x05
#define CW_RESTORE_FACTORY_SETTING		0x06
#define CW_GET_FIRMWARE_VERSION			0x07
#define CW_GET_SUPPORTED_PROTOCOL		0x08
#define CW_UPGRADE_DTU_TOOL_FIRMWARE	0xFC
#define CW_UPGRADE_HMI_FIRMWARE			0xFD
#define CW_UPGRADE_BLP_FIRMWARE			0xFE
#define CW_REBOOT_DEVICE				0xFF

#define CW_ECHO_OF_USER_LOGIN				(CW_USER_LOGIN + 0x80)
#define CW_ECHO_OF_GET_PARAM_CONFIG			(CW_GET_PARAM_CONFIG + 0x80)
#define CW_ECHO_OF_PUT_PARAM_CONFIG			(CW_PUT_PARAM_CONFIG + 0x80)
#define CW_ECHO_OF_SYN_REALTIME_DATA		(CW_SYN_REALTIME_DATA + 0x80)
#define CW_ECHO_OF_QUERY_HISTORY_DATA		(CW_QUERY_HISTORY_DATA + 0x80)
#define CW_ECHO_OF_EXPORT_HISTORY_DATA		(CW_EXPORT_HISTORY_DATA + 0x80)
#define CW_ECHO_OF_RESTORE_FACTORY_SETTING	(CW_RESTORE_FACTORY_SETTING + 0x80)
#define CW_ECHO_OF_GET_FIRMWARE_VERSION		(CW_GET_FIRMWARE_VERSION + 0x80)
#define CW_ECHO_OF_GET_SUPPORTED_PROTOCOL	(CW_GET_SUPPORTED_PROTOCOL + 0x80)
#define CW_ECHO_OF_UPGRADE_HMI_FIRMWARE		CW_UPGRADE_HMI_FIRMWARE
#define CW_ECHO_OF_UPGRADE_BLP_FIRMWARE		CW_UPGRADE_BLP_FIRMWARE	//Bussiness Layer Program
#define CW_ECHO_OF_REBOOT_DEVICE			CW_REBOOT_DEVICE

#define TLV_TAG_SEG_OFFSET		0
#define TLV_TAG_SEG_LEN			2
#define TLV_LEN_SEG_OFFSET		2
#define TLV_LEN_SEG_LEN			2
#define TLV_VALUE_SEG_OFFSET	(TLV_LEN_SEG_OFFSET + TLV_LEN_SEG_LEN)

#define FTP_PKT_FLAG_LAST	0x01
#define FTP_PKT_FLAG_ACK	0x10
#define FTP_PKT_FLAG_RESEND	0x20
#define FTP_PKT_FLAG_ERROR	0x40

#define FIRMWARE_UPDATE_TMP_DIR					"/home/app/shucaiyi/tmp"
#define FIRMWARE_UPDATE_HMI_TMP_FILE			"GUI.bin"
#define FIRMWARE_UPDATE_HMI_FILE_PATH			GUI_PROG_PATH
#define FIRMWARE_UPDATE_BLP_TMP_FILE			"Core.bin"
#define FIRMWARE_UPDATE_BLP_FILE_PATH			CORE_PROG_PATH
#define FIRMWARE_UPDATE_DTU_TOOL_TMP_FILE		"Inhand.bin"
#define FIRMWARE_UPDATE_DTU_TOOL_FILE_PATH		DTU_CFG_PROG_PATH

//Login Tags
#define TLV_TAG_LOGIN_USER					0x0001
#define TLV_TAG_LOGIN_PASSWORD				0x0002

//Basic Parameter Related Tags
#define TLV_TAG_ALL_SYS_PARAM			0x0100
#define TLV_TAG_SIM_ID					0x0101
#define TLV_TAG_MN_ID					0x0102
#define TLV_TAG_SYSTEM_TYPE				0x0103
#define TLV_TAG_STORAGE_INTERVAL		0x0104
#define TLV_TAG_UPLOAD_TYPE				0x0105
#define TLV_TAG_UPLOAD_INTERVAL			0x0106
#define TLV_TAG_RTD_UPLOAD				0x0107
#define TLV_TAG_TMD_UPLOAD				0x0108
#define TLV_TAG_HSD_UPLOAD				0x0109
#define TLV_TAG_DSD_UPLOAD				0x010A
#define TLV_TAG_SWITCH_SENSE			0x010B
#define TLV_TAG_ALARM_ENABLE			0x010C

//Com Parameter Related Tags
#define TLV_TAG_ALL_COM_PARAM			0x0200
#define TLV_TAG_COM1_PARAM				0x0201
#define TLV_TAG_COM1_PROTOCOL			0x0202
#define TLV_TAG_COM2_PARAM				0x0203
#define TLV_TAG_COM2_PROTOCOL			0x0204
#define TLV_TAG_COM3_PARAM				0x0205
#define TLV_TAG_COM3_PROTOCOL			0x0206
#define TLV_TAG_COM4_PARAM				0x0207
#define TLV_TAG_COM4_PROTOCOL			0x0208

//Net Parameter Related Tags
#define TLV_TAG_ALL_NET_PARAM			0x0300
#define TLV_TAG_LINK_MODE				0x0301
#define TLV_TAG_DEVICE_IP_ADDR			0x0302
#define TLV_TAG_DEVICE_NETMASK			0x0303
#define TLV_TAG_REMOTE_IP_ADDR			0x0304
#define TLV_TAG_REMOTE_PORT				0x0305

//Analog Parameter Related Tags
#define TLV_TAG_ALL_ANALOG_PARAM		0x0400
#define TLV_TAG_ANALOG_CH1_PARAM		0x0401
#define TLV_TAG_ANALOG_CH16_PARAM		0x0410

//Serial Parameter Related Tags
#define TLV_TAG_ALL_SERIAL_PARAM		0x0500
#define TLV_TAG_SERIAL_CH1_PARAM		0x0501
#define TLV_TAG_SERIAL_CH32_PARAM		0x0520

//Dio Parameter Related Tags
#define TLV_TAG_ALL_DIO_PARAM			0x0600
#define TLV_TAG_DI_CH1_PARAM			0x0601
#define TLV_TAG_DI_CH8_PARAM			0x0608
#define TLV_TAG_DO_CH1_PARAM			0x0609
#define TLV_TAG_DO_CH8_PARAM			0x0610

//Dtu Parameter Related Tags
#define	TLV_TAG_ALL_DTU_PARAM			0x0700

//Firmware Version Related Tags
#define TLV_TAG_ALL_FIRMWARE_VERSION	0xFF00
#define TLV_TAG_HMI_VERSION				0xFF01
#define TLV_TAG_BLP_VERSION				0xFF02
#define TLV_TAG_DTU_TOOL_VERSION		0xFF03

//Set Structure Aligned By 1Byte
#pragma pack(1)
typedef struct rkMsgPkt {
	uint8_t		header[3];
	uint8_t		cw;
	uint16_t	len;
	uint8_t 	msg[RK_PROTOCOL_MESSAGE_MAX_LEN];
} rkMsgPkt_t;
#pragma pack()

#pragma pack(1)
typedef struct rkFtpMsgPkt {
	uint16_t	seq;
	uint8_t		flag;
	uint16_t	len;
	uint8_t		msg[RK_FTP_MESSAGE_MAX_LEN];
} rkFtpMsgPkt_t;
#pragma pack()

void rkInitMsgHeader(rkMsgPkt_t *pkt, uint8_t cw);
void rkFillMsgTail(rkMsgPkt_t *pkt);
int rkPushTlvToMsg(rkMsgPkt_t *pkt, uint16_t tag, uint16_t len, const void *value);

enum ParamType {
    AllParam,
    SystemParam,
    ComParam,
    NetParam,
    AnalogParam,
    SerialParam,
    DioParam,
    DtuParam
};

uint8_t rkCheckLoginPermit(rkMsgPkt_t *pkt);
int rkGenEchoMsgOfLogin(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfGetConfig(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfPutConfig(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfUpgradeFirmware(int type, rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfUpgradeBlpFirmware(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfGetConfig(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfRebootDevice(rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfGetFirmwareVersion(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg);
int rkGenEchoMsgOfGetSupportedProtocol(rkMsgPkt_t *sendMsg);

int rkPushConfigTlvsToMsg(rkMsgPkt_t *pkt, enum ParamType type);
int rkPushSysConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushComConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushNetConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushAnalogConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushSerialConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushDioConfigTlvsToMsg(rkMsgPkt_t *pkt);
int rkPushDtuConfigTlvsToMsg(rkMsgPkt_t *pkt);

int rkVerifyMsgTail(rkMsgPkt_t *pkt);
int rkParseParamConfig(rkMsgPkt_t *pkt);
int rkPushConfigTlvToMSgByTag(rkMsgPkt_t *pkt, uint16_t tag);

#endif // RKPROTOCOL_H
