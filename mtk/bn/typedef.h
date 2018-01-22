/*******************************************************************************
Header file:	typedef.h
Description:	所有文件公用的一些数据类型宏定义,此头文件需要在任何.h文件和.c文件中包含
				这样方便可读,维护和移植
Author:			曾建清
Date:			2007-06-03 星期日
*******************************************************************************/

#ifndef		__TYPEDEF_H__
#define		__TYPEDEF_H__


#include	<stddef.h>

#ifndef		TRUE
#define		TRUE		1
#endif
#ifndef		FALSE
#define		FALSE		0
#endif

//typedef		signed	char	BOOL;
//typedef		unsigned char	uchar;
typedef		unsigned char	uint8;
typedef		signed	char	schar;
typedef		signed 	char	int8;
typedef		unsigned short	uint16;
typedef		  signed short	int16;
typedef		unsigned int	uint32;
typedef		  signed int	int32;
typedef		unsigned long	ulong32;
typedef		  signed long	long32;
typedef	unsigned long long	uint64;
typedef	  signed long long	int64;


#endif
