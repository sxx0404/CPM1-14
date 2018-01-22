#ifndef __J_PEA_OSC_H__
#define __J_PEA_OSC_H__

#include "j_sdk.h"

#define J_SDK_MAX_PEA_OSC_RULE_NUM  	8 // 支持最大规则数
#define J_SDK_MAX_POLYGON_POINT     	8 // 支持多边形边界点个数
#define J_SDK_MAX_TARGET_TRACK_POINT    40 // 目标最大轨迹点个数
#define J_SDK_MAX_TARGET_NUM			64 // 目标个数
#define J_SDK_MAX_EVENT_NUM				128 // 事件个数
#define J_SDK_MAX_PEA_OSC_NAME_LEN  	32  // 规则名称长度
#define J_SDK_IVS_FRAME_MAGIC 			0x49565330 //'IVS0'

#define J_SDK_MAX_CPC_RULE_NUM          1
#define J_SDK_MAX_CPC_RULE_POINT        4
#define J_SDK_MAX_CPC_TARGET_POINT      1
#define J_SDK_MAX_CPC_TARGET_NUM        15
#define J_SDK_MAX_CPC_EVENT_NUM         1
#define J_SDK_MAX_CPC_NAME_LEN  	    32  // 规则名称长度


#ifndef _WIN32
# define DEF_PRAGMA_PACKED					__attribute__((packed))
#else
# define DEF_PRAGMA_PACKED					
#endif

#pragma pack(push, 1)

// 场景类型
typedef enum J_Scene_Type_E
{
	J_INDOOR_SCENE  = 0,		//  室内场景 
	J_OUTDOOR_SCENE	= 1 		// 室外场景 
} JSceneTypeE;

//穿越方向
typedef enum _J_Cross_Direction_E_
{  
    J_BOTH_DIRECTION  = 0,      //双向
    J_LEFT_GO_RIGHT   = 1,      //从左到右, 若线段处于水平位置，则为从上到下方向
    J_RIGHT_GO_LEFT   = 2       //从右到左, 若线段处于水平位置，则为从下到上方向
}JCrossDirectionE;

// 规则类型定义:
typedef enum _JPeaOscRuleType_E_
{
	J_PEA_OSC_RULE_BEG_TYPE     = 101,
    J_TRIPWIRE_RULE_TYPE        = 101,       //单绊线-->	JTripWireRuleS
    J_MUTITRIPWIRE_RULE_TYPE    = 102,       //双绊线,暂不支持
    J_PERI_METER_RULE_TYPE      = 103,       //警戒区-->	JPerimeterRuleS
    J_LOITER_RULE_TYPE          = 104,       //徘徊,暂不支持
	J_LEFT_RULE_TYPE            = 105,       //物品遗留检测-->JLeftRuleS
    J_TAKE_RULE_TYPE            = 106,        //物品丢失检测--> JTakeRuleS
    J_PEA_OSC_RULE_BUTT_TYPE
}JPeaOscRuleTypeE;

// 事件类型定义
typedef enum _JPeaOscEventType_E_
{
	J_TGT_NONE_EVENT 	  	  = 0x00000000, // 未触发事件
	J_TGT_UNKNOWN_EVENT       = 0x00000001,	// 未知事件
    J_TGT_TRIPWIRE_EVENT      = 0x00000002, //单绊线
    J_TGT_MUTITRIPWIRE_EVENT  = 0x00000004, //双绊线
	J_TGT_PERI_METER_EVENT	  = 0x00000008, //警戒区报警
	J_TGT_LOITER_EVENT        = 0x00000010,	// 徘徊 
	J_TGT_LEFT_EVENT		  = 0x00000020,	//物品遗留
	J_TGT_TAKE_EVENT	      = 0x00000040, //物品丢失
}JPeaOscEventTypeE;

// 线
typedef struct _jLine
{
	JPoint start_point;
	JPoint end_point;
}JLine;

// 矩形--> JRectangle 

// 多边形
typedef struct _jPolygon
{
    uint32_t	point_num;                        //多边形点数
    JPoint		points[J_SDK_MAX_POLYGON_POINT];  //多边形点坐标
}JPolygon;

//单绊线参数  
typedef struct _JTripWireRuleS_
{
    JLine 	stLine;     //线参数
    uint32_t u32CrossDir;  //JCrossDirectionE方向                       
}JTripWireRuleS;

//周界防范
typedef struct _JPerimeterRuleS_
{
    JPolygon	stPolygon;          //多边形区域
    uint32_t	u32Mode;            //周界模式 0 入侵 1 进入 2 离开
}JPerimeterRuleS;

// 物品遗留
typedef struct _JLeftRuleS_
{
    JPolygon	stPolygon;          //多边形区域
}JLeftRuleS;

// 物品丢失
typedef struct _JTakeRuleS_
{
    JPolygon	stPolygon;          //多边形区域
}JTakeRuleS;

// 规则联合体
typedef union _jPeaOscRuleDefine_U_
{
	JTripWireRuleS stTripWire;
	JPerimeterRuleS stPerimeter;
	JLeftRuleS 		stLeft;
	JTakeRuleS 		stTake;
	uint8_t        u8Size[64]; // 固定大小
}jPeaOscRuleDefineU;

// 规则参数定义
typedef struct _jPeaOscRuleDefine
{
	uint8_t u8RuleType; // JPeaOscRuleTypeE
	uint8_t u8Res[3];
	jPeaOscRuleDefineU uRuleDefine;
}JPeaOscRuleDefine;

//目标过滤器参数结构体( 暂不支持，预留)
typedef struct _jPeaOscTgtFliter_
{
    uint8_t		u8Enable;                   //使能目标过滤
	uint8_t 	u8Res[3];
    uint32_t   	u32FliterTgts;              // 目前，bit0: 人, bit1: 车
}JPeaOscTgtFliter;

//时间过滤参数( 目前仅物品遗留、物品丢失规则支持最小时间限制，
//最大时间限制均不支持, 定义仅作预留使用)
typedef struct _jPeaOscTimeFliter_
{
    uint8_t           u8Enable;                   //使能过滤
    uint8_t           u8MinTime;                  //最小时间
    uint8_t           u8MaxTime;                  //最大时间
    uint8_t           u8Res;
}JPeaOscTimeFliter;

//尺寸大小限制参数
typedef struct _jPeaOscAreaFliter_
{
	uint32_t		u32MinSize;				//最小尺寸
	uint32_t		u32MaxSize;				//最大尺寸
}jPeaOscAreaFliter;

//尺寸宽高限制参数
typedef struct _jPeaOscWHFliter_
{	
	uint16_t		u16MinWidth;				//最小宽度
	uint16_t		u16MinHeight;				//最小高度
	uint16_t		u16MaxWidth;				//最大宽度
	uint16_t		u16MaxHeight;				//最大高度
}jPeaOscWHFliter;

//尺寸过滤器参数结构体( 目前仅物品遗留、物品丢失规则支持u8FilterType = 0 的
//限制参数, 其他定义仅作预留使用)
typedef struct _jPeaOscSizeFliter_
{
    uint8_t           u8Enable;                   // 使能尺寸过滤
    uint8_t           u8FilterType;				// 限制类型，0   限制总体尺寸，1   限制宽高
    uint8_t           u8Res[2];
    union
    {
        jPeaOscWHFliter stParaWH;  
        jPeaOscAreaFliter stParaArea;
    };
}JPeaOscSizeFliter;

// 规则限制定义
typedef struct _jPeaOscRuleLimit
{
	JPeaOscTgtFliter	stTgtFilter;
	JPeaOscTimeFliter	stTimeFilter;
	JPeaOscSizeFliter   stSizeFilter;
}JPeaOscRuleLimit;

// 规则异常处理定义
typedef struct _jPeaOscRuleExpHdl
{
	uint8_t	report_alarm; 	// 客户端/平台是否上报J_SDK_PEA_OSC_ALARM 告警
	uint8_t u8Res[3];
	JJointAction joint_action;	// 事件联动
}JPeaOscRuleExpHdl;

// 检测规则
typedef struct _jPeaOscRule
{
	uint8_t check_enable;	//使能规则
	uint8_t check_level;	//警戒级别0 ~3 无警报 低级警报 中级警报 高级警报，预留定义
	uint8_t res[6];         // 预留
	uint8_t rule_name[J_SDK_MAX_PEA_OSC_NAME_LEN]; //规则名称
	JPeaOscRuleDefine stRulePara;   // 规则参数定义
	JPeaOscRuleLimit  stRuleLimit;  // 规则限制定义
	JPeaOscRuleExpHdl stRuleExpHdl; // 异常处理定义
}JPeaOscRule;

// PEA/OSC 检测配置项
typedef struct _jPeaOscCfg
{
    uint8_t	enable;     // pea,osc 使能
    uint8_t scene_type; // 场景类型JSceneTypeE ，预留定义，目前设置无效果
	uint8_t	target_level; //目标输出灵敏度级别0~2 高中低，预留定义，目前设置无效果
	uint8_t res[13];   // 预留
	JSegment	sched_time[J_SDK_MAX_SEG_SZIE]; // 时间段设置
	uint16_t rule_max_width;  //规则定义参考宽度
	uint16_t rule_max_height; //规则定义参考高度
	JPeaOscRule rules[J_SDK_MAX_PEA_OSC_RULE_NUM]; // 检测规则
}JPeaOscCfg;

// ----------------------   PEA / OSC 帧数据内容格式定义Begin  ------------------//

// 数据包含信息
typedef struct _jPeaOscDataInfo
{
	uint8_t info_mask;     // 目前bit0: 有效帧标识，为0 表示清除所有的规则/目标		
	uint8_t rule_num;      // 包含规则个数(info_mask & 0x1 == 1时有效)
	uint8_t event_num;     // 包含事件个数(info_mask & 0x1 == 1时有效)
	uint8_t target_num;    // 包含目标个数(info_mask & 0x1 == 1时有效)
	uint16_t refer_width;	// 参考宽度
	uint16_t refer_height;	// 参考高度
	uint32_t time_stamp;	// 时间戳(info_mask & 0x1 == 1 时有效)
}JPeaOscDataInfo;

// 分析结果规则参数定义
typedef struct _jPeaOscResultRule
{
	uint8_t  u8RuleType; // JPeaOscRuleTypeE
	uint8_t  u8RuleIdx; // 实际规则号( 0 ~ J_SDK_MAX_PEA_OSC_RULE_NUM - 1)
	uint8_t  u8Res[2];
	jPeaOscRuleDefineU uRuleDefine;
}JPeaOscResultRule;

// 事件信息定义
typedef struct _jPeaOscEvent
{
	uint32_t event_type;				//事件类型  JPeaOscEventTypeE
	uint32_t event_id;					//事件标识 
	uint8_t event_rule_index; 			//事件触发的规则编号, 值为JPeaOscResultRule.u8RuleIdx, 即参数配置里的第几条规则
	uint8_t event_level; 				//事件警报级别
	uint8_t res[2];
	uint32_t event_tgt_id;				// 事件触发目标ID
	JRectangle tgt_rect; 				// 事件触发目标所在区域
}JPeaOscEvent;

// 目标信息定义
typedef struct _jPeaOscTarget
{
	uint32_t tgt_size; 	// 该目标信息长度
    uint32_t tgt_id;  	// 目标ID
    uint32_t tgt_event;	//目标触发的事件类型 JPeaOscEventTypeE
	uint8_t tgt_rule_index; //目标 触发的规则编号, 值为JPeaOscResultRule.u8RuleIdx, 即参数配置里的第几条规则,未触发规则为0xff 
	uint8_t tgt_type;	// 目标类型人，车等 0XFF: 所有类型/未知类型,1 人2 车
	uint8_t tgt_have_track; // 是否有轨迹信息
	uint8_t tgt_track_point_num; //轨迹点个数
	uint8_t res1[8];	// 预留
	JRectangle tgt_rect; // 目标所在区域
	int16_t	tgt_move_speed;		// 速率（coord /s）-1 未知
	int16_t tgt_move_direction; // 方向（0~359度）-1 未知
	JPoint  tgt_track_points[J_SDK_MAX_TARGET_TRACK_POINT]; // 实际个数可变，由tgt_track_point_num 值指定
}JPeaOscTarget;

/******************************************************************************
 PeaOsc 帧数据定义(大小可变)
 JPeaOscResultRule *pRules = ( JPeaOscResultRule *)((char *)&JPeaOscFrameData + sizeof(JPeaOscDataInfo));
 JPeaOscEvent *pEvents = (JPeaOscEvent *)(pRules + rule_num);
 JPeaOscTargets *pTargets = (JPeaOscTargets *)(pEvents + event_num);
******************************************************************************/
typedef struct _jPeaOscFrameData
{
	JPeaOscDataInfo stDataInfo;
	JPeaOscResultRule stRuleDef[J_SDK_MAX_PEA_OSC_RULE_NUM]; // 实际个数可变，由rule_num 指定
	JPeaOscEvent stEvents[J_SDK_MAX_EVENT_NUM];		// 实际个数可变，由event_num 指定
	JPeaOscTarget stTargets[J_SDK_MAX_TARGET_NUM];    	// 实际个数可变，由target_num 指定
}JPeaOscFrameData;

typedef enum _JIvsFrameType_E_
{
	J_IVS_PEA_OSC_FRAME = 0x00000001, // PEA OSC 分析结果数据---> JPeaOscFrameData
	J_IVS_CPC_FRAME     = 0x00000002, // CPC数据
	J_IVS_HERD_FRAME	= 0x00000004, // 牧业分析数据
    J_IVS_FRAME_BUTT_TYPE
}JIvsFrameTypeE;
// ----------------------   PEA / OSC 帧数据内容格式定义End  ------------------//

//------------------------CPC---------------------------------------------------------//

//CPC检测规则
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
	uint16_t rule_max_width;  //规则定义参考宽度
	uint16_t rule_max_height; //规则定义参考高度
    JCpcRuleCfg rule_cfg;
}JCpcCfg;

//===============cpc 分析结果==============
// 数据包含信息
typedef struct _JCpcDataInfo
{
	uint8_t info_mask;     // 目前bit0: 有效帧标识，为0 表示清除所有的规则/目标		
	uint8_t rule_num;      // 包含规则个数(info_mask & 0x1 == 1时有效)
	uint8_t event_num;     // reserved
	uint8_t target_num;    // 包含目标个数(info_mask & 0x1 == 1时有效)
	uint16_t refer_width;	// 参考宽度
	uint16_t refer_height;	// 参考高度
	uint32_t time_stamp;	// 时间戳(info_mask & 0x1 == 1 时有效)
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
    J_CPC_ENTER_CROSS  = 0,     // 进入
    J_CPC_LEAVE_CROSS,          // 离开
}JCpcCrossTypeE;

//cpc target info
typedef struct _JCpcTarget
{
    uint32_t  size;          //
    uint32_t  target_size;   // 目标（人）头（方形框）尺寸, 介于最小最大之间
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
//=============cpc 分析结果 end==============
//------------------------CPC End-----------------------------------------------------//
// ----------------------   牧业分析 帧数据内容格式定义Begin  ------------------//

/******************************************************************************
 牧业分析 帧数据包含
 J_IVS_FRAME_DATA + JHerdTroughInfo + 多个HI_TROUGH_HEAD_S
 
******************************************************************************/

// 牧业分析信息结构定义
typedef struct _jHerdTroughInfo
{
	uint8_t data_mode; 		// 0: 1位代表1个小方块区域信息， 1: 1个字节代表1个小方块区域信息;目前默认为0 
	uint8_t area_mode; 		// 0: 代表仅上报在区域内部的小方块草料信息，数据从上到下从左到右分布，
							// 1:  代表上报各区域最小外接矩形区域的小方块草料信息，数据从上到下从左到右分布，目前默认为0 
	uint8_t rep_size;		// 上报是否有草料区域单位大小1: 1x1 方块2: 2X2方块... 大小参照nCSWidth 和 nCSHeight
	uint8_t had_flag;  		// 是否带草料分布数据信息
	uint16_t refer_width;	// 参考宽度
	uint16_t refer_height;	// 参考高度
}JHerdTroughInfo;

//牧业分析每个槽的数据
typedef struct _HI_TROUGH_HEAD_S_
{
	uint8_t		 trough_id;
	uint8_t		 area_flag; //0内槽，1外槽	
	uint8_t		 percent;
	uint8_t		 res;
	JPoint 		 point[4];
	uint32_t	 data_len;
	void 		*pData[0];
}HI_TROUGH_HEAD_S;


// ----------------------   牧业分析 帧数据内容格式定义End  ------------------//

// 智能分析帧结构定义
typedef struct _IVS_FRAME_DATA_
{
	uint32_t u32Magic; // J_SDK_IVS_FRAME_MAGIC 网络字节序
	uint32_t u32Type;  // JIvsFrameTypeE
	uint32_t u32Size;	// Data Size
	void *pData[0];
}J_IVS_FRAME_DATA;

#pragma pack(pop)

#endif //__J_PEA_OSC_H__

