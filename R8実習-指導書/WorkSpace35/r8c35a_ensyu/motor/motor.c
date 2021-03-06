/****************************************************************************/
/* 対象マイコン R8C/35A                                                     */
/* ﾌｧｲﾙ内容     ミニマイコンカーVer.2のモータ制御                           */
/* バージョン   Ver.1.20                                                    */
/* Date         2010.04.19                                                  */
/* Copyright    ルネサスマイコンカーラリー事務局                            */
/*              日立インターメディックス株式会社                            */
/****************************************************************************/
/*
入力：マイコンボード上のディップスイッチ
出力：ミニマイコンカーVer.2の右モータ、左モータ

ミニマイコンカーVer.2の右モータ、左モータを制御します。。
マイコンボード上のディップスイッチで、スピード調整することができます。
*/

/*======================================*/
/* インクルード                         */
/*======================================*/
#include "sfr_r835a.h"                  /* R8C/35A SFRの定義ファイル    */

/*======================================*/
/* シンボル定義                         */
/*======================================*/
#define PWM_CYCLE   39999               /* モータPWMの周期              */

/*======================================*/
/* プロトタイプ宣言                     */
/*======================================*/
void init( void );
void timer( unsigned long timer_set );
void motor( int data1, int data2 );
unsigned char dipsw_get( void );
void led_out( unsigned char led );

/*======================================*/
/* グローバル変数の宣言                 */
/*======================================*/
unsigned long cnt_rb;                   /* タイマRB用                   */

/************************************************************************/
/* メインプログラム                                                     */
/************************************************************************/
void main( void )
{  int L,R,d;
    init();                             /* 初期化                       */
    asm(" fset I ");                    /* 全体の割り込み許可           */

	while(1){
		d=dipsw_get() & 0x03;

		if(d==1) R=100;
		if(d==2) R=-100;
		if(d==0||d==3) R=0;
		d=dipsw_get() & 0x0c;
		if(d==4) L= 100;
		if(d==8) L=-100;
		if(d=0||d==0x0c) L=0;
		led_out(dipsw_get());		
        	motor( L , R);
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
    pd2 = 0xfe;                         /* 7-1:モータドライブ部 0:PushSW*/
    pd3 = 0xfb;                         /* 4:Buzzer 2:IR                */
    pd4 = 0x80;                         /* 7:XOUT 6:XIN 5-3:DIP SW 2:VREF*/
    pd5 = 0x40;                         /* 7:DIP SW                     */
    pd6 = 0xff;

    /* タイマRBの設定 */
    /* 割り込み周期 = 1 / 20[MHz]   * (TRBPRE+1) * (TRBPR+1)
                    = 1 / (20*10^6) * 200        * 100
                    = 0.001[s] = 1[ms]
    */
    trbmr  = 0x00;                      /* 動作モード、分周比設定       */
    trbpre = 200-1;                     /* プリスケーラレジスタ         */
    trbpr  = 100-1;                     /* プライマリレジスタ           */
    trbic  = 0x07;                      /* 割り込み優先レベル設定       */
    trbcr  = 0x01;                      /* カウント開始                 */

    /* タイマRD リセット同期PWMモードの設定*/
    /* PWM周期 = 1 / 20[MHz]   * カウントソース * (TRDGRA0+1)
               = 1 / (20*10^6) * 8              * 40000
               = 0.016[s] = 16[ms]
    */
    trdfcr  = 0x01;                     /* リセット同期PWMモードに設定  */
    trdmr   = 0xf0;                     /* バッファレジスタ設定         */
    trdoer1 = 0xdd;                     /* 出力端子の選択               */
    trdpsr0 = 0x00;                     /* TRDIOB0,C0,D0端子設定        */
    trdpsr1 = 0x04;                     /* TRDIOA1,B1,C1,D1端子設定     */
    trdcr0  = 0x23;                     /* ソースカウントの選択:f8      */
    trdgra0 = trdgrc0 = PWM_CYCLE;      /* 周期                         */
    trdgrb0 = trdgrd0 = 0;              /* P2_2端子のON幅設定           */
//    trdgra1 = trdgrc1 = 0;              /* P2_4端子のON幅設定           */
    trdgrb1 = trdgrd1 = 0;              /* P2_5端子のON幅設定           */
    trdstr  = 0x0d;                     /* TRD0カウント開始             */
}

/************************************************************************/
/* ディップスイッチ値読み込み                                           */
/* 戻り値 スイッチ値 0〜15                                              */
/************************************************************************/
unsigned char dipsw_get( void )
{
    unsigned char sw, sw1, sw2;

    sw1 = (p5>>4) & 0x08;               /* ディップスイッチ読み込み3    */
    sw2 = (p4>>3) & 0x07;               /* ディップスイッチ読み込み2,1,0*/
    sw = sw1 | sw2;                     /* P5とP4の値を合わせる         */

    return  sw;
}
/************************************************************************/
/* マイコン部のLED出力                                                  */
/* 引数　 スイッチ値 0〜15                                              */
/************************************************************************/
void led_out( unsigned char led )
{
    unsigned char data;

    led = ~led;
    led &= 0x0f;
    data = p1 & 0xf0;
    p1 = data | led;
}


/************************************************************************/
/* タイマ本体                                                           */
/* 引数　 タイマ値 1=1ms                                                */
/************************************************************************/
void timer( unsigned long timer_set )
{
    cnt_rb = 0;
    while( cnt_rb < timer_set );
}

/************************************************************************/
/* タイマRB 割り込み処理                                                */
/************************************************************************/
#pragma interrupt intTRB(vect=24)
void intTRB( void )
{
    cnt_rb++;
}

/************************************************************************/
/* モータ速度制御                                                       */
/* 引数　 左モータ:-100〜100、右モータ:-100〜100                        */
/*        0で停止、100で正転100%、-100で逆転100%                        */
/* 戻り値 なし                                                          */
/************************************************************************/
void motor( int data1, int data2 )
{
    int    motor_r, motor_l, sw_data;

//    sw_data = dipsw_get() + 5;
//    motor_l = data1 * sw_data / 20;
//    motor_r = data2 * sw_data / 20;

    /* 左モータ制御 */
    if( motor_l >= 0 ) {
        p2_3=0;
        p2_4=1;
        trdgrd1 = (long)( PWM_CYCLE - 1 ) * motor_l / 100;
    } else {
        p2_3=1;
        p2_4=0;
        trdgrd1 = (long)( PWM_CYCLE - 1 ) * ( -motor_l ) / 100;
    }

    /* 右モータ制御 */
    if( motor_r >= 0 ) {
        p2_6=0;
        p2_7=1;
        trdgrd0 = (long)( PWM_CYCLE - 1 ) * motor_r / 100;
    } else {
        p2_6=1;
        p2_7=0;
       trdgrd0 = (long)( PWM_CYCLE - 1 ) * ( -motor_r ) / 100;
    }
}

/************************************************************************/
/* end of file                                                          */
/************************************************************************/