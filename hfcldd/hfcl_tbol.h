/*
 * hfcl_tbol.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_tbol.h,v 1.3.8.7.28.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:55 toyo Exp $
 */


/*--------------------------------------------------------------------------*/
/*                  Little Endian <-> Big Endian conversion macro           */
/*                                                                          */
/*                  Conversion from Big Endian to Little Endian             */
/*                                                                          */
/* Macro name : HFC_2B_TO_2L(Ltl, Big)                                      */
/* Function   : Convert 2 bytes value from Big-Endian to Little-Endian      */
/*                                                                          */
/* Macro name : HFC_4B_TO_4L(Ltl, Big)                                      */
/* Function   : Convert 4 bytes value from Big-Endian to Little-Endian      */
/*                                                                          */
/* Macro name : HFC_8B_TO_XL(Ltl, Big)                                      */
/* Function   : Convert 8 bytes value from Big-Endian to Little-Endian      */
/*                                                                          */
/* Macro name : HFC_BP_TO_LP(Ltl, Big)                                      */
/* Function   : Convert 8 bytes pointer from Big-Endian to Little-Endian    */
/*              Big-Endian format is only allowed for 8bytes pointer        */
/*                                                                          */
/*                  Conversion from Little Endian to Big Endian             */
/*                                                                          */
/* Macro name : HFC_2L_TO_2B(Big, Ltl)                                      */
/* Function   : Convert 2 bytes value from Little-Endian to Big-Endian      */
/*                                                                          */
/* Macro name : HFC_4L_TO_4B(Big, Ltl)                                      */
/* Function   : Convert 4 bytes value from Little-Endian to Big-Endian      */
/*                                                                          */
/* Macro name : HFC_8L_TO_8B(Big, Ltl)                                      */
/* Function   : Convert 8 bytes value from Little-Endian to Big-Endian      */
/*                                                                          */
/* Macro name : HFC_LP_TO_BP(Big, Ltl)                                      */
/* Function   : Convert 8 bytes pointer from Little-Endian to Big-Endian    */
/*              Big-Endian format is only allowed for 8bytes pointer        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct _HFC_2BYTE{
	unsigned char Byte0;
	unsigned char Byte1;
} HFC_2BYTE;

typedef struct _HFC_4BYTE{
	unsigned char Byte0;
	unsigned char Byte1;
	unsigned char Byte2;
	unsigned char Byte3;
} HFC_4BYTE;

typedef struct _HFC_8BYTE{
	unsigned char Byte0;
	unsigned char Byte1;
	unsigned char Byte2;
	unsigned char Byte3;
	unsigned char Byte4;
	unsigned char Byte5;
	unsigned char Byte6;
	unsigned char Byte7;
} HFC_8BYTE;


#define HFC_2B_TO_XL(LtlEnd2, BigEnd, Order){                   \
	(((Order) >= 2) ? ((((HFC_2BYTE *)(&(LtlEnd2)))->Byte1) =   \
	(((HFC_2BYTE *)(&(BigEnd)))->Byte0)) : 0);                  \
	(((Order) >= 1) ? ((((HFC_2BYTE *)(&(LtlEnd2)))->Byte0) =   \
	(((HFC_2BYTE *)(&(BigEnd)))->Byte1)) : 0);                  \
}

#define HFC_4B_TO_XL(LtlEnd4, BigEnd, Order){                   \
	(((Order) >= 4) ? ((((HFC_4BYTE *)(&(LtlEnd4)))->Byte3) =   \
	(((HFC_4BYTE *)(&(BigEnd)))->Byte0)) : 0);                  \
	(((Order) >= 3) ? ((((HFC_4BYTE *)(&(LtlEnd4)))->Byte2) =   \
	(((HFC_4BYTE *)(&(BigEnd)))->Byte1)) : 0);                  \
	(((Order) >= 2) ? ((((HFC_4BYTE *)(&(LtlEnd4)))->Byte1) =   \
	(((HFC_4BYTE *)(&(BigEnd)))->Byte2)) : 0);                  \
	(((Order) >= 1) ? ((((HFC_4BYTE *)(&(LtlEnd4)))->Byte0) =   \
	(((HFC_4BYTE *)(&(BigEnd)))->Byte3)) : 0);                  \
}

#define HFC_8B_TO_XL(LtlEnd8, BigEnd, Order){                   \
	(((Order) >= 8) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte7) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte0)) : 0);                  \
	(((Order) >= 7) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte6) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte1)) : 0);                  \
	(((Order) >= 6) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte5) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte2)) : 0);                  \
	(((Order) >= 5) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte4) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte3)) : 0);                  \
	(((Order) >= 4) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte3) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte4)) : 0);                  \
	(((Order) >= 3) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte2) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte5)) : 0);                  \
	(((Order) >= 2) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte1) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte6)) : 0);                  \
	(((Order) >= 1) ? ((((HFC_8BYTE *)(&(LtlEnd8)))->Byte0) =   \
	(((HFC_8BYTE *)(&(BigEnd)))->Byte7)) : 0);                  \
}

#define HFC_XL_TO_2B(BigEnd2, LtlEnd, Order){                   \
	((HFC_2BYTE *)(&(BigEnd2)))->Byte0 =                        \
	(((Order) >= 2) ? (((HFC_2BYTE *)(&(LtlEnd)))->Byte1) : 0); \
	((HFC_2BYTE *)(&(BigEnd2)))->Byte1 =                        \
	(((Order) >= 1) ? (((HFC_2BYTE *)(&(LtlEnd)))->Byte0) : 0); \
}

#define HFC_XL_TO_4B(BigEnd4, LtlEnd, Order){                   \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte0 =                        \
	(((Order) >= 4) ? (((HFC_4BYTE *)(&(LtlEnd)))->Byte3) : 0); \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte1 =                        \
	(((Order) >= 3) ? (((HFC_4BYTE *)(&(LtlEnd)))->Byte2) : 0); \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte2 =                        \
	(((Order) >= 2) ? (((HFC_4BYTE *)(&(LtlEnd)))->Byte1) : 0); \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte3 =                        \
	(((Order) >= 1) ? (((HFC_4BYTE *)(&(LtlEnd)))->Byte0) : 0); \
}

#define HFC_XL_TO_8B(BigEnd8, LtlEnd, Order){                   \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte0 =                        \
	(((Order) >= 8) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte7) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte1 =                        \
	(((Order) >= 7) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte6) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte2 =                        \
	(((Order) >= 6) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte5) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte3 =                        \
	(((Order) >= 5) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte4) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte4 =                        \
	(((Order) >= 4) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte3) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte5 =                        \
	(((Order) >= 3) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte2) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte6 =                        \
	(((Order) >= 2) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte1) : 0); \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte7 =                        \
	(((Order) >= 1) ? (((HFC_8BYTE *)(&(LtlEnd)))->Byte0) : 0); \
}

#define HFC_2B_TO_2L(LtlEnd2, BigEnd2) {                                        \
	((HFC_2BYTE *)(&(LtlEnd2)))->Byte1 = ((HFC_2BYTE *)(&(BigEnd2)))->Byte0;    \
	((HFC_2BYTE *)(&(LtlEnd2)))->Byte0 = ((HFC_2BYTE *)(&(BigEnd2)))->Byte1;    \
}

#define HFC_4B_TO_4L(LtlEnd4, BigEnd4) {                                        \
	((HFC_4BYTE *)(&(LtlEnd4)))->Byte3 = ((HFC_4BYTE *)(&(BigEnd4)))->Byte0;    \
	((HFC_4BYTE *)(&(LtlEnd4)))->Byte2 = ((HFC_4BYTE *)(&(BigEnd4)))->Byte1;    \
	((HFC_4BYTE *)(&(LtlEnd4)))->Byte1 = ((HFC_4BYTE *)(&(BigEnd4)))->Byte2;    \
	((HFC_4BYTE *)(&(LtlEnd4)))->Byte0 = ((HFC_4BYTE *)(&(BigEnd4)))->Byte3;    \
}

#define HFC_8B_TO_8L(LtlEnd8, BigEnd8) {                                        \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte7 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte0;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte6 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte1;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte5 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte2;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte4 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte3;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte3 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte4;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte2 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte5;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte1 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte6;    \
	((HFC_8BYTE *)(&(LtlEnd8)))->Byte0 = ((HFC_8BYTE *)(&(BigEnd8)))->Byte7;    \
}
	
#define HFC_2L_TO_2B(BigEnd2, LtlEnd2) {                                        \
	((HFC_2BYTE *)(&(BigEnd2)))->Byte1 = ((HFC_2BYTE *)(&(LtlEnd2)))->Byte0;    \
	((HFC_2BYTE *)(&(BigEnd2)))->Byte0 = ((HFC_2BYTE *)(&(LtlEnd2)))->Byte1;    \
}

#define HFC_4L_TO_4B(BigEnd4, LtlEnd4) {                                        \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte3 = ((HFC_4BYTE *)(&(LtlEnd4)))->Byte0;    \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte2 = ((HFC_4BYTE *)(&(LtlEnd4)))->Byte1;    \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte1 = ((HFC_4BYTE *)(&(LtlEnd4)))->Byte2;    \
	((HFC_4BYTE *)(&(BigEnd4)))->Byte0 = ((HFC_4BYTE *)(&(LtlEnd4)))->Byte3;    \
}

#define HFC_8L_TO_8B(BigEnd8, LtlEnd8) {                                        \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte7 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte0;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte6 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte1;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte5 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte2;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte4 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte3;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte3 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte4;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte2 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte5;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte1 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte6;    \
	((HFC_8BYTE *)(&(BigEnd8)))->Byte0 = ((HFC_8BYTE *)(&(LtlEnd8)))->Byte7;    \
}

#ifdef _WIN64
#define HFC_BP_TO_LP(LtlEndP, BigEndP) HFC_8B_TO_8L((LtlEndP), (BigEndP))
#define HFC_LP_TO_BP(LtlEndP, BigEndP) HFC_8L_TO_8B((LtlEndP), (BigEndP))
#else
#define HFC_BP_TO_LP(LtlEndP, BigEndP) HFC_8B_TO_XL((LtlEndP), (BigEndP), 4)
#define HFC_LP_TO_BP(LtlEndP, BigEndP) HFC_XL_TO_8B((LtlEndP), (BigEndP), 4)
#endif
	
