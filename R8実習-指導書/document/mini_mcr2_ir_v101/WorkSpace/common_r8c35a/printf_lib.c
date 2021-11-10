/****************************************************************************/
/* 対象マイコン R8C/35A                                                     */
/* ﾌｧｲﾙ内容     printf,scanf関連処理                                        */
/* バージョン   Ver.1.20                                                    */
/* Date         2010.04.19                                                  */
/* Copyright    ルネサスマイコンカーラリー事務局                            */
/*              日立インターメディックス株式会社                            */
/****************************************************************************/

/*======================================*/
/* インクルード                         */
/*======================================*/
#include    <stdio.h>
#include    "sfr_r835a.h"               /* R8C/35A SFRの定義ファイル    */
#include    "printf_lib.h"              /* printf関連処理               */

/*======================================*/
/* シンボル定義                         */
/*======================================*/
#define         SEND_BUFF_SIZE  64      /* 送信バッファサイズ           */
#define         RECV_BUFF_SIZE  32      /* 受信バッファサイズ           */

/*======================================*/
/* グローバル変数の宣言                 */
/*======================================*/
/* 送信バッファ */
static char            send_buff[SEND_BUFF_SIZE];
static char            *send_w = send_buff;
static char            *send_r = send_buff;
static int             send_count = 0;

/* 受信バッファ */
static char            recv_buff[RECV_BUFF_SIZE];
static char            *recv_w = recv_buff;
static char            *recv_r = recv_buff;

/* printf,scanf関係 */
FILE _iob[4];

/*======================================*/
/* プロトタイプ宣言                     */
/*======================================*/
int write( int s );
int read( void );
void setSendBuff( char c);
int getSendBuff( char *c );

/************************************************************************/
/* UART0の初期化、及びprintf関係をUART0に割り当て                       */
/* 引数　 通信速度                                                      */
/* 戻り値 なし                                                          */
/************************************************************************/
void init_uart0_printf( int sp )
{
    stdin->_cnt = stdout->_cnt = stdaux->_cnt = stdprn->_cnt = 0;
    stdin->_flag = _IOREAD;
    stdout->_flag = _IOWRT;
    stdaux->_flag = _IORW;
    stdprn->_flag = _IOWRT;

    stdin->_mod = _TEXT;
    stdout->_mod = _TEXT;
    stdaux->_mod = _BIN;
    stdprn->_mod = _TEXT;

    stdin->_func_in = read;
    stdout->_func_in = NULL;
    stdaux->_func_in = read;
    stdprn->_func_in = NULL;

    stdin->_func_out = NULL;
    stdout->_func_out = write;
    stdaux->_func_out = write;
    stdprn->_func_out = NULL;

    /* UART0の設定 */
    u0sr = 0x05;                        /* P14=TXD0,P15=RXD0に設定      */
    if( sp == SPEED_4800 ) {
        /* 4800bps u0brg = 2.5MHz / (4800 * 16) - 1 = 31.552 = 32 */
        u0c0 = 0x01;                    /* カウントソースなどの設定     */
        u0c1 = 0x05;                    /* 送信､受信許可                */
        u0brg = 32;                     /* 通信速度 = 4800pbs           */
    } else if( sp == SPEED_9600 ) {
        /* 9600bps u0brg = 20MHz / (9600 * 16) - 1 =129.208 = 129 */
        u0c0 = 0x00;                    /* カウントソースなどの設定     */
        u0c1 = 0x05;                    /* 送信､受信許可                */
        u0brg = 129;                    /* 通信速度 = 9600pbs           */
    } else if( sp == SPEED_19200 ) {
        /* 19200bps u0brg = 20MHz / (19200 * 16) - 1 = 64.104 = 64 */
        u0c0 = 0x00;                    /* カウントソースなどの設定     */
        u0c1 = 0x05;                    /* 送信､受信許可                */
        u0brg = 64;                     /* 通信速度 = 19200pbs          */
    } else if( sp == SPEED_38400 ) {
        /* 38400bps u0brg = 20MHz / (38400 * 16) - 1 = 31.552 = 32 */
        u0c0 = 0x00;                    /* カウントソースなどの設定     */
        u0c1 = 0x05;                    /* 送信､受信許可                */
        u0brg = 32;                     /* 通信速度 = 38400pbs          */
    }
    u0mr = 0x05;                        /* UART0 ﾃﾞｰﾀ長8bit 1ｽﾄｯﾌﾟﾋﾞｯﾄ  */
}

/************************************************************************/
/* printfで呼び出される関数                                             */
/* ユーザーからは呼び出せません                                         */
/************************************************************************/
int write( int c )
{
    char    put_data;

    if( c == '\n' )  {
        setSendBuff( '\r' );
    } else if( c == '\b' ) {
        setSendBuff( '\b' );
        setSendBuff( ' ' );
    }
    setSendBuff( c );

    if( (s0tic & 0x07) == 0 ) {
        getSendBuff( &put_data );
        put_uart0( put_data );
        s0tic = 0x07;                   /* 送信割り込み優先レベル設定   */
    }

    return 1;   // TRUE
}

/************************************************************************/
/* scanfで呼び出される関数                                              */
/* ユーザーからは呼び出せません                                         */
/************************************************************************/
int read( void )
{
    char    c;

    if( recv_r == recv_w ) {
        do {
            /* 受信待ち */
            while( get_uart0( &c ) != 1 );

            switch( c ) {
                case '\b':  /* バックスペース */
                    /* 何もバッファにないならBSは無効 */
                    if( recv_r == recv_w ) continue;
                    /* あるなら一つ戻る */
                    recv_w--;
                    break;
                case '\r':  /* Enterキー */
                    *recv_w++ = c = '\n';
                    *recv_w++ = '\r';
                    break;
                default:
                    if( recv_w >= recv_buff+RECV_BUFF_SIZE-2 ) continue;
                    *recv_w++ = c;
                    break;
            }
            /* エコーバック 入力された文字を返す */
            write( c );
        } while( c != '\n' );
    }
    c = *recv_r++;
    if( recv_r == recv_w ) recv_r = recv_w = recv_buff;

    return c;
}

/************************************************************************/
/* 送信バッファに保存                                                   */
/* 引数　 格納文字                                                      */
/* 戻り値 なし                                                          */
/* メモ   バッファがフルの場合、空くまで待ちます                        */
/************************************************************************/
void setSendBuff( char c )
{
    /* バッファが空くまで待つ */
    while( SEND_BUFF_SIZE == send_count );

    asm( "FCLR I" );                    /* 全体の割り込み禁止           */

    *send_w++ = c;
    if( send_w >= send_buff+SEND_BUFF_SIZE ) send_w = send_buff;
    send_count++;

    asm( "FSET I" );                    /* 全体の割り込み許可           */
}

/************************************************************************/
/* 送信バッファから取得                                                 */
/* 引数　 格納する文字のアドレス                                        */
/* 戻り値 0:データなし 1:データあり                                     */
/************************************************************************/
int getSendBuff( char *c )
{
    int ret = 0;

    if( send_count ) {
        asm( "FCLR I" );                /* 全体の割り込み禁止           */

        *c = *send_r++;
        if( send_r >= send_buff+SEND_BUFF_SIZE ) send_r = send_buff;
        send_count--;
        ret = 1;

        asm( "FSET I" );                /* 全体の割り込み許可           */
    }
    return ret;
}

/************************************************************************/
/* URAT0 送信割り込み                                                   */
/* 引数　 なし                                                          */
/* 戻り値 なし                                                          */
/************************************************************************/
#pragma interrupt   _uart0_trance(vect=17)
void _uart0_trance( void )
{
    char    c;
    int     ret;

    ir_s0tic = 0;

    ret = getSendBuff( &c );
    if( ret ) {
        put_uart0( c );                 /* データあるなら送信           */
    } else {
        s0tic = 0x00;                   /* データ無いなら終わり         */
    }
}

/************************************************************************/
/* １文字受信                                                           */
/* 引数　 受信文字格納アドレス                                          */
/* 戻り値 -1:受信エラー 0:受信なし 1:受信あり 文字は*sに格納            */
/************************************************************************/
int get_uart0( char *s )
{
    int ret = 0;
    int data;
    int i;

    if (ri_u0c1 == 1){                  /* 受信データあり？             */
        data = u0rb;
        *s = (char)data;
        ret = 1;
        if( data & 0xf000 ) {           /* エラーあり？                 */
            /* エラー時は再設定 */
            re_u0c1 = 0;
            for( i=0; i<50; i++ );
            re_u0c1 = 1;

            ret = -1;
        }
    }
    return ret;
}

/************************************************************************/
/* １文字出力                                                           */
/* 引数　 送信データ                                                    */
/* 戻り値 0:送信中のため、送信できず 1:送信セット完了                   */
/************************************************************************/
int put_uart0( char r )
{
    if(ti_u0c1 == 1) {                  /* 送信データなし？             */
        u0tbl = r;
        return 1;
    } else {
        /* 先に送信中(今回のデータは送信せずに終了) */
        return 0;
    }
}

/************************************************************************/
/* end of file                                                          */
/************************************************************************/
