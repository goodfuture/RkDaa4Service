#ifndef _RK_XML_PARSE_H_
#define _RK_XML_PARSE_H_

#include "rktype.h"
#include "mxml.h"

#define XML_SYSTEM_PARAM_NAME_SIM_NUM				"SIM"
#define XML_SYSTEM_PARAM_NAME_MN_NUM				"MN"
#define XML_SYSTEM_PARAM_NAME_SYSTEM_TYPE			"ST"
#define XML_SYSTEM_PARAM_NAME_PASSWORD				"PW"
#define XML_SYSTEM_PARAM_NAME_DSI					"DSI"
#define XML_SYSTEM_PARAM_NAME_DUM					"DUM"
#define XML_SYSTEM_PARAM_NAME_DUI					"DUI"
#define XML_SYSTEM_PARAM_NAME_RTD_UPLOAD			"RDUEN"
#define XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD	"MDUEN"
#define XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD		"HDUEN"
#define XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD		"DDUEN"
#define XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD	"SDUEN"
#define XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD			"ALARMEN"

typedef mxml_node_t XmlNode;
typedef mxml_type_t XmlType;

void rkXmlSetWorkDir(const char *);

XmlNode *rkXmlNewXml();

XmlNode *rkXmlLoadFile(const char *fileName);

int rkXmlGetNodeDepth(XmlNode *node);

XmlType rkXmlLoadCallBack(XmlNode *node);

int rkXmlSaveFile(const char *fileName, XmlNode *declareNode);

void rkXmlFree(XmlNode *node);

int rkXmlSetParamVal(XmlNode *parent, const char *paramName, const char *paramVal);

int rkXmlSetGroupParamVal(XmlNode *parent, const char *groupName, const char *paramName, const char *paramVal);

int rkXmlParseSysParam(struct sys *sys);
int rkXmlSaveAllSysParam(struct sys *sys);
int rkXmlSaveSysParam(const char *name, void *value);

int rkXmlParseNetParam(struct net *net);
int rkXmlSaveNetParam(struct net *net);

int rkXmlParseUartParam(struct uart *uart);
int rkXmlSaveAllUartParam(struct uart *uart);
int rkXmlSaveUartParam(int channel, struct edev *param);

int rkXmlSaveComParam(int channel, struct com *param);

int rkXmlParseDioParam(struct dio *dio);
int rkXmlSaveDioParam(struct dio *dio);

int rkXmlParseAnalogParam(struct analog *analog);
int rkXmlSaveAnalogParam(int channel, struct aiparam *param);
int rkXmlSaveAllAnalogParam(struct analog *analog);

int rkXmlParseAiCalParam(struct analog *analog);

int rkXmlGenAiCalibFile(struct aical *cal, uint8_t num);

#endif /* _RK_XML_PARSE_H_ */
