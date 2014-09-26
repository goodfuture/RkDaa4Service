#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rkxml.h"
#include "rktype.h"
#include "rkprotocol.h"

extern struct context ctx;

void rkInitMsgHeader(rkMsgPkt_t *pkt, uint8_t cw)
{
	pkt->header[0] = RK_PROTOCOL_HEADER >> 16 & 0xFF;
	pkt->header[1] = RK_PROTOCOL_HEADER >> 8 & 0xFF;
	pkt->header[2] = RK_PROTOCOL_HEADER & 0xFF;
	pkt->cw = cw;
	pkt->len = 0;
}

void rkFillMsgTail(rkMsgPkt_t *pkt)
{
	uint16_t crc16;
	crc16 = rkCrc16(pkt->msg, pkt->len);
	pkt->msg[pkt->len] = crc16 >> 8;
	pkt->msg[pkt->len + 1] = crc16 & 0xFF;
	pkt->len += RK_PROTOCOL_TAIL_LEN;
}

int rkPushTlvToMsg(rkMsgPkt_t *pkt, uint16_t tag, uint16_t len, const void *value)
{
	int need_len = RK_PROTOCOL_HEADER_LEN + pkt->len + RK_PROTOCOL_TAIL_LEN;
	need_len += TLV_TAG_SEG_LEN + TLV_LEN_SEG_LEN + len;
	if (need_len >= RK_PROTOCOL_MESSAGE_MAX_LEN) {
		return 0;
	}

	tag = htons(tag);
	uint8_t *ptr = pkt->msg + pkt->len;
	memcpy(ptr, &tag, TLV_TAG_SEG_LEN);
	uint16_t tag_value_len = htons(len);
	memcpy(ptr + TLV_LEN_SEG_OFFSET, &tag_value_len, TLV_LEN_SEG_LEN);
	memcpy(ptr + TLV_VALUE_SEG_OFFSET, value, len);
	pkt->len += TLV_VALUE_SEG_OFFSET + len;

	return 1;
}

int rkPushConfigTlvsToMsg(rkMsgPkt_t *pkt, enum ParamType type)
{
	int ret = 0;
	static int pos = 0;

	switch(type) {
		case SystemParam:
			ret = rkPushSysConfigTlvsToMsg(pkt);
			break;
		case ComParam:
			ret = rkPushComConfigTlvsToMsg(pkt);
			break;
		case NetParam:
			ret = rkPushNetConfigTlvsToMsg(pkt);
			break;
		case AnalogParam:
			ret = rkPushAnalogConfigTlvsToMsg(pkt);
			break;
		case SerialParam:
			ret = rkPushSerialConfigTlvsToMsg(pkt);
			break;
		case DioParam:
			ret = rkPushDioConfigTlvsToMsg(pkt);
			break;
		case DtuParam:
			ret = rkPushDtuConfigTlvsToMsg(pkt);
			break;
		case AllParam: 
			{
				switch(pos) {
					case 0:
						ret = rkPushSysConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 0;
							return 0;
						}
					case 1:
						ret = rkPushComConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 1;
							return 0;
						}
					case 2:
						ret = rkPushNetConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 2;
							return 0;
						}
					case 3:
						ret = rkPushAnalogConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 3;
							return 0;
						}
					case 4:
						ret = rkPushSerialConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 4;
							return 0;
						}
					case 5:
						ret = rkPushDioConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 5;
							return 0;
						}
					case 6:
						ret = rkPushDtuConfigTlvsToMsg(pkt);
						if (!ret) {
							pos = 6;
							return 0;
						}
					default:
						pos = 0;
						break;
				}
			}
			break; 
	}

	return ret;
}

int rkGenEchoMsgOfLogin(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg)
{
	uint8_t result = rkCheckLoginPermit(recvMsg);

	ctx.m_uLoginFlag = result;

	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_USER_LOGIN);
	sendMsg->len++;
	sendMsg->msg[0] = result;
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfGetSupportedProtocol(rkMsgPkt_t *sendMsg)
{
	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_GET_SUPPORTED_PROTOCOL);
	strcpy(sendMsg->msg, ctx.m_aDynamicProtocolSet);
	sendMsg->len += strlen(ctx.m_aDynamicProtocolSet);
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfRebootDevice(rkMsgPkt_t *sendMsg)
{
	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_REBOOT_DEVICE);
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfGetFirmwareVersion(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg)
{
	int pos = 0;
	uint16_t tag, len;

	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_GET_FIRMWARE_VERSION);

	while(pos < recvMsg->len - RK_PROTOCOL_TAIL_LEN) {
		tag = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		len = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		pos += len;

		switch(tag) {
			case TLV_TAG_ALL_FIRMWARE_VERSION:
				len = strlen(ctx.m_aVersion[0]);
				rkPushTlvToMsg(sendMsg, TLV_TAG_HMI_VERSION, len, ctx.m_aVersion[0]);
				len = strlen(ctx.m_aVersion[1]);
				rkPushTlvToMsg(sendMsg, TLV_TAG_BLP_VERSION, len, ctx.m_aVersion[1]);
				len = strlen(ctx.m_aVersion[2]);
				rkPushTlvToMsg(sendMsg, TLV_TAG_DTU_TOOL_VERSION, len, ctx.m_aVersion[2]);
				break;
			case TLV_TAG_HMI_VERSION:
				len = strlen(ctx.m_aVersion[0]);
				rkPushTlvToMsg(sendMsg, tag, len, ctx.m_aVersion[0]);
				break;
			case TLV_TAG_BLP_VERSION:
				len = strlen(ctx.m_aVersion[1]);
				rkPushTlvToMsg(sendMsg, tag, len, ctx.m_aVersion[1]);
				break;
			case TLV_TAG_DTU_TOOL_VERSION:
				len = strlen(ctx.m_aVersion[2]);
				rkPushTlvToMsg(sendMsg, tag, len, ctx.m_aVersion[2]);
				break;
			default:
				break;
		}
	}
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfUpgradeFirmware(int type, rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg)
{
	FILE *fp;
	int write_cnt;
	static long pos = 0;
	char tmp_file_path[1024];
	rkFtpMsgPkt_t *recvFtpMsg, *sendFtpMsg;

	if (type == 0) {
		sprintf(tmp_file_path, "%s/%s", FIRMWARE_UPDATE_TMP_DIR, FIRMWARE_UPDATE_HMI_TMP_FILE);
	} else if (type == 1) {
		sprintf(tmp_file_path, "%s/%s", FIRMWARE_UPDATE_TMP_DIR, FIRMWARE_UPDATE_BLP_TMP_FILE);
	} else if (type == 2) {
		sprintf(tmp_file_path, "%s/%s", FIRMWARE_UPDATE_TMP_DIR, FIRMWARE_UPDATE_DTU_TOOL_TMP_FILE);
	} else {
		sendFtpMsg->flag |= FTP_PKT_FLAG_ERROR;
		rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_UPGRADE_HMI_FIRMWARE);
		sendMsg->len += RK_FTP_HEADER_LEN;
		rkFillMsgTail(sendMsg);
		return 0;
	}

	recvFtpMsg = (rkFtpMsgPkt_t *)recvMsg->msg;
	sendFtpMsg = (rkFtpMsgPkt_t *)sendMsg->msg;

	sendFtpMsg->seq = recvFtpMsg->seq;
	sendFtpMsg->flag = recvFtpMsg->flag;
	sendFtpMsg->len = 0;

	recvFtpMsg->seq = ntohs(recvFtpMsg->seq);
	recvFtpMsg->len = ntohs(recvFtpMsg->len);
	//printf("seq = %d, flag = %02X, len = %d\n", recvFtpMsg->seq, recvFtpMsg->flag, recvFtpMsg->len);
	//printf("path : %s\n", tmp_file_path);

	if (recvFtpMsg->seq == 1) {
		if (access(FIRMWARE_UPDATE_TMP_DIR, F_OK)) {
			if (mkdir(FIRMWARE_UPDATE_TMP_DIR, 0777)) {
				fprintf(stderr, "%s\n", strerror(errno));
				sendFtpMsg->flag |= FTP_PKT_FLAG_ERROR;
				rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_UPGRADE_HMI_FIRMWARE);
				sendMsg->len += RK_FTP_HEADER_LEN;
				rkFillMsgTail(sendMsg);
				return 0;
			}
		}
		fp= fopen(tmp_file_path, "w+");
		pos = 0;
	} else {
		fp = fopen(tmp_file_path, "r+");
	}

	if (fp == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		sendFtpMsg->flag |= FTP_PKT_FLAG_ERROR;
		rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_UPGRADE_HMI_FIRMWARE);
		sendMsg->len += RK_FTP_HEADER_LEN;
		rkFillMsgTail(sendMsg);
		return 0;
	}

	fseek(fp, pos, SEEK_SET);
	write_cnt = fwrite(recvFtpMsg->msg, 1, recvFtpMsg->len, fp);
	fclose(fp);

	if (write_cnt != recvFtpMsg->len) {
		sendFtpMsg->flag |= FTP_PKT_FLAG_RESEND;
	} else {
		sendFtpMsg->flag |= FTP_PKT_FLAG_ACK;
		pos += write_cnt;
	}

	if (recvFtpMsg->flag & FTP_PKT_FLAG_LAST) {
		/*
		   fprintf(stderr, "Last1\n");
		   FILE *fp1 = fopen(FIRMWARE_UPDATE_HMI_FILE_TEMP_PATH, "r");
		   FILE *fp2 = fopen(FIRMWARE_UPDATE_HMI_FILE_SAVE_PATH, "w+");
		   fprintf(stderr, "Last2\n");
		   if (fp1 == NULL || fp2 == NULL) {
		   if (fp1 != NULL) {
		   fclose(fp1);
		   }
		   if (fp2 != NULL) {
		   fclose(fp2);
		   }
		   fprintf(stderr, "Error:%s\n", strerror(errno));
		   sendFtpMsg->flag &= ~FTP_PKT_FLAG_ACK;
		   sendFtpMsg->flag |= FTP_PKT_FLAG_ERROR;
		   } else {
		   uint16_t read_size, write_size;
		   uint8_t buf[1024];
		   puts("OPEN OK");
		   do {
		   read_size = fread(buf, 1, sizeof(buf), fp1);
		   write_size = fwrite(buf, 1, read_size, fp2);
		   printf("read_size = %d, write_size = %d\n", read_size, write_size);
		   } while(read_size);
		   fclose(fp1);
		   fclose(fp2);
		   }
		 */
	}

	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_UPGRADE_HMI_FIRMWARE);
	sendMsg->len += RK_FTP_HEADER_LEN;
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfGetConfig(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg)
{
	int ret;
	static int pos = 0;
	uint16_t tag, len;

	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_GET_PARAM_CONFIG);

	while(pos < recvMsg->len - RK_PROTOCOL_TAIL_LEN) {
		tag = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		len = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		pos += len;
		switch(tag) {
			case TLV_TAG_ALL_SYS_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, SystemParam);
				break;
			case TLV_TAG_ALL_COM_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, ComParam);
				break;
			case TLV_TAG_ALL_NET_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, NetParam);
				break;
			case TLV_TAG_ALL_ANALOG_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, AnalogParam);
				break;
			case TLV_TAG_ALL_SERIAL_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, SerialParam);
				break;
			case TLV_TAG_ALL_DIO_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, DioParam);
				break;
			case TLV_TAG_ALL_DTU_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, DtuParam);
				break;
			default:
				break;
		}
		if (ret == 0) {
			pos -= len;
			pos -= 4;
			rkFillMsgTail(sendMsg);
			return 0;
		}
	}

	pos = 0;
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkGenEchoMsgOfPutConfig(rkMsgPkt_t *recvMsg, rkMsgPkt_t *sendMsg)
{
	int ret;
	static int pos = 0;
	uint16_t tag, len;

	if (!pos) {
		rkParseParamConfig(recvMsg);
	}

	rkInitMsgHeader(sendMsg, (uint8_t)CW_ECHO_OF_PUT_PARAM_CONFIG);

	while(pos < recvMsg->len - RK_PROTOCOL_TAIL_LEN) {
		tag = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		len = recvMsg->msg[pos] << 8 | recvMsg->msg[pos + 1];
		pos += 2;
		pos += len;
		//printf("pos = %d, tag = %04X, len = %04X\n", pos, tag, len);
		switch(tag) {
			case TLV_TAG_ALL_SYS_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, SystemParam);
				break;
			case TLV_TAG_ALL_COM_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, ComParam);
				break;
			case TLV_TAG_ALL_NET_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, NetParam);
				break;
			case TLV_TAG_ALL_ANALOG_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, AnalogParam);
				break;
			case TLV_TAG_ALL_SERIAL_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, SerialParam);
				break;
			case TLV_TAG_ALL_DIO_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, DioParam);
				break;
			case TLV_TAG_ALL_DTU_PARAM:
				ret = rkPushConfigTlvsToMsg(sendMsg, DtuParam);
				break;
			default:
				ret = rkPushConfigTlvToMSgByTag(sendMsg, tag);
				break;
		}
		if (ret == 0) {
			pos -= len;
			pos -= 4;
			rkFillMsgTail(sendMsg);
			return 0;
		}
	}

	pos = 0;
	rkFillMsgTail(sendMsg);

	return 1;
}

int rkPushSysConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	static int pos = 0;
	int ret;

	switch(pos) {
		case 0:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_SIM_ID, strlen(ctx.m_tSystemParam.sim), ctx.m_tSystemParam.sim);
			if (!ret) {
				pos = 0;
				return 0;
			}
		case 1:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_MN_ID, strlen(ctx.m_tSystemParam.mn), ctx.m_tSystemParam.mn);
			if (!ret) {
				pos = 1;
				return 0;
			}
		case 2:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_SYSTEM_TYPE, 1, &ctx.m_tSystemParam.st);
			if (!ret) {
				pos = 2;
				return 0;
			}
		case 3: {
					uint16_t	value = ctx.m_tSystemParam.dsi;
					value = htons(value);
					ret = rkPushTlvToMsg(pkt, TLV_TAG_STORAGE_INTERVAL, 2, &value);
					if (!ret) {
						pos = 3;
						return 0;
					}
				}
		case 4:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_UPLOAD_TYPE, 1, &ctx.m_tSystemParam.dum);
				if (!ret) {
					pos = 4;
					return 0;
				}
		case 5: {
					uint16_t value = ctx.m_tSystemParam.dui;
					value = htons(value);
					ret = rkPushTlvToMsg(pkt, TLV_TAG_UPLOAD_INTERVAL, 2, &value);
					if (!ret) {
						pos = 5;
						return 0;
					}
				}
		case 6:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_RTD_UPLOAD, 1, &ctx.m_tSystemParam.rduen);
				if (!ret) {
					pos = 6;
					return 0;
				}
		case 7:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_TMD_UPLOAD, 1, &ctx.m_tSystemParam.mduen);
				if (!ret) {
					pos = 7;
					return 0;
				}
		case 8:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_HSD_UPLOAD, 1, &ctx.m_tSystemParam.hduen);
				if (!ret) {
					pos = 8;
					return 0;
				}
		case 9:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_DSD_UPLOAD, 1, &ctx.m_tSystemParam.dduen);
				if (!ret) {
					pos = 9;
					return 0;
				}
		case 10:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_SWITCH_SENSE, 1, &ctx.m_tSystemParam.alarmen);
				if (!ret) {
					pos = 10;
					return 0;
				}
		case 11:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_ALARM_ENABLE, 1, &ctx.m_tSystemParam.alarmen);
				if (!ret) {
					pos = 11;
					return 0;
				}
		default:
				pos = 0;
	}

	return 1;
}

int rkPushComConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	int ret;
	static int id = 0, pos = 0;

	for (; id < COM_NUM; id++) {
		char string[256];
		memset(string, 0, sizeof(string));
		sprintf(string, "baud=%d,databit=%d,stopbit=%d,parity=%d",
				ctx.m_tUartParam.m_tComParam[id].baud,
				ctx.m_tUartParam.m_tComParam[id].db,
				ctx.m_tUartParam.m_tComParam[id].sb,
				ctx.m_tUartParam.m_tComParam[id].parity);
		switch(pos) {
			case 0:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM1_PARAM + id * 2, strlen(string), string);
				if (!ret) {
					pos = 0;
					return 0;
				}
			case 1:
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM1_PROTOCOL + id * 2, strlen(ctx.m_tUartParam.m_tComParam[id].proto), ctx.m_tUartParam.m_tComParam[id].proto);
				if (!ret) {
					pos = 1;
					return 0;
				}
			default:
				pos = 0;
		}
	}
	id = 0;

	return 1;
}

int rkPushNetConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	int ret;
	static int pos = 0;
	switch(pos) {
		case 0:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_LINK_MODE, 1, &ctx.m_tNetParam.cm);
			if (!ret) {
				pos = 0;
				return 0;
			}
		case 1:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_DEVICE_IP_ADDR, strlen(ctx.m_tNetParam.laddr), ctx.m_tNetParam.laddr);
			if (!ret) {
				pos = 1;
				return 0;
			}
		case 2:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_DEVICE_NETMASK, strlen(ctx.m_tNetParam.mask), ctx.m_tNetParam.mask);
			if (!ret) {
				pos = 2;
				return 0;
			}
		case 3:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_REMOTE_IP_ADDR, strlen(ctx.m_tNetParam.raddr), ctx.m_tNetParam.raddr);
			if (!ret) {
				pos = 3;
				return 0;
			}
		case 4: {
					uint16_t	value = ctx.m_tNetParam.rport;
					value = htons(value);
					ret = rkPushTlvToMsg(pkt, TLV_TAG_REMOTE_PORT, 2, &value);
					if (!ret) {
						pos = 4;
						return 0;
					}
				}
		default:
				pos = 0;
	}

	return 1;
}

int rkPushAnalogConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	static int id = 0;
	int ret;

	for (; id < AI_NUM; id++) {
		char string[512];
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,type=%d,code=%s,ulv=%f,llv=%f,utv=%f,ltv=%f,convert=%d,formula=%s",
				ctx.m_tAnalogParam.m_tChannelParam[id].inuse,
				ctx.m_tAnalogParam.m_tChannelParam[id].type,
				ctx.m_tAnalogParam.m_tChannelParam[id].code,
				ctx.m_tAnalogParam.m_tChannelParam[id].ulv,
				ctx.m_tAnalogParam.m_tChannelParam[id].llv,
				ctx.m_tAnalogParam.m_tChannelParam[id].utv,
				ctx.m_tAnalogParam.m_tChannelParam[id].ltv,
				ctx.m_tAnalogParam.m_tChannelParam[id].isconv,
				ctx.m_tAnalogParam.m_tChannelParam[id].fml);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_ANALOG_CH1_PARAM + id, strlen(string), string);
		if (!ret) {
			return 0;
		}
	}
	id = 0;

	return 1;
}

int rkPushSerialConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	static int id = 0;
	int ret;

	for (; id < EI_NUM; id++) {
		char string[512];
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,code=%s,com=%d,type=%d,devaddr=%d,regaddr=%d,ulv=%f,llv=%f,utv=%f,ltv=%f,convert=%d,formula=%s",
				ctx.m_tUartParam.m_tChannelParam[id].inuse,
				ctx.m_tUartParam.m_tChannelParam[id].code,
				ctx.m_tUartParam.m_tChannelParam[id].com,
				ctx.m_tUartParam.m_tChannelParam[id].datatype,
				ctx.m_tUartParam.m_tChannelParam[id].devid,
				ctx.m_tUartParam.m_tChannelParam[id].regaddr + 1,
				ctx.m_tUartParam.m_tChannelParam[id].ulv,
				ctx.m_tUartParam.m_tChannelParam[id].llv,
				ctx.m_tUartParam.m_tChannelParam[id].utv,
				ctx.m_tUartParam.m_tChannelParam[id].ltv,
				ctx.m_tUartParam.m_tChannelParam[id].isconv,
				ctx.m_tUartParam.m_tChannelParam[id].fml);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_SERIAL_CH1_PARAM + id, strlen(string), string);
		if (!ret) {
			return 0;
		}
	}
	id = 0;

	return 1;
}

int rkPushDioConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	static int pos = 0, id = 0;
	int ret;

	switch(pos) {
		case 0:
			for (; id < DI_NUM; id++) {
				char string[128];
				memset(string, 0, sizeof(string));
				sprintf(string, "inuse=%d,code=%s",
						ctx.m_tDioParam.m_tDiParam[id].inuse,
						ctx.m_tDioParam.m_tDiParam[id].code);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_DI_CH1_PARAM + id, strlen(string), string);
				if (!ret) {
					return 0;
				}
			}
			pos = 1;
			id = 0;
		case 1:
			for (; id < DO_NUM; id++) {
				char string[128];
				memset(string, 0, sizeof(string));
				sprintf(string, "inuse=%d,code=%s,status=%d",
						ctx.m_tDioParam.m_tDoParam[id].inuse,
						ctx.m_tDioParam.m_tDoParam[id].code,
						ctx.m_tDioParam.m_tDoParam[id].val);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_DO_CH1_PARAM + id, strlen(string), string);
				if (!ret) {
					return 0;
				}
			}
		default:
			pos = 0;
			id = 0;
	}

	return 1;
}

int rkPushDtuConfigTlvsToMsg(rkMsgPkt_t *pkt)
{
	return 1;
}

int rkVerifyMsgTail(rkMsgPkt_t *pkt)
{
	uint16_t calc_crc, msg_crc;

	msg_crc = pkt->msg[pkt->len - 2] << 8 | pkt->msg[pkt->len - 1];
	calc_crc = rkCrc16(pkt->msg, pkt->len - 2);

	//qDebug("MSG_CRC:%04X,CALC_CRC:%04X", msg_crc, calc_crc);
	if (msg_crc == calc_crc) {
		return 1;
	}

	return 0;
}

uint8_t rkCheckLoginPermit(rkMsgPkt_t *pkt)
{
	uint16_t tag;
	uint16_t len;
	int pos = 0;
	char user[32], passwd[32];

	memset(user, 0, sizeof(user));
	memset(passwd, 0, sizeof(passwd));

	while(pos < pkt->len - RK_PROTOCOL_TAIL_LEN) {
		tag = pkt->msg[pos] << 8 | pkt->msg[pos + 1];
		pos += 2;
		//printf("pos = %d, tag = %04X\n", pos, tag);
		len = pkt->msg[pos] << 8 | pkt->msg[pos + 1];
		pos += 2;
		//printf("pos = %d, len = %04X\n", pos, len);
		if (tag == TLV_TAG_LOGIN_USER) {
			memcpy(user, &pkt->msg[pos], len);
		} else if (tag == TLV_TAG_LOGIN_PASSWORD) {
			memcpy(passwd, &pkt->msg[pos], len);
		}
		pos += len;
	}

	if (!strcmp(user, "admin") && !strcmp(passwd, "123456")) {
		return 1;
	} else {
		return 0;
	}
}

int rkParseParamConfig(rkMsgPkt_t *pkt)
{
	int pos = 0;
	uint16_t tag, len;
	uint8_t *value = NULL;

	while(pos < pkt->len - RK_PROTOCOL_TAIL_LEN) {
		tag = pkt->msg[pos] << 8 | pkt->msg[pos + 1];
		pos += 2;
		len = pkt->msg[pos] << 8 | pkt->msg[pos + 1];
		pos += 2;
		value = pkt->msg + pos;
		pos += len;
		//printf("\nTAG : %04X, LEN : %04X\n", tag, len);

		if (tag >= TLV_TAG_COM1_PARAM && tag <= TLV_TAG_COM4_PROTOCOL) {
			int index = tag - TLV_TAG_COM1_PARAM;
			char buf[64], *ptr = NULL;

			if (index % 2 == 0) {
				index = index / 2;
				memset(buf, 0, sizeof(buf));
				memcpy(buf, value, len);
#ifdef DEBUG_MESSAGE_PARSE_OUTPUT
				printf("COM[%d] : %s\n", index, buf);
#endif
				ptr = strstr(buf, "baud=");
				if (ptr)
					ctx.m_tUartParam.m_tComParam[index].baud = atoi(ptr + 5);
				ptr = strstr(buf, "databit=");
				if (ptr)
					ctx.m_tUartParam.m_tComParam[index].db = atoi(ptr + 8);
				ptr = strstr(buf, "stopbit=");
				if (ptr)
					ctx.m_tUartParam.m_tComParam[index].sb = atoi(ptr + 8);
				ptr = strstr(buf, "parity=");
				if (ptr)
					ctx.m_tUartParam.m_tComParam[index].parity = atoi(ptr + 7);
				rkXmlSaveComParam(index, &ctx.m_tUartParam.m_tComParam[index]);
			} else {
				index = index / 2;
				memset(ctx.m_tUartParam.m_tComParam[index].proto, 0, 32);
				memcpy(ctx.m_tUartParam.m_tComParam[index].proto, value, len);
				rkXmlSaveComParam(index, &ctx.m_tUartParam.m_tComParam[index]);
			}
		}

		if(tag >= TLV_TAG_ANALOG_CH1_PARAM && tag <= TLV_TAG_ANALOG_CH16_PARAM) {
			int index = tag - TLV_TAG_ANALOG_CH1_PARAM;
			char buf[512];
			memset(buf, 0, len + 1);
			memcpy(buf, value, len);
#ifdef DEBUG_MESSAGE_PARSE_OUTPUT
			printf("Analog[%d] : %s\n", index + 1, buf);
#endif
			char *ptr = strstr(buf, "inuse=");
			if (ptr)
				ctx.m_tAnalogParam.m_tChannelParam[index].inuse =  atoi(ptr + 6) ? 1 : 0;

			if ((ptr = strstr(buf, "type="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].type = atoi(ptr + 5) ? 1 : 0;
			}

			if ((ptr = strstr(buf, "code="))) {
				ptr += 5;
				char *tmp_ptr = strchr(ptr, ',');
				memset(ctx.m_tAnalogParam.m_tChannelParam[index].code, 0, 8);
				if (tmp_ptr) {
					memcpy(ctx.m_tAnalogParam.m_tChannelParam[index].code, ptr, tmp_ptr - ptr);
				} else {
					strcpy(ctx.m_tAnalogParam.m_tChannelParam[index].code, ptr);
				}
			}

			if ((ptr = strstr(buf, "ulv="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].ulv = atof(ptr + 4);
			} 

			if ((ptr = strstr(buf, "llv="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].llv= atof(ptr + 4);
			}

			if ((ptr = strstr(buf, "utv="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].utv = atof(ptr + 4);
			} 

			if ((ptr = strstr(buf, "ltv="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].ltv = atof(ptr + 4);
			}

			if ((ptr = strstr(buf, "convert="))) {
				ctx.m_tAnalogParam.m_tChannelParam[index].isconv = atoi(ptr + 8);
			} 

			if ((ptr = strstr(buf, "formula="))) {
				ptr += 8;
				char *tmp_ptr = strchr(ptr, ',');
				memset(ctx.m_tAnalogParam.m_tChannelParam[index].fml, 0, sizeof(ctx.m_tAnalogParam.m_tChannelParam[index].fml));
				if (tmp_ptr) {
					memcpy(ctx.m_tAnalogParam.m_tChannelParam[index].fml, ptr, tmp_ptr - ptr);
				} else {
					strcpy(ctx.m_tAnalogParam.m_tChannelParam[index].fml, ptr);
				}
			}

			rkXmlSaveAnalogParam(index, &ctx.m_tAnalogParam.m_tChannelParam[index]);
		}

		if(tag >= TLV_TAG_SERIAL_CH1_PARAM && tag <= TLV_TAG_SERIAL_CH32_PARAM) {
			int index = tag - TLV_TAG_SERIAL_CH1_PARAM;
			char buf[512];
			memset(buf, 0, len + 1);
			memcpy(buf, value, len);
#ifdef DEBUG_MESSAGE_PARSE_OUTPUT
			printf("Serial[%d] : %s\n", index + 1, buf);
#endif
			char *ptr = strstr(buf, "inuse=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].inuse =  atoi(ptr + 6);
			ptr = strstr(buf, "type=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].datatype = atoi(ptr + 5);
			ptr = strstr(buf, "code=");
			if (ptr) {
				ptr += 5;
				char *tmp_ptr = strchr(ptr, ',');
				memset(ctx.m_tUartParam.m_tChannelParam[index].code, 0, 8);
				if (tmp_ptr) {
					memcpy(ctx.m_tUartParam.m_tChannelParam[index].code, ptr, tmp_ptr - ptr);
				} else {
					strcpy(ctx.m_tUartParam.m_tChannelParam[index].code, ptr);
				}
			}
			ptr = strstr(buf, "com=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].com = atoi(ptr + 4);
			ptr = strstr(buf, "devaddr=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].devid = atoi(ptr + 8);
			ptr = strstr(buf, "regaddr=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].regaddr = atoi(ptr + 8);
			ptr = strstr(buf, "ulv=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].ulv = atof(ptr + 4);
			ptr = strstr(buf, "llv=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].llv= atof(ptr + 4);
			ptr = strstr(buf, "utv=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].utv = atof(ptr + 4);
			ptr = strstr(buf, "ltv=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].ltv = atof(ptr + 4);
			ptr = strstr(buf, "convert=");
			if (ptr)
				ctx.m_tUartParam.m_tChannelParam[index].isconv = atoi(ptr + 8);
			ptr = strstr(buf, "formula=");
			if (ptr) {
				ptr += 8;
				char *tmp_ptr = strchr(ptr, ',');
				memset(ctx.m_tUartParam.m_tChannelParam[index].fml, 0, sizeof(ctx.m_tUartParam.m_tChannelParam[index].fml));
				if (tmp_ptr) {
					memcpy(ctx.m_tUartParam.m_tChannelParam[index].fml, ptr, tmp_ptr - ptr);
				} else {
					strcpy(ctx.m_tUartParam.m_tChannelParam[index].fml, ptr);
				}
			}
			rkXmlSaveUartParam(index, &ctx.m_tUartParam.m_tChannelParam[index]);
		}

		if(tag >= TLV_TAG_DI_CH1_PARAM && tag <= TLV_TAG_DI_CH8_PARAM) {
			int index = tag - TLV_TAG_DI_CH1_PARAM;
			char buf[64], *ptr = NULL;
			memset(buf, 0, sizeof(buf));
			memcpy(buf, value, len);
#ifdef DEBUG_MESSAGE_PARSE_OUTPUT
			printf("DI[%d] : %s\n", index + 1, buf);
#endif
			ptr = strstr(buf, "inuse=");
			if (ptr) {
				ctx.m_tDioParam.m_tDiParam[index].inuse = atoi(buf + 6);
			}

			ptr = strstr(buf, "code=");
			if (ptr) {
				ptr += 5;
				char *tmp = strchr(ptr, ',');
				memset(ctx.m_tDioParam.m_tDiParam[index].code, 0, 8);
				if (tmp) {
					memcpy(ctx.m_tDioParam.m_tDiParam[index].code, ptr, tmp - ptr);
				} else {
					strcpy(ctx.m_tDioParam.m_tDiParam[index].code, ptr);
				}
			}
			rkXmlSaveDioParam(&ctx.m_tDioParam);
		}

		if (tag >= TLV_TAG_DO_CH1_PARAM && tag <= TLV_TAG_DO_CH8_PARAM) {
			int index = tag - TLV_TAG_DO_CH1_PARAM;
			char buf[64], *ptr = NULL;
			memset(buf, 0, sizeof(buf));
			memcpy(buf, value, len);
#ifdef DEBUG_MESSAGE_PARSE_OUTPUT
			printf("DO[%d] : %s\n", index + 1, buf);
#endif
			ptr = strstr(buf, "inuse=");
			if (ptr) {
				ctx.m_tDioParam.m_tDoParam[index].inuse = atoi(buf + 6);
			}

			ptr = strstr(buf, "code=");
			if (ptr) {
				ptr += 5;
				char *tmp = strchr(ptr, ',');
				memset(ctx.m_tDioParam.m_tDoParam[index].code, 0, 8);
				if (tmp) {
					memcpy(ctx.m_tDioParam.m_tDoParam[index].code, ptr, tmp - ptr);
				} else {
					strcpy(ctx.m_tDioParam.m_tDoParam[index].code, ptr);
				}
			}

			ptr = strstr(buf, "status=");
			if (ptr) {
				ctx.m_tDioParam.m_tDoParam[index].val = atoi(ptr + 7);
			}
			rkXmlSaveDioParam(&ctx.m_tDioParam);
		}

		switch(tag) {
			case TLV_TAG_SIM_ID:
				memset(ctx.m_tSystemParam.sim, 0, 12);
				memcpy(ctx.m_tSystemParam.sim, value, len);
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_SIM_NUM, ctx.m_tSystemParam.sim);
				break;
			case TLV_TAG_MN_ID:
				memset(ctx.m_tSystemParam.mn, 0, 16);
				memcpy(ctx.m_tSystemParam.mn, value, len);
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_MN_NUM, ctx.m_tSystemParam.mn);
				break;
			case TLV_TAG_SYSTEM_TYPE:
				ctx.m_tSystemParam.st = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_SYSTEM_TYPE, &ctx.m_tSystemParam.st);
				break;
			case TLV_TAG_STORAGE_INTERVAL:
				ctx.m_tSystemParam.dsi = value[0] << 8 | value[1];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_DSI, &ctx.m_tSystemParam.dsi);
				break;
			case TLV_TAG_UPLOAD_TYPE:
				ctx.m_tSystemParam.dum = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_DUM, &ctx.m_tSystemParam.dum);
				break;
			case TLV_TAG_UPLOAD_INTERVAL:
				ctx.m_tSystemParam.dui = value[0] << 8 | value[1];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_DUI, &ctx.m_tSystemParam.dui);
				break;
			case TLV_TAG_RTD_UPLOAD:
				ctx.m_tSystemParam.rduen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_RTD_UPLOAD, &ctx.m_tSystemParam.rduen);
				break;
			case TLV_TAG_TMD_UPLOAD:
				ctx.m_tSystemParam.mduen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD, &ctx.m_tSystemParam.mduen);
				break;
			case TLV_TAG_HSD_UPLOAD:
				ctx.m_tSystemParam.hduen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD, &ctx.m_tSystemParam.hduen);
				break;
			case TLV_TAG_DSD_UPLOAD:
				ctx.m_tSystemParam.dduen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD, &ctx.m_tSystemParam.dduen);
				break;
			case TLV_TAG_SWITCH_SENSE:
				ctx.m_tSystemParam.alarmen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD, &ctx.m_tSystemParam.sduen);
				break;
			case TLV_TAG_ALARM_ENABLE:
				ctx.m_tSystemParam.alarmen = value[0];
				rkXmlSaveSysParam(XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD, &ctx.m_tSystemParam.alarmen);
				break;
			case TLV_TAG_LINK_MODE:
				ctx.m_tNetParam.cm = value[0];
				rkXmlSaveNetParam(&ctx.m_tNetParam);
				break;
			case TLV_TAG_DEVICE_IP_ADDR:
				memcpy(ctx.m_tNetParam.laddr, value, len);
				ctx.m_tNetParam.laddr[len] = 0;
				rkXmlSaveNetParam(&ctx.m_tNetParam);
				break;
			case TLV_TAG_DEVICE_NETMASK:
				memcpy(ctx.m_tNetParam.mask, value, len);
				ctx.m_tNetParam.mask[len] = 0;
				rkXmlSaveNetParam(&ctx.m_tNetParam);
				break;
			case TLV_TAG_REMOTE_IP_ADDR:
				memcpy(ctx.m_tNetParam.raddr, value, len);
				ctx.m_tNetParam.raddr[len] = 0;
				rkXmlSaveNetParam(&ctx.m_tNetParam);
				break;
			case TLV_TAG_REMOTE_PORT:
				ctx.m_tNetParam.rport = value[0] << 8 | value[1];
				rkXmlSaveNetParam(&ctx.m_tNetParam);
				break;
		}
	}

	return 1;
}

int rkPushConfigTlvToMSgByTag(rkMsgPkt_t *pkt, uint16_t tag)
{
	int ret, index;
	char string[512];

	if (tag >= TLV_TAG_ANALOG_CH1_PARAM && tag <= TLV_TAG_ANALOG_CH16_PARAM) {
		index = tag - TLV_TAG_ANALOG_CH1_PARAM;
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,type=%d,code=%s,ulv=%f,llv=%f,utv=%f,ltv=%f,convert=%d,formula=%s",
				ctx.m_tAnalogParam.m_tChannelParam[index].inuse,
				ctx.m_tAnalogParam.m_tChannelParam[index].type,
				ctx.m_tAnalogParam.m_tChannelParam[index].code,
				ctx.m_tAnalogParam.m_tChannelParam[index].ulv,
				ctx.m_tAnalogParam.m_tChannelParam[index].llv,
				ctx.m_tAnalogParam.m_tChannelParam[index].utv,
				ctx.m_tAnalogParam.m_tChannelParam[index].ltv,
				ctx.m_tAnalogParam.m_tChannelParam[index].isconv,
				ctx.m_tAnalogParam.m_tChannelParam[index].fml);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_ANALOG_CH1_PARAM + index, strlen(string), string);
		return ret;
	}

	if (tag >= TLV_TAG_SERIAL_CH1_PARAM && tag <= TLV_TAG_SERIAL_CH32_PARAM) {
		index = tag - TLV_TAG_SERIAL_CH1_PARAM;
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,code=%s,com=%d,type=%d,devaddr=%d,regaddr=%d,ulv=%f,llv=%f,utv=%f,ltv=%f,convert=%d,formula=%s",
				ctx.m_tUartParam.m_tChannelParam[index].inuse,
				ctx.m_tUartParam.m_tChannelParam[index].code,
				ctx.m_tUartParam.m_tChannelParam[index].com,
				ctx.m_tUartParam.m_tChannelParam[index].datatype,
				ctx.m_tUartParam.m_tChannelParam[index].devid,
				ctx.m_tUartParam.m_tChannelParam[index].regaddr,
				ctx.m_tUartParam.m_tChannelParam[index].ulv,
				ctx.m_tUartParam.m_tChannelParam[index].llv,
				ctx.m_tUartParam.m_tChannelParam[index].utv,
				ctx.m_tUartParam.m_tChannelParam[index].ltv,
				ctx.m_tUartParam.m_tChannelParam[index].isconv,
				ctx.m_tUartParam.m_tChannelParam[index].fml);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_SERIAL_CH1_PARAM + index, strlen(string), string);
		return ret;
	}

	if (tag >= TLV_TAG_DI_CH1_PARAM && tag <= TLV_TAG_DI_CH8_PARAM) {
		index = tag - TLV_TAG_DI_CH1_PARAM;
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,code=%s",
				ctx.m_tDioParam.m_tDiParam[index].inuse,
				ctx.m_tDioParam.m_tDiParam[index].code);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_DI_CH1_PARAM + index, strlen(string), string);
		return ret;
	}

	if (tag >= TLV_TAG_DO_CH1_PARAM && tag <= TLV_TAG_DO_CH8_PARAM) {
		index = tag - TLV_TAG_DO_CH1_PARAM;
		memset(string, 0, sizeof(string));
		sprintf(string, "inuse=%d,code=%s,status=%d",
				ctx.m_tDioParam.m_tDoParam[index].inuse,
				ctx.m_tDioParam.m_tDoParam[index].code,
				ctx.m_tDioParam.m_tDoParam[index].val);
		ret = rkPushTlvToMsg(pkt, TLV_TAG_DO_CH1_PARAM + index, strlen(string), string);
		return ret;
	}

	switch(tag) {
		case TLV_TAG_SIM_ID:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_SIM_ID, strlen(ctx.m_tSystemParam.sim), ctx.m_tSystemParam.sim);
			break;
		case TLV_TAG_MN_ID:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_MN_ID, strlen(ctx.m_tSystemParam.mn), ctx.m_tSystemParam.mn);
			break;
		case TLV_TAG_SYSTEM_TYPE:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_SYSTEM_TYPE, 1, &ctx.m_tSystemParam.st);
			break;
		case TLV_TAG_STORAGE_INTERVAL:
			{
				uint16_t	value = ctx.m_tSystemParam.dsi;
				value = htons(value);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_STORAGE_INTERVAL, 2, &value);
				break;
			}
		case TLV_TAG_UPLOAD_TYPE:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_UPLOAD_TYPE, 1, &ctx.m_tSystemParam.dum);
			break;
		case TLV_TAG_UPLOAD_INTERVAL: 
			{
				uint16_t value = ctx.m_tSystemParam.dui;
				value = htons(value);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_UPLOAD_INTERVAL, 2, &value);
				break;
			}
		case TLV_TAG_RTD_UPLOAD:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_RTD_UPLOAD, 1, &ctx.m_tSystemParam.rduen);
			break;
		case TLV_TAG_TMD_UPLOAD:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_TMD_UPLOAD, 1, &ctx.m_tSystemParam.mduen);
			break;
		case TLV_TAG_HSD_UPLOAD:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_HSD_UPLOAD, 1, &ctx.m_tSystemParam.hduen);
			break;
		case TLV_TAG_DSD_UPLOAD:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_DSD_UPLOAD, 1, &ctx.m_tSystemParam.dduen);
			break;
		case TLV_TAG_SWITCH_SENSE:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_SWITCH_SENSE, 1, &ctx.m_tSystemParam.alarmen);
			break;
		case TLV_TAG_ALARM_ENABLE:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_ALARM_ENABLE, 1, &ctx.m_tSystemParam.alarmen);
			break;
		case TLV_TAG_COM1_PARAM:
			{ 
				char string[256];
				memset(string, 0, sizeof(string));
				sprintf(string, "baud=%d,databit=%d,stopbit=%d,parity=%d",
						ctx.m_tUartParam.m_tComParam[0].baud,
						ctx.m_tUartParam.m_tComParam[0].db,
						ctx.m_tUartParam.m_tComParam[0].sb,
						ctx.m_tUartParam.m_tComParam[0].parity);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM1_PARAM, strlen(string), string);
				break;
			}
		case TLV_TAG_COM1_PROTOCOL:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_COM1_PROTOCOL, strlen(ctx.m_tUartParam.m_tComParam[0].proto), ctx.m_tUartParam.m_tComParam[0].proto);
			break;
		case TLV_TAG_COM2_PARAM:
			{ 
				char string[256];
				memset(string, 0, sizeof(string));
				sprintf(string, "baud=%d,databit=%d,stopbit=%d,parity=%d",
						ctx.m_tUartParam.m_tComParam[1].baud,
						ctx.m_tUartParam.m_tComParam[1].db,
						ctx.m_tUartParam.m_tComParam[1].sb,
						ctx.m_tUartParam.m_tComParam[1].parity);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM2_PARAM, strlen(string), string);
				break;
			}
		case TLV_TAG_COM2_PROTOCOL:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_COM2_PROTOCOL, strlen(ctx.m_tUartParam.m_tComParam[1].proto), ctx.m_tUartParam.m_tComParam[1].proto);
			break;
		case TLV_TAG_COM3_PARAM:
			{ 
				char string[256];
				memset(string, 0, sizeof(string));
				sprintf(string, "baud=%d,databit=%d,stopbit=%d,parity=%d",
						ctx.m_tUartParam.m_tComParam[2].baud,
						ctx.m_tUartParam.m_tComParam[2].db,
						ctx.m_tUartParam.m_tComParam[2].sb,
						ctx.m_tUartParam.m_tComParam[2].parity);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM3_PARAM, strlen(string), string);
				break;
			}
		case TLV_TAG_COM3_PROTOCOL:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_COM3_PROTOCOL, strlen(ctx.m_tUartParam.m_tComParam[2].proto), ctx.m_tUartParam.m_tComParam[2].proto);
			break;
		case TLV_TAG_COM4_PARAM:
			{ 
				char string[256];
				memset(string, 0, sizeof(string));
				sprintf(string, "baud=%d,databit=%d,stopbit=%d,parity=%d",
						ctx.m_tUartParam.m_tComParam[3].baud,
						ctx.m_tUartParam.m_tComParam[3].db,
						ctx.m_tUartParam.m_tComParam[3].sb,
						ctx.m_tUartParam.m_tComParam[3].parity);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_COM4_PARAM, strlen(string), string);
				break;
			}
		case TLV_TAG_COM4_PROTOCOL:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_COM4_PROTOCOL, strlen(ctx.m_tUartParam.m_tComParam[3].proto), ctx.m_tUartParam.m_tComParam[3].proto);
			break;
		case TLV_TAG_LINK_MODE:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_LINK_MODE, 1, &ctx.m_tNetParam.cm);
			break;
		case TLV_TAG_DEVICE_IP_ADDR:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_DEVICE_IP_ADDR, strlen(ctx.m_tNetParam.laddr), ctx.m_tNetParam.laddr);
			break;
		case TLV_TAG_DEVICE_NETMASK:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_DEVICE_NETMASK, strlen(ctx.m_tNetParam.mask), ctx.m_tNetParam.mask);
			break;
		case TLV_TAG_REMOTE_IP_ADDR:
			ret = rkPushTlvToMsg(pkt, TLV_TAG_REMOTE_IP_ADDR, strlen(ctx.m_tNetParam.raddr), ctx.m_tNetParam.raddr);
			break;
		case TLV_TAG_REMOTE_PORT:
			{
				uint16_t	value = ctx.m_tNetParam.rport;
				value = htons(value);
				ret = rkPushTlvToMsg(pkt, TLV_TAG_REMOTE_PORT, 2, &value);
				break;
			}
		default:
			break;
	}

	return ret;
}
