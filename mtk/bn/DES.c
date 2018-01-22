// DES.cpp: implementation of the CDES class.
//
//////////////////////////////////////////////////////////////////////
#include "DES.h"

#define false  0
#define true   1
//#include "NormalTool.h"


static uchar RunDes(uchar bType,uchar bMode,char* In,char* Out,unsigned datalen,const char* Key,const unsigned char keylen,const char* cveci);

//计算并填充子密钥到SubKey数据中
static void SetSubKey(PSubKey pSubKey, const char Key[8]);

//DES单元运算
static void DES(char Out[8], char In[8], const PSubKey pSubKey, uchar Type);
////////////////////////////////////////////////////////////////////////
// initial permutation IP
const char IP_Table[64] = {
	58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
		62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
		57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
		61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7
};
// final permutation IP^-1
const char IPR_Table[64] = {
	40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
		38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
		36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
		34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25
};
// expansion operation matrix
const char E_Table[48] = {
	32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
		8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
		16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
		24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1
};
// 32-bit permutation function P used on the output of the S-boxes
const char P_Table[32] = {
	16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10,
		2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25
};
// permuted choice table (key)
const char PC1_Table[56] = {
	57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18,
		10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36,
		63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22,
		14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4
};
// permuted choice key (table)
const char PC2_Table[48] = {
	14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10,
		23, 19, 12, 4, 26, 8, 16, 7, 27, 20, 13, 2,
		41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
		44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};
// number left rotations of pc1
const char LOOP_Table[16] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};
// The (in)famous S-boxes
const char S_Box[8][4][16] = {
	// S1
	{
		{14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
		{0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
		{4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
		{15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
	},
	// S2
	{
		{15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
		{3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
		{0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
		{13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
	},
	// S3
	{
		{10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
		{13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
		{13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
		{1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
	},
	// S4
	{
		{7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
		{13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
		{10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
		{3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
	},
	// S5
	{
		{2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
		{14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
		{4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
		{11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
	},
	// S6
	{
		{12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
		{10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
		{9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
		{4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
	},
	// S7
	{
		{4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
		{13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
		{1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
		{6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
	},
	// S8
	{
		{13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
		{1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
		{7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
		{2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
	}
};




/*******************************************************************/
/*
  函 数 名 称:	ByteToBit
  功 能 描 述：	把BYTE转化为Bit流
  参 数 说 明：	Out:	输出的Bit流[in][out]
				In:		输入的BYTE流[in]
				bits:	Bit流的长度[in]

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void ByteToBit(uchar *Out, const char *In, int bits)
{
     int i;
    for ( i=0; i<bits; ++i)
        Out[i] = (In[i>>3]>>(7 - (i&7))) & 1;
}

/*******************************************************************/
/*
  函 数 名 称:	BitToByte
  功 能 描 述：	把Bit转化为Byte流
  参 数 说 明：	Out:	输出的BYTE流[in][out]
				In:		输入的Bit流[in]
				bits:	Bit流的长度[in]

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void BitToByte(char *Out, const uchar *In, int bits)
{
    int i;
    memset(Out, 0, bits>>3);
    for ( i=0; i<bits; ++i)
        Out[i>>3] |= In[i]<<(7 - (i&7));
}



/*******************************************************************/
/*
  函 数 名 称:	RotateL
  功 能 描 述：	把BIT流按位向左迭代
  参 数 说 明：	In:		输入的Bit流[in]
				len:	Bit流的长度[in]
				loop:	向左迭代的长度

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void RotateL(uchar *In, int len, int loop)
{
	uchar Tmp[256];

    memcpy(Tmp, In, loop);
    memcpy(In, In+loop, len-loop);
    memcpy(In+len-loop, Tmp, loop);
}



/*******************************************************************/
/*
  函 数 名 称:	Xor
  功 能 描 述：	把两个Bit流进行异或
  参 数 说 明：	InA:	输入的Bit流[in][out]
				InB:	输入的Bit流[in]
				loop:	Bit流的长度

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void Xor(uchar *InA, const uchar *InB, int len)
{
     int i;
    for ( i=0; i<len; ++i)
        InA[i] ^= InB[i];
}


/*******************************************************************/
/*
  函 数 名 称:	Transform
  功 能 描 述：	把两个Bit流按表进行位转化
  参 数 说 明：	Out:	输出的Bit流[out]
				In:		输入的Bit流[in]
				Table:	转化需要的表指针
				len:	转化表的长度

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void Transform(uchar *Out, uchar *In, const char *Table, int len)
{
	uchar Tmp[256];
	int i;

    for (i=0; i<len; ++i)
        Tmp[i] = In[ Table[i]-1 ];
    memcpy(Out, Tmp, len);
}



/*******************************************************************/
/*
  函 数 名 称:	S_func
  功 能 描 述：	实现数据加密S BOX模块
  参 数 说 明：	Out:	输出的32Bit[out]
				In:		输入的48Bit[in]

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void S_func(uchar Out[32], const uchar In[48])
{
    unsigned char i, j , k ;
	int l;
    for (i=0; i<8; ++i,In+=6,Out+=4)
	{
        j = (In[0]<<1) + In[5];
        k = (In[1]<<3) + (In[2]<<2) + (In[3]<<1) + In[4];	//组织SID下标

		for ( l=0; l<4; ++l)								//把相应4bit赋值
			Out[l] = (S_Box[i][j][k]>>(3 - l)) & 1;
    }
}


/*******************************************************************/
/*
  函 数 名 称:	F_func
  功 能 描 述：	实现数据加密到输出P
  参 数 说 明：	Out:	输出的32Bit[out]
				In:		输入的48Bit[in]

  返回值 说明：	void
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/
static void F_func(uchar In[32], const uchar Ki[48])
{
    uchar MR[48];
    Transform(MR, In, E_Table, 48);
    Xor(MR, Ki, 48);
    S_func(In, MR);
    Transform(In, In, P_Table, 32);
}


/*******************************************************************/
/*
  函 数 名 称:	RunDes
  功 能 描 述：	执行DES算法对文本加解密
  参 数 说 明：	bType	:类型：加密ENCRYPT，解密DECRYPT
				bMode	:模式：ECB,CBC
				In		:待加密串指针
				Out		:待输出串指针
				datalen	:待加密串的长度，同时Out的缓冲区大小应大于或者等于datalen
				Key		:密钥(可为8位,16位,24位)支持3密钥
				keylen	:密钥长度，多出24位部分将被自动裁减

  返回值 说明：	uchar	:是否加密成功
  作       者:	邹德强
  更 新 日 期：	2003.12.19
*******************************************************************/

uchar RunDes(uchar bType,uchar bMode,char* In,char* Out,unsigned datalen,const char* Key,const unsigned char keylen, const char* cveci)
{
	char cvec[8];
	uchar m_SubKey[3][16][48];
					//秘钥
	unsigned char nKey;
	int i, j, k;

	//判断输入合法性
	if (!(In && Out && Key && datalen && keylen>=8))
		return false;
	//只处理8的整数倍，不足长度自己填充
	if (datalen & 0x00000007)
		return false;

	memcpy(cvec, cveci, 8);

	//构造并生成SubKeys
	 nKey	=	(keylen>>3)>3 ? 3: (keylen>>3);
	for ( i=0;i<nKey;i++)
	{
		SetSubKey(&m_SubKey[i],&Key[i<<3]);
	}

	if (bMode == ECB)	//ECB模式
	{
		if (nKey	==	1)	//单Key
		{
			for (i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[0],bType);
			}
		}
		else
		if (nKey == 2)	//3DES 2Key
		{
			for ( i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[0],bType);
			}
		}
		else			//3DES 3Key
		{
			for (i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[bType? 2 : 0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[bType? 0 : 2],bType);
			}
		}
	}
	else				//CBC模式
	{
		char	cvin[8]	=	""; //中间变量

		if (nKey == 1)	//单Key
		{
			for ( i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if (bType	==	ENCRYPT)
				{
					for ( k=0;k<8;++k)		//将输入与扭转变量异或
					{
						cvin[k]	=	In[k] ^ cvec[k];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}

				DES(Out,cvin,&m_SubKey[0],bType);

				if (bType	==	ENCRYPT)
				{
					memcpy(cvec,Out,8);			//将输出设定为扭转变量
				}
				else
				{
					for ( k=0;k<8;++k)		//将输出与扭转变量异或
					{
						Out[k]	=	Out[k] ^ cvec[k];
					}
					memcpy(cvec,cvin,8);			//将输入设定为扭转变量
				}
			}
		}
		else
		if (nKey == 2)	//3DES CBC 2Key
		{
			for ( i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if (bType	==	ENCRYPT)
				{
					for ( k=0;k<8;++k)		//将输入与扭转变量异或
					{
						cvin[k]	=	In[k] ^ cvec[k];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}

				DES(Out,cvin,&m_SubKey[0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[0],bType);

				if (bType	==	ENCRYPT)
				{
					memcpy(cvec,Out,8);			//将输出设定为扭转变量
				}
				else
				{
					for ( k=0;k<8;++k)		//将输出与扭转变量异或
					{
						Out[k]	=	Out[k] ^ cvec[k];
					}
					memcpy(cvec,cvin,8);			//将输入设定为扭转变量
				}
			}
		}
		else			//3DES CBC 3Key
		{
			for ( i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if (bType	==	ENCRYPT)
				{
					for ( k=0;k<8;++k)		//将输入与扭转变量异或
					{
						cvin[k]	=	In[k] ^ cvec[k];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}

				DES(Out,cvin,&m_SubKey[bType ? 2 : 0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[bType ? 0 : 2],bType);

				if (bType	==	ENCRYPT)
				{
					memcpy(cvec,Out,8);			//将输出设定为扭转变量
				}
				else
				{
					for ( k=0;k<8;++k)		//将输出与扭转变量异或
					{
						Out[k]	=	Out[k] ^ cvec[k];
					}
					memcpy(cvec,cvin,8);			//将输入设定为扭转变量
				}
			}
		}
	}

	return true;
}




/*******************************************************************/
/*
  函 数 名 称:	RunPad
  功 能 描 述：	根据协议对加密前的数据进行填充
  参 数 说 明：	bType	:类型：PAD类型
				In		:数据串指针
				Out		:填充输出串指针
				datalen	:数据的长度
				padlen	:(in,out)输出buffer的长度，填充后的长度

  返回值 说明：	uchar	:是否填充成功
  作       者:	邹德强
  修 改 历 史：

  更 新 日 期：	2003.12.19
*******************************************************************/
#if 0
uchar	RunPad(int nType,const char* In,unsigned datalen,char* Out,unsigned& padlen)
{
	int res = (datalen & 0x00000007);


	if (padlen< (datalen+8-res))
	{
		return false;
	}
	else
	{
		padlen	=	(datalen+8-res);
		memcpy(Out,In,datalen);
	}


	if (nType	==	PAD_ISO_1)
	{
		memset(Out+datalen,0x00,8-res);
	}
	else
	if (nType	==	PAD_ISO_2)
	{
		memset(Out+datalen,0x80,1);
		memset(Out+datalen,0x00,7-res);
	}
	else
	if (nType	==	PAD_PKCS_7)
	{
		memset(Out+datalen,8-res,8-res);
	}
	else
	{
		return false;
	}

	return true;
}

#endif


//计算并填充子密钥到SubKey数据中
void SetSubKey(PSubKey pSubKey, const char Key[8])
{
    int i;
	uchar K[64], *KL=&K[0], *KR=&K[28];
    ByteToBit(K, Key, 64);
    Transform(K, K, PC1_Table, 56);
    for ( i=0; i<16; ++i) {
        RotateL(KL, 28, LOOP_Table[i]);
        RotateL(KR, 28, LOOP_Table[i]);
        Transform((*pSubKey)[i], K, PC2_Table, 48);
    }
}



//DES单元运算
void DES(char Out[8], char In[8], const PSubKey pSubKey, uchar Type)
{
    int i;
    uchar M[64], tmp[32], *Li=&M[0], *Ri=&M[32];
    ByteToBit(M, In, 64);
    Transform(M, M, IP_Table, 64);
    if ( Type == ENCRYPT )
	{
        for ( i=0; i<16; ++i)
		{
            memcpy(tmp, Ri, 32);		//Ri[i-1] 保存
            F_func(Ri, (*pSubKey)[i]);	//Ri[i-1]经过转化和SBox输出为P
            Xor(Ri, Li, 32);			//Ri[i] = P XOR Li[i-1]
            memcpy(Li, tmp, 32);		//Li[i] = Ri[i-1]
        }
    }
	else
	{
        for ( i=15; i>=0; --i)
		{
			memcpy(tmp, Ri, 32);		//Ri[i-1] 保存
            F_func(Ri, (*pSubKey)[i]);	//Ri[i-1]经过转化和SBox输出为P
            Xor(Ri, Li, 32);			//Ri[i] = P XOR Li[i-1]
            memcpy(Li, tmp, 32);		//Li[i] = Ri[i-1]
        }
	}
	RotateL(M,64,32);					//Ri与Li换位重组M
    Transform(M, M, IPR_Table, 64);		//最后结果进行转化
    BitToByte(Out, M, 64);				//组织成字符
}

/*
input: 11223344556677881122334455667788
key: 11112222333344445555666677778888

3des-cbc (with init-vector 0000...00): 3C1C389B59528ECFDD0292A92FCBB3CB
3des-ecb: 3C1C389B59528ECF3C1C389B59528ECF

input: 11223344556677881122334455667788
key: 1111222233334444

des-cbc with init-vector 0000...00: D7BFC9EF6781CD2213366859BB540BF8
des-ecb: D7BFC9EF6781CD22D7BFC9EF6781CD22
         11223344556677881122334455667788
*/

//	static uchar RunDes(uchar bType,uchar bMode,char* In,char* Out,unsigned datalen,const char* Key,const unsigned char keylen);
/*******************************************************************/
/*
  函 数 名 称:	RunDes
  功 能 描 述：	执行DES算法对文本加解密
  参 数 说 明：	bType	:类型：加密ENCRYPT，解密DECRYPT
				bMode	:模式：ECB,CBC
				In		:待加密串指针
				Out		:待输出串指针
				datalen	:待加密串的长度，同时Out的缓冲区大小应大于或者等于datalen
				Key		:密钥(可为8位,16位,24位)支持3密钥
				keylen	:密钥长度，多出24位部分将被自动裁减

  返回值 说明：	uchar	:是否加密成功
  作       者:	邹德强
  修 改 历 史：	2004.7.6,修改ECB/CBC 3key模式bug

  更 新 日 期：	2003.12.19
*******************************************************************/

void DesCbc2keyEncode(uchar *data, int len, uchar *key, const char* cveci)
{
	RunDes(0, 1, (char *)data, (char *)data, len, (char *)key, 8, cveci);
}

void DesCbc2keyDecode(uchar *data, int len, uchar *key, const char* cveci)
{
	RunDes(1, 1, (char *)data, (char *)data, len, (char *)key, 8, cveci);
}

void ThDesCbc2keyEncode(uchar *data, int len, uchar *key, const char* cveci)
{
	RunDes(0, 1, (char *)data, (char *)data, len, (char *)key, 16, cveci);
}

void ThDesCbc2keyDecode(uchar *data, int len, uchar *key, const char* cveci)
{
	RunDes(1, 1, (char *)data, (char *)data, len, (char *)key, 16, cveci);
}
