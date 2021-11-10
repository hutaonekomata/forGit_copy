/****************************************************************************/
/* 対象マイコン R8C/35A                                                     */
/* ﾌｧｲﾙ内容     I2CによるEEP-ROM(24C256B)の制御                             */
/* バージョン   Ver.1.20                                                    */
/* Date         2010.04.19                                                  */
/* Copyright    ルネサスマイコンカーラリー事務局                            */
/*              日立インターメディックス株式会社                            */
/****************************************************************************/
/*
入力：P0_7-P0_0(ディップスイッチなど)、マイコンボードのディップスイッチ
出力：UART0

I2C通信でEEP-ROMにデータを読み書きするサンプルプログラムです。
10msごとに5秒間、ポート0の状態と、ディップスイッチの状態をEEP-ROMに保存、
その後、UART0でパソコンのRS-232Cへ出力します。
*/

/*======================================*/
/* インクルード                         */
/*======================================*/
#include    <stdio.h>
#include    "sfr_r835a.h"               /* R8C/35A SFRの定義ファイル    */
#include    "printf_lib.h"              /* printf使用ライブラリ         */
#include    "i2c_eeprom_lib.h"          /* EEP-ROM(24C256)使用ライブラリ*/

/*======================================*/
/* シンボル定義                         */
/*======================================*/

/*======================================*/
/* プロトタイプ宣言                     */
/*======================================*/
void init( void );
unsigned char dipsw_get( void );
void convertHexToBin( unsigned char hex, char *s );

/*======================================*/
/* グローバル変数の宣言                 */
/*======================================*/
unsigned long   cnt_rb;                 /* タイマRB用                   */
int             pattern;                /* パターン番号                 */

/* データ保存関連 */
int             iTimer10;               /* 取得間隔計算用               */
int             saveIndex;              /* 保存インデックス             */
int             saveSendIndex;          /* 送信インデックス             */
int             saveFlag;               /* 保存フラグ                   */
char            saveData[8];            /* 一時保存エリア               */
int             flag;

/************************************************************************/
/* メインプログラム                                                     */
/************************************************************************/
void main( void )
{
    char s[10];                         /* 16進数→2進数変換用          */

    /* マイコン機能の初期化 */
    init();                             /* 初期化                       */
    init_uart0_printf( SPEED_9600 );    /* UART0とprintf関連の初期化    */
    initI2CEeprom();                    /* EEP-ROM初期設定              */
    asm(" fset I ");                    /* 全体の割り込み許可           */

    while( 1 ) {

    I2CEepromProcess();                 /* I2C EEP-ROM保存処理          */

    switch( pattern ) {
    case 0:
        /* EEP-ROMクリア */
        printf( "\n" );
        printf( "Data is being cleared now...\n" );
        clearI2CEeprom();               /* 数秒かかる                   */
        printf( "\n" );
        printf( "Data recording...\n" );
        pattern = 1;
        saveIndex = 0;
        saveFlag = 1;                   /* データ保存開始               */
        cnt_rb = 0;
        break;

    case 1:
        /* データ保存中　保存自体は割り込みの中で行う */
        if( flag == 1 ) {
            flag = 0;
            printf( "%ld\r", cnt_rb/1000 );
        }
        if( cnt_rb >= 5000 ) {
            saveFlag = 0;               /* 保存終了                 */
            pattern = 2;                /* データ転送処理へ         */
            cnt_rb = 0;
        }
        break;

    case 2:
        /* タイトル転送、準備 */
        while( !checkI2CEeprom() );     /* 最後のデータ書き込むまで待つ */
        printf( "\n" );
        printf( "i2c eeprom Data Out\n" );
        printf( "p0 data,dip sw data\n" );

        saveSendIndex = 0;
        pattern = 3;
        break;

    case 3:
        /* データ転送 */
        convertHexToBin( readI2CEeprom( saveSendIndex+0 ), s );
        printf( "=\"%8s\",%02x\n", s,
                    (unsigned char)readI2CEeprom( saveSendIndex+1 ) );
        saveSendIndex += 2;
        if( saveIndex <= saveSendIndex ) {
            printf( "\nEnd!!\n" );
            pattern = 4;
            cnt_rb = 0;
        }
        break;

    case 4:
        /* 転送終了 */
        break;

    default:
        /* どれでもない場合は待機状態に戻す */
        pattern = 0;
        break;
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
    pd6 = 0xff;

    /* タイマRBの設定 */
    /* 割り込み周期 = 1 / 20[MHz]    * (TRBPRE+1) * (TRBPR+1)
                    = 1 / (20*10^-6) * 200        * 100
                    = 0.001[s] = 1[ms]
    */
    trbpre = 200-1;                     /* プリスケーラレジスタ         */
    trbpr  = 100-1;                     /* プライマリレジスタ           */
    trbmr  = 0x00;                      /* 動作モード、分周比設定       */
    trbic  = 0x07;                      /* 割り込み優先レベル設定       */
    tstart_trbcr = 1;                   /* カウント開始                 */
}

/************************************************************************/
/* タイマRB 割り込み処理                                                */
/************************************************************************/
#pragma interrupt _timer_rb(vect=24)
void _timer_rb( void )
{
    cnt_rb++;

    if( cnt_rb % 1000 == 0 ) flag = 1;

    /* データ保存関連 */
    iTimer10++;
    if( iTimer10 >= 10 ) {
        iTimer10 = 0;
        if( saveFlag ) {
            saveData[0] = p0;
            saveData[1] = dipsw_get();
            setPageWriteI2CEeprom( saveIndex, 2, saveData );
            saveIndex += 2;
            if( saveIndex >= 0x8000 ) saveFlag = 0;
        }
    }
}

/************************************************************************/
/* ディップスイッチ値読み込み                                           */
/* 戻り値 スイッチ値 0～15                                              */
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
/* １６進数→２進数変換                                                 */
/* 引数　 １６進数データ、変換後のデータ格納アドレス                    */
/* 戻り値 なし                                                          */
/************************************************************************/
void convertHexToBin( unsigned char hex, char *s )
{
    int     i;

    for( i=0; i<8; i++ ) {
        if( hex & 0x80 ) {
            *s++ = '1';                 /* "1"のときの変換データ        */
        } else {
            *s++ = '0';                 /* "0"のときの変換データ        */
        }
        hex <<= 1;
    }
    *s = '\0';
}

/************************************************************************/
/* end of file                                                          */
/************************************************************************/
