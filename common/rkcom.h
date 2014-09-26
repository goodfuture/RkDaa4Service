#ifndef _RK_COMMON_H_
#define _RK_COMMON_H_

#define DTU_CFG_PROG_PATH		"./Inhand.bin"
#define CORE_PROG_PATH			"./Core.bin"
#define GUI_PROG_PATH			"./GUI.bin"

#define DEF_CFG_FILE_DIR		"/etc/userconfig" 
#define DEF_RTD_DB_PATH			"/media/rkrtd.db"
#define DEF_MSG_DB_PATH			"/media/rkmsg.db"
#define DEF_DYN_LIB_DIR			"/usr/lib/extend"

#define SYS_CFG_FILE			"system.xml"
#define NET_CFG_FILE			"network.xml"
#define SER_CFG_FILE			 "uart.xml"
#define DIO_CFG_FILE			"dio.xml"
#define AI_CFG_FILE				"analog.xml"
#define AI_CALIB_FILE			"aicalib.xml"

#define DTU_DEV_INTERFACE		"/dev/ttyS1"
#define DIO_DEV_INTERFACE		"/dev/dio"
#define AI_DEV_NAME_PREFIX		"/dev/adc"
#define COM_DEV_NAME_PREFIX		"/dev/com"

#define COM_NUM		4
#define EI_NUM		32
#define AI_NUM		16
#define DI_NUM		8
#define DO_NUM		8

/* ALARM TYPE */
typedef enum {
	ALARM_N = 0, /* Normal */
	ALARM_H, /* High */
	ALARM_L, /* Low */
	ALARM_E,
} ALARM_T;

#define UI_CMD_RELOAD_USR_CFG	0
#define UI_CMD_DISABLE_DTU		1
#define UI_CMD_ENABLE_DTU		2
#define UI_CMD_CORE_PROG_EXIT	3
#define UI_CMD_QRY_DATABASE		4

#endif /* _RK_COMMON_H_ */
