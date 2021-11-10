/*======================================*/
/* プロトタイプ宣言                     */
/*======================================*/
void initI2CEeprom( void );
void selectI2CEepromAddress( unsigned char address );
char readI2CEeprom( unsigned long address );
void writeI2CEeprom( unsigned long address, char write );
void setPageWriteI2CEeprom( unsigned long address, int count, char* data );
void I2CEepromProcess( void );
void clearI2CEeprom( void );
int checkI2CEeprom( void );
