/****************************************************************************/
/* �Ώۃ}�C�R�� R8C/35A                                                     */
/* ̧�ٓ��e     I2C�ɂ��EEP-ROM(24C256B)�̐���                             */
/* �o�[�W����   Ver.1.20                                                    */
/* Date         2010.04.19                                                  */
/* Copyright    ���l�T�X�}�C�R���J�[�����[������                            */
/*              �����C���^�[���f�B�b�N�X�������                            */
/****************************************************************************/
/*
���́FP0_7-P0_0(�f�B�b�v�X�C�b�`�Ȃ�)�A�}�C�R���{�[�h�̃f�B�b�v�X�C�b�`
�o�́FUART0

I2C�ʐM��EEP-ROM�Ƀf�[�^��ǂݏ�������T���v���v���O�����ł��B
10ms���Ƃ�5�b�ԁA�|�[�g0�̏�ԂƁA�f�B�b�v�X�C�b�`�̏�Ԃ�EEP-ROM�ɕۑ��A
���̌�AUART0�Ńp�\�R����RS-232C�֏o�͂��܂��B
*/

/*======================================*/
/* �C���N���[�h                         */
/*======================================*/
#include    <stdio.h>
#include    "sfr_r835a.h"               /* R8C/35A SFR�̒�`�t�@�C��    */
#include    "printf_lib.h"              /* printf�g�p���C�u����         */
#include    "i2c_eeprom_lib.h"          /* EEP-ROM(24C256)�g�p���C�u����*/

/*======================================*/
/* �V���{����`                         */
/*======================================*/

/*======================================*/
/* �v���g�^�C�v�錾                     */
/*======================================*/
void init( void );
unsigned char dipsw_get( void );
void convertHexToBin( unsigned char hex, char *s );

/*======================================*/
/* �O���[�o���ϐ��̐錾                 */
/*======================================*/
unsigned long   cnt_rb;                 /* �^�C�}RB�p                   */
int             pattern;                /* �p�^�[���ԍ�                 */

/* �f�[�^�ۑ��֘A */
int             iTimer10;               /* �擾�Ԋu�v�Z�p               */
int             saveIndex;              /* �ۑ��C���f�b�N�X             */
int             saveSendIndex;          /* ���M�C���f�b�N�X             */
int             saveFlag;               /* �ۑ��t���O                   */
char            saveData[8];            /* �ꎞ�ۑ��G���A               */
int             flag;

/************************************************************************/
/* ���C���v���O����                                                     */
/************************************************************************/
void main( void )
{
    char s[10];                         /* 16�i����2�i���ϊ��p          */

    /* �}�C�R���@�\�̏����� */
    init();                             /* ������                       */
    init_uart0_printf( SPEED_9600 );    /* UART0��printf�֘A�̏�����    */
    initI2CEeprom();                    /* EEP-ROM�����ݒ�              */
    asm(" fset I ");                    /* �S�̂̊��荞�݋���           */

    while( 1 ) {

    I2CEepromProcess();                 /* I2C EEP-ROM�ۑ�����          */

    switch( pattern ) {
    case 0:
        /* EEP-ROM�N���A */
        printf( "\n" );
        printf( "Data is being cleared now...\n" );
        clearI2CEeprom();               /* ���b������                   */
        printf( "\n" );
        printf( "Data recording...\n" );
        pattern = 1;
        saveIndex = 0;
        saveFlag = 1;                   /* �f�[�^�ۑ��J�n               */
        cnt_rb = 0;
        break;

    case 1:
        /* �f�[�^�ۑ����@�ۑ����̂͊��荞�݂̒��ōs�� */
        if( flag == 1 ) {
            flag = 0;
            printf( "%ld\r", cnt_rb/1000 );
        }
        if( cnt_rb >= 5000 ) {
            saveFlag = 0;               /* �ۑ��I��                 */
            pattern = 2;                /* �f�[�^�]��������         */
            cnt_rb = 0;
        }
        break;

    case 2:
        /* �^�C�g���]���A���� */
        while( !checkI2CEeprom() );     /* �Ō�̃f�[�^�������ނ܂ő҂� */
        printf( "\n" );
        printf( "i2c eeprom Data Out\n" );
        printf( "p0 data,dip sw data\n" );

        saveSendIndex = 0;
        pattern = 3;
        break;

    case 3:
        /* �f�[�^�]�� */
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
        /* �]���I�� */
        break;

    default:
        /* �ǂ�ł��Ȃ��ꍇ�͑ҋ@��Ԃɖ߂� */
        pattern = 0;
        break;
    }
    }
}

/************************************************************************/
/* R8C/35A �X�y�V�����t�@���N�V�������W�X�^(SFR)�̏�����                */
/************************************************************************/
void init( void )
{
    int i;

    /* �N���b�N��XIN�N���b�N(20MHz)�ɕύX */
    prc0  = 1;                          /* �v���e�N�g����               */
    cm13  = 1;                          /* P4_6,P4_7��XIN-XOUT�[�q�ɂ���*/
    cm05  = 0;                          /* XIN�N���b�N���U              */
    for(i=0; i<50; i++ );               /* ���肷��܂ŏ����҂�(��10ms) */
    ocd2  = 0;                          /* �V�X�e���N���b�N��XIN�ɂ���  */
    prc0  = 0;                          /* �v���e�N�gON                 */

    /* �|�[�g�̓��o�͐ݒ� */
    prc2 = 1;                           /* PD0�̃v���e�N�g����          */
    pd0 = 0xe0;                         /* 7-5:LED 4:MicroSW 3-0:Sensor */
    p1  = 0x0f;                         /* 3-0:LED�͏���                */
    pd1 = 0xdf;                         /* 5:RXD0 4:TXD0 3-0:LED        */
    pd2 = 0xfe;                         /* 0:PushSW                     */
    pd3 = 0xfb;                         /* 4:Buzzer 2:IR                */
    pd4 = 0x83;                         /* 7:XOUT 6:XIN 5-3:DIP SW 2:VREF*/
    pd5 = 0x40;                         /* 7:DIP SW                     */
    pd6 = 0xff;

    /* �^�C�}RB�̐ݒ� */
    /* ���荞�ݎ��� = 1 / 20[MHz]    * (TRBPRE+1) * (TRBPR+1)
                    = 1 / (20*10^-6) * 200        * 100
                    = 0.001[s] = 1[ms]
    */
    trbpre = 200-1;                     /* �v���X�P�[�����W�X�^         */
    trbpr  = 100-1;                     /* �v���C�}�����W�X�^           */
    trbmr  = 0x00;                      /* ���샂�[�h�A������ݒ�       */
    trbic  = 0x07;                      /* ���荞�ݗD�惌�x���ݒ�       */
    tstart_trbcr = 1;                   /* �J�E���g�J�n                 */
}

/************************************************************************/
/* �^�C�}RB ���荞�ݏ���                                                */
/************************************************************************/
#pragma interrupt _timer_rb(vect=24)
void _timer_rb( void )
{
    cnt_rb++;

    if( cnt_rb % 1000 == 0 ) flag = 1;

    /* �f�[�^�ۑ��֘A */
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
/* �f�B�b�v�X�C�b�`�l�ǂݍ���                                           */
/* �߂�l �X�C�b�`�l 0�`15                                              */
/************************************************************************/
unsigned char dipsw_get( void )
{
    unsigned char sw, sw1, sw2;

    sw1 = (p5>>4) & 0x08;               /* �f�B�b�v�X�C�b�`�ǂݍ���3    */
    sw2 = (p4>>3) & 0x07;               /* �f�B�b�v�X�C�b�`�ǂݍ���2,1,0*/
    sw = sw1 | sw2;                     /* P5��P4�̒l�����킹��         */

    return  sw;
}

/************************************************************************/
/* �P�U�i�����Q�i���ϊ�                                                 */
/* �����@ �P�U�i���f�[�^�A�ϊ���̃f�[�^�i�[�A�h���X                    */
/* �߂�l �Ȃ�                                                          */
/************************************************************************/
void convertHexToBin( unsigned char hex, char *s )
{
    int     i;

    for( i=0; i<8; i++ ) {
        if( hex & 0x80 ) {
            *s++ = '1';                 /* "1"�̂Ƃ��̕ϊ��f�[�^        */
        } else {
            *s++ = '0';                 /* "0"�̂Ƃ��̕ϊ��f�[�^        */
        }
        hex <<= 1;
    }
    *s = '\0';
}

/************************************************************************/
/* end of file                                                          */
/************************************************************************/
