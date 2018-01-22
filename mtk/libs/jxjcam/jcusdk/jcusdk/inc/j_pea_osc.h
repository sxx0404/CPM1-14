#ifndef __J_PEA_OSC_H__
#define __J_PEA_OSC_H__

#include "j_sdk.h"

#define J_SDK_MAX_PEA_OSC_RULE_NUM  	8 // ֧����������
#define J_SDK_MAX_POLYGON_POINT     	8 // ֧�ֶ���α߽�����
#define J_SDK_MAX_TARGET_TRACK_POINT    40 // Ŀ�����켣�����
#define J_SDK_MAX_TARGET_NUM			64 // Ŀ�����
#define J_SDK_MAX_EVENT_NUM				128 // �¼�����
#define J_SDK_MAX_PEA_OSC_NAME_LEN  	32  // �������Ƴ���
#define J_SDK_IVS_FRAME_MAGIC 			0x49565330 //'IVS0'

#define J_SDK_MAX_CPC_RULE_NUM          1
#define J_SDK_MAX_CPC_RULE_POINT        4
#define J_SDK_MAX_CPC_TARGET_POINT      1
#define J_SDK_MAX_CPC_TARGET_NUM        15
#define J_SDK_MAX_CPC_EVENT_NUM         1
#define J_SDK_MAX_CPC_NAME_LEN  	    32  // �������Ƴ���


#ifndef _WIN32
# define DEF_PRAGMA_PACKED					__attribute__((packed))
#else
# define DEF_PRAGMA_PACKED					
#endif

#pragma pack(push, 1)

// ��������
typedef enum J_Scene_Type_E
{
	J_INDOOR_SCENE  = 0,		//  ���ڳ��� 
	J_OUTDOOR_SCENE	= 1 		// ���ⳡ�� 
} JSceneTypeE;

//��Խ����
typedef enum _J_Cross_Direction_E_
{  
    J_BOTH_DIRECTION  = 0,      //˫��
    J_LEFT_GO_RIGHT   = 1,      //������, ���߶δ���ˮƽλ�ã���Ϊ���ϵ��·���
    J_RIGHT_GO_LEFT   = 2       //���ҵ���, ���߶δ���ˮƽλ�ã���Ϊ���µ��Ϸ���
}JCrossDirectionE;

// �������Ͷ���:
typedef enum _JPeaOscRuleType_E_
{
	J_PEA_OSC_RULE_BEG_TYPE     = 101,
    J_TRIPWIRE_RULE_TYPE        = 101,       //������-->	JTripWireRuleS
    J_MUTITRIPWIRE_RULE_TYPE    = 102,       //˫����,�ݲ�֧��
    J_PERI_METER_RULE_TYPE      = 103,       //������-->	JPerimeterRuleS
    J_LOITER_RULE_TYPE          = 104,       //�ǻ�,�ݲ�֧��
	J_LEFT_RULE_TYPE            = 105,       //��Ʒ�������-->JLeftRuleS
    J_TAKE_RULE_TYPE            = 106,        //��Ʒ��ʧ���--> JTakeRuleS
    J_PEA_OSC_RULE_BUTT_TYPE
}JPeaOscRuleTypeE;

// �¼����Ͷ���
typedef enum _JPeaOscEventType_E_
{
	J_TGT_NONE_EVENT 	  	  = 0x00000000, // δ�����¼�
	J_TGT_UNKNOWN_EVENT       = 0x00000001,	// δ֪�¼�
    J_TGT_TRIPWIRE_EVENT      = 0x00000002, //������
    J_TGT_MUTITRIPWIRE_EVENT  = 0x00000004, //˫����
	J_TGT_PERI_METER_EVENT	  = 0x00000008, //����������
	J_TGT_LOITER_EVENT        = 0x00000010,	// �ǻ� 
	J_TGT_LEFT_EVENT		  = 0x00000020,	//��Ʒ����
	J_TGT_TAKE_EVENT	      = 0x00000040, //��Ʒ��ʧ
}JPeaOscEventTypeE;

// ��
typedef struct _jLine
{
	JPoint start_point;
	JPoint end_point;
}JLine;

// ����--> JRectangle 

// �����
typedef struct _jPolygon
{
    uint32_t	point_num;                        //����ε���
    JPoint		points[J_SDK_MAX_POLYGON_POINT];  //����ε�����
}JPolygon;

//�����߲���  
typedef struct _JTripWireRuleS_
{
    JLine 	stLine;     //�߲���
    uint32_t u32CrossDir;  //JCrossDirectionE����                       
}JTripWireRuleS;

//�ܽ����
typedef struct _JPerimeterRuleS_
{
    JPolygon	stPolygon;          //���������
    uint32_t	u32Mode;            //�ܽ�ģʽ 0 ���� 1 ���� 2 �뿪
}JPerimeterRuleS;

// ��Ʒ����
typedef struct _JLeftRuleS_
{
    JPolygon	stPolygon;          //���������
}JLeftRuleS;

// ��Ʒ��ʧ
typedef struct _JTakeRuleS_
{
    JPolygon	stPolygon;          //���������
}JTakeRuleS;

// ����������
typedef union _jPeaOscRuleDefine_U_
{
	JTripWireRuleS stTripWire;
	JPerimeterRuleS stPerimeter;
	JLeftRuleS 		stLeft;
	JTakeRuleS 		stTake;
	uint8_t        u8Size[64]; // �̶���С
}jPeaOscRuleDefineU;

// �����������
typedef struct _jPeaOscRuleDefine
{
	uint8_t u8RuleType; // JPeaOscRuleTypeE
	uint8_t u8Res[3];
	jPeaOscRuleDefineU uRuleDefine;
}JPeaOscRuleDefine;

//Ŀ������������ṹ��( �ݲ�֧�֣�Ԥ��)
typedef struct _jPeaOscTgtFliter_
{
    uint8_t		u8Enable;                   //ʹ��Ŀ�����
	uint8_t 	u8Res[3];
    uint32_t   	u32FliterTgts;              // Ŀǰ��bit0: ��, bit1: ��
}JPeaOscTgtFliter;

//ʱ����˲���( Ŀǰ����Ʒ��������Ʒ��ʧ����֧����Сʱ�����ƣ�
//���ʱ�����ƾ���֧��, �������Ԥ��ʹ��)
typedef struct _jPeaOscTimeFliter_
{
    uint8_t           u8Enable;                   //ʹ�ܹ���
    uint8_t           u8MinTime;                  //��Сʱ��
    uint8_t           u8MaxTime;                  //���ʱ��
    uint8_t           u8Res;
}JPeaOscTimeFliter;

//�ߴ��С���Ʋ���
typedef struct _jPeaOscAreaFliter_
{
	uint32_t		u32MinSize;				//��С�ߴ�
	uint32_t		u32MaxSize;				//���ߴ�
}jPeaOscAreaFliter;

//�ߴ������Ʋ���
typedef struct _jPeaOscWHFliter_
{	
	uint16_t		u16MinWidth;				//��С���
	uint16_t		u16MinHeight;				//��С�߶�
	uint16_t		u16MaxWidth;				//�����
	uint16_t		u16MaxHeight;				//���߶�
}jPeaOscWHFliter;

//�ߴ�����������ṹ��( Ŀǰ����Ʒ��������Ʒ��ʧ����֧��u8FilterType = 0 ��
//���Ʋ���, �����������Ԥ��ʹ��)
typedef struct _jPeaOscSizeFliter_
{
    uint8_t           u8Enable;                   // ʹ�ܳߴ����
    uint8_t           u8FilterType;				// �������ͣ�0   ��������ߴ磬1   ���ƿ��
    uint8_t           u8Res[2];
    union
    {
        jPeaOscWHFliter stParaWH;  
        jPeaOscAreaFliter stParaArea;
    };
}JPeaOscSizeFliter;

// �������ƶ���
typedef struct _jPeaOscRuleLimit
{
	JPeaOscTgtFliter	stTgtFilter;
	JPeaOscTimeFliter	stTimeFilter;
	JPeaOscSizeFliter   stSizeFilter;
}JPeaOscRuleLimit;

// �����쳣������
typedef struct _jPeaOscRuleExpHdl
{
	uint8_t	report_alarm; 	// �ͻ���/ƽ̨�Ƿ��ϱ�J_SDK_PEA_OSC_ALARM �澯
	uint8_t u8Res[3];
	JJointAction joint_action;	// �¼�����
}JPeaOscRuleExpHdl;

// ������
typedef struct _jPeaOscRule
{
	uint8_t check_enable;	//ʹ�ܹ���
	uint8_t check_level;	//���伶��0 ~3 �޾��� �ͼ����� �м����� �߼�������Ԥ������
	uint8_t res[6];         // Ԥ��
	uint8_t rule_name[J_SDK_MAX_PEA_OSC_NAME_LEN]; //��������
	JPeaOscRuleDefine stRulePara;   // �����������
	JPeaOscRuleLimit  stRuleLimit;  // �������ƶ���
	JPeaOscRuleExpHdl stRuleExpHdl; // �쳣������
}JPeaOscRule;

// PEA/OSC ���������
typedef struct _jPeaOscCfg
{
    uint8_t	enable;     // pea,osc ʹ��
    uint8_t scene_type; // ��������JSceneTypeE ��Ԥ�����壬Ŀǰ������Ч��
	uint8_t	target_level; //Ŀ����������ȼ���0~2 ���еͣ�Ԥ�����壬Ŀǰ������Ч��
	uint8_t res[13];   // Ԥ��
	JSegment	sched_time[J_SDK_MAX_SEG_SZIE]; // ʱ�������
	uint16_t rule_max_width;  //������ο����
	uint16_t rule_max_height; //������ο��߶�
	JPeaOscRule rules[J_SDK_MAX_PEA_OSC_RULE_NUM]; // ������
}JPeaOscCfg;

// ----------------------   PEA / OSC ֡�������ݸ�ʽ����Begin  ------------------//

// ���ݰ�����Ϣ
typedef struct _jPeaOscDataInfo
{
	uint8_t info_mask;     // Ŀǰbit0: ��Ч֡��ʶ��Ϊ0 ��ʾ������еĹ���/Ŀ��		
	uint8_t rule_num;      // �����������(info_mask & 0x1 == 1ʱ��Ч)
	uint8_t event_num;     // �����¼�����(info_mask & 0x1 == 1ʱ��Ч)
	uint8_t target_num;    // ����Ŀ�����(info_mask & 0x1 == 1ʱ��Ч)
	uint16_t refer_width;	// �ο����
	uint16_t refer_height;	// �ο��߶�
	uint32_t time_stamp;	// ʱ���(info_mask & 0x1 == 1 ʱ��Ч)
}JPeaOscDataInfo;

// ������������������
typedef struct _jPeaOscResultRule
{
	uint8_t  u8RuleType; // JPeaOscRuleTypeE
	uint8_t  u8RuleIdx; // ʵ�ʹ����( 0 ~ J_SDK_MAX_PEA_OSC_RULE_NUM - 1)
	uint8_t  u8Res[2];
	jPeaOscRuleDefineU uRuleDefine;
}JPeaOscResultRule;

// �¼���Ϣ����
typedef struct _jPeaOscEvent
{
	uint32_t event_type;				//�¼�����  JPeaOscEventTypeE
	uint32_t event_id;					//�¼���ʶ 
	uint8_t event_rule_index; 			//�¼������Ĺ�����, ֵΪJPeaOscResultRule.u8RuleIdx, ������������ĵڼ�������
	uint8_t event_level; 				//�¼���������
	uint8_t res[2];
	uint32_t event_tgt_id;				// �¼�����Ŀ��ID
	JRectangle tgt_rect; 				// �¼�����Ŀ����������
}JPeaOscEvent;

// Ŀ����Ϣ����
typedef struct _jPeaOscTarget
{
	uint32_t tgt_size; 	// ��Ŀ����Ϣ����
    uint32_t tgt_id;  	// Ŀ��ID
    uint32_t tgt_event;	//Ŀ�괥�����¼����� JPeaOscEventTypeE
	uint8_t tgt_rule_index; //Ŀ�� �����Ĺ�����, ֵΪJPeaOscResultRule.u8RuleIdx, ������������ĵڼ�������,δ��������Ϊ0xff 
	uint8_t tgt_type;	// Ŀ�������ˣ����� 0XFF: ��������/δ֪����,1 ��2 ��
	uint8_t tgt_have_track; // �Ƿ��й켣��Ϣ
	uint8_t tgt_track_point_num; //�켣�����
	uint8_t res1[8];	// Ԥ��
	JRectangle tgt_rect; // Ŀ����������
	int16_t	tgt_move_speed;		// ���ʣ�coord /s��-1 δ֪
	int16_t tgt_move_direction; // ����0~359�ȣ�-1 δ֪
	JPoint  tgt_track_points[J_SDK_MAX_TARGET_TRACK_POINT]; // ʵ�ʸ����ɱ䣬��tgt_track_point_num ֵָ��
}JPeaOscTarget;

/******************************************************************************
 PeaOsc ֡���ݶ���(��С�ɱ�)
 JPeaOscResultRule *pRules = ( JPeaOscResultRule *)((char *)&JPeaOscFrameData + sizeof(JPeaOscDataInfo));
 JPeaOscEvent *pEvents = (JPeaOscEvent *)(pRules + rule_num);
 JPeaOscTargets *pTargets = (JPeaOscTargets *)(pEvents + event_num);
******************************************************************************/
typedef struct _jPeaOscFrameData
{
	JPeaOscDataInfo stDataInfo;
	JPeaOscResultRule stRuleDef[J_SDK_MAX_PEA_OSC_RULE_NUM]; // ʵ�ʸ����ɱ䣬��rule_num ָ��
	JPeaOscEvent stEvents[J_SDK_MAX_EVENT_NUM];		// ʵ�ʸ����ɱ䣬��event_num ָ��
	JPeaOscTarget stTargets[J_SDK_MAX_TARGET_NUM];    	// ʵ�ʸ����ɱ䣬��target_num ָ��
}JPeaOscFrameData;

typedef enum _JIvsFrameType_E_
{
	J_IVS_PEA_OSC_FRAME = 0x00000001, // PEA OSC �����������---> JPeaOscFrameData
	J_IVS_CPC_FRAME     = 0x00000002, // CPC����
	J_IVS_HERD_FRAME	= 0x00000004, // ��ҵ��������
    J_IVS_FRAME_BUTT_TYPE
}JIvsFrameTypeE;
// ----------------------   PEA / OSC ֡�������ݸ�ʽ����End  ------------------//

//------------------------CPC---------------------------------------------------------//

//CPC������
//cpc config
typedef struct _JCpcRule
{
    uint8_t enable;         /* enable  */
    uint8_t res[7];         /* (reserved) */
    JPoint  rule_point[J_SDK_MAX_CPC_RULE_POINT];/* CPC Rule Point */
}JCpcRule;

//cpc rule cfg
typedef struct _JCpcRuleCfg
{
    uint8_t   enable;
    uint8_t   sensitivity;       /* Algorithm Sensitivity */
    uint8_t   count_max;         /* Maximum targets couted in one frame */
    uint8_t   res[5];           /* reserved */
    uint16_t  size_min;         /* Minimum diamater of target heads */
    uint16_t  size_max;         /* Maximum diamater of target heads */
    JCpcRule  rules[J_SDK_MAX_CPC_RULE_NUM];       //
}JCpcRuleCfg;

//cpc config
typedef struct _JCpcCfg
{
    uint32_t  size;
    uint8_t   enable;
    uint8_t   res[3];
    JSegment  segment[J_SDK_MAX_SEG_SZIE]; //time segment
	uint16_t rule_max_width;  //������ο����
	uint16_t rule_max_height; //������ο��߶�
    JCpcRuleCfg rule_cfg;
}JCpcCfg;

//===============cpc �������==============
// ���ݰ�����Ϣ
typedef struct _JCpcDataInfo
{
	uint8_t info_mask;     // Ŀǰbit0: ��Ч֡��ʶ��Ϊ0 ��ʾ������еĹ���/Ŀ��		
	uint8_t rule_num;      // �����������(info_mask & 0x1 == 1ʱ��Ч)
	uint8_t event_num;     // reserved
	uint8_t target_num;    // ����Ŀ�����(info_mask & 0x1 == 1ʱ��Ч)
	uint16_t refer_width;	// �ο����
	uint16_t refer_height;	// �ο��߶�
	uint32_t time_stamp;	// ʱ���(info_mask & 0x1 == 1 ʱ��Ч)
}JCpcDataInfo;

//for cpc ivs frame
typedef struct _JCpcResultRule
{
    uint8_t   rule_type;    //reserved
    uint8_t   rule_index;   //reserved
    uint8_t   res[2];
    JCpcRule  rule;
}JCpcResultRule;

//for reserved
typedef struct _JCpcEvent
{
    uint8_t res[32];
}JCpcEvent;

//cross line type
typedef enum _JCpcCrossTypeE
{
    J_CPC_ENTER_CROSS  = 0,     // ����
    J_CPC_LEAVE_CROSS,          // �뿪
}JCpcCrossTypeE;

//cpc target info
typedef struct _JCpcTarget
{
    uint32_t  size;          //
    uint32_t  target_size;   // Ŀ�꣨�ˣ�ͷ�����ο򣩳ߴ�, ������С���֮��
	uint8_t   cross_type;    // 0-enter  1-leave
	uint8_t   point_num;     // 
	uint8_t   res[7];       // res
	JPoint    target_point[J_SDK_MAX_CPC_TARGET_POINT];  // target's center point
}JCpcTarget;

//cpc frame
typedef struct _jCpcFrameData
{
    JCpcDataInfo    stDataInfo;
    JCpcResultRule  stRules[J_SDK_MAX_CPC_RULE_NUM];
    JCpcEvent       stEvents[J_SDK_MAX_CPC_EVENT_NUM];
    JCpcTarget      stTargets[J_SDK_MAX_CPC_TARGET_NUM];
}JCpcFrameData;
//=============cpc ������� end==============
//------------------------CPC End-----------------------------------------------------//
// ----------------------   ��ҵ���� ֡�������ݸ�ʽ����Begin  ------------------//

/******************************************************************************
 ��ҵ���� ֡���ݰ���
 J_IVS_FRAME_DATA + JHerdTroughInfo + ���HI_TROUGH_HEAD_S
 
******************************************************************************/

// ��ҵ������Ϣ�ṹ����
typedef struct _jHerdTroughInfo
{
	uint8_t data_mode; 		// 0: 1λ����1��С����������Ϣ�� 1: 1���ֽڴ���1��С����������Ϣ;ĿǰĬ��Ϊ0 
	uint8_t area_mode; 		// 0: ������ϱ��������ڲ���С���������Ϣ�����ݴ��ϵ��´����ҷֲ���
							// 1:  �����ϱ���������С��Ӿ��������С���������Ϣ�����ݴ��ϵ��´����ҷֲ���ĿǰĬ��Ϊ0 
	uint8_t rep_size;		// �ϱ��Ƿ��в�������λ��С1: 1x1 ����2: 2X2����... ��С����nCSWidth �� nCSHeight
	uint8_t had_flag;  		// �Ƿ�����Ϸֲ�������Ϣ
	uint16_t refer_width;	// �ο����
	uint16_t refer_height;	// �ο��߶�
}JHerdTroughInfo;

//��ҵ����ÿ���۵�����
typedef struct _HI_TROUGH_HEAD_S_
{
	uint8_t		 trough_id;
	uint8_t		 area_flag; //0�ڲۣ�1���	
	uint8_t		 percent;
	uint8_t		 res;
	JPoint 		 point[4];
	uint32_t	 data_len;
	void 		*pData[0];
}HI_TROUGH_HEAD_S;


// ----------------------   ��ҵ���� ֡�������ݸ�ʽ����End  ------------------//

// ���ܷ���֡�ṹ����
typedef struct _IVS_FRAME_DATA_
{
	uint32_t u32Magic; // J_SDK_IVS_FRAME_MAGIC �����ֽ���
	uint32_t u32Type;  // JIvsFrameTypeE
	uint32_t u32Size;	// Data Size
	void *pData[0];
}J_IVS_FRAME_DATA;

#pragma pack(pop)

#endif //__J_PEA_OSC_H__

