/****************************************************************************/
/* 対象マイコン R8C/35A                                                     */
/* ﾌｧｲﾙ内容     タイマRB割り込みによるタイマ                                */
/* バージョン   Ver.1.20                                                    */
/* Date         2010.04.19                                                  */
/* Copyright    ルネサスマイコンカーラリー事務局                            */
/*              日立インターメディックス株式会社                            */
/****************************************************************************/
/*
出力：P6_7-P6_0(LEDなど)

ポート6に繋いだLEDを1秒間隔で点滅させます。
タイマはタイマRB割り込みによる正確なタイマを使用します。
*/

/*======================================*/
/* インクルード                         */
/*======================================*/
#include "sfr_r835a.h"                  /* R8C/35A SFRの定義ファイル    */

/*======================================*/
/* シンボル定義                         */
/*======================================*/

/*======================================*/
/* プロトタイプ宣言                     */
/*======================================*/
void init( void );
unsigned char pushsw_get( void );
void timer( unsigned long timer_set );

/*======================================*/
/* グローバル変数の宣言                 */
/*======================================*/
unsigned long cnt_rb;                   /* タイマRB用                   */

/************************************************************************/
/* メインプログラム                                                     */
/************************************************************************/
void main( void )
{
	unsigned char d;
	int jud;
	
    init();                             /* 初期化                       */
	asm(" fset I ");                    /* 全体の割り込み許可           */
	
	jud = 0;
	while( 1 ){
		d = pushsw_get();
		if(jud == 0 && d){
			jud = 1;
		}
		
		if(jud == 1){
			if(p6 < 0xff){
				p6 = ((p6 << 1) + 1);
				timer( 1000 );
			}
		}else{
			if(p6 > 0x00){
				p6 = (p6 >> 1);
				timer( 1000 );
			}
		}
		
		if(p6 >= 0xff && d){
			jud = 2;
		}else if(p6 <= 0x00 && d){
			jud = 1;
		}
	}
}

/************************************************************************/
/* R8C/35A スペシャルファンクションレジスタ(SFR)の初期化                */
/************************************************************************/
void init( void )
{
    int i;

    /* クロックをXINクロック(20MHz)に変更 */
    prc0  = 1;                          /* プロテクト解除               */
    cm13  = 1;                          /* P4_6,P4_7をXIN-XOUT端子にする*/
    cm05  = 0;                          /* XINクロック発振              */
    for(i=0; i<50; i++ );               /* 安定するまで少し待つ(約10ms) */
    ocd2  = 0;                          /* システムクロックをXINにする  */
    prc0  = 0;                          /* プロテクトON                 */

    /* ポートの入出力設定 */
    prc2 = 1;                           /* PD0のプロテクト解除          */
    pd0 = 0xe0;                         /* 7-5:LED 4:MicroSW 3-0:Sensor */
    p1  = 0x0f;                         /* 3-0:LEDは消灯                */
    pd1 = 0xdf;                         /* 5:RXD0 4:TXD0 3-0:LED        */
    pd2 = 0xfe;                         /* 0:PushSW                     */
    pd3 = 0xfb;                         /* 4:Buzzer 2:IR                */
    pd4 = 0x83;                         /* 7:XOUT 6:XIN 5-3:DIP SW 2:VREF*/
    pd5 = 0x40;                         /* 7:DIP SW                     */
    pd6 = 0xff;                         /* LEDなど出力                  */
}

/************************************************************************/
/* プッシュスイッチ値読み込み                                           */
/* 戻り値 プッシュスイッチの値 0:OFF 1:ON                               */
/************************************************************************/
unsigned char pushsw_get( void )
{
    unsigned char sw;

    sw  = ~p2;                          /* プッシュスイッチ読み込み     */
    sw &= 0x01;                         /* 不要ビットを"0"にする        */

    return  sw;
}

/************************************************************************/
/* タイマ本体                                                           */
/* 引数　 タイマ値 1=1ms                                                */
/************************************************************************/
void timer( unsigned long timer_set )
{
    int i;

    do {
        for( i=0; i<1240; i++ );
    } while( timer_set-- );
}

/************************************************************************/
/* end of file                                                          */
/************************************************************************/
