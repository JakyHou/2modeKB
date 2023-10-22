#include "keyscan.h"
#include "CH58x_common.h"
#include "RingBuffer/lwrb.h"
#include "config.h"
#include "device_config.h"
#include "RF_task/rf_task.h"
#include "HAL/HAL.h"
#include "USB/usbuser.h"

#include "backlight/backlight.h"
#define my_keyboard 1  //为1时程序变为适配于丐17    0为适配沁恒87的原函数
#define ROW 5
#define COL 7
#ifdef my_keyboard
static uint8_t LED_Table_Value[20][3]={0};
const uint8_t LED_Table_Value_Temp[20][3]=
{
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

const uint8_t LED_Table[] =                    //适配于丐17+4的键码表
{
        // B3  B4  B5  B6  B7
        0,  0,  17,  13, 9, 6,3, //B18                //null       Backspace        1           4          7         fn
        0,  0,  16, 12, 8, 5, 0, //B19                //null              0         2           5          8         (
        0,  0,  15, 11, 7, 4, 2, //B20                //null              .         3           6          9         )
        0,  0,  14, 10, 0, 0, 1, //B21                //null           Enter        /           *          -         +
        0,  0,  0,  0,  0,  0, 0, //c4                 //null            NULL      NULL        NULL        NULL       NULL
};

const uint8_t keytale_8b[] =                    
{
//             R0   R1     R2   R3   R4 
//              Q   E     R     U     O
        0x00, 0x14, 0x08, 0x15, 0x18, 0x12,  //c0                
//             W    S     G     H       L
        0x00, 0x1A, 0x16, 0x0A, 0x0B, 0x0F, //c1               
//            SYM   D      T     Y     I
        0x00, 0x36, 0x07, 0x17, 0x1C, 0x0C,  //c2               
//             A     P     Rs   ENTER BACKS
        0x00, 0x04, 0x13, 0x39, 0x28, 0x2a,  //c3                
//             ALT   X     V     B     $
        0x00, 0x04, 0x1B, 0x19, 0x05, 0x2b,  //c4                
//            SPAC  Z      C      N     M
        0x00, 0x2c, 0x1D, 0x06, 0x11, 0x10,  //c5                 
//             MIC   Ls    F     J      K
        0x00, 0xfe, 0x02, 0x09, 0x0D, 0x0E, //c6        
  
        //只用到了以上
        //注意  R1-C0为左上角R2-C0为第二行第一列，以此类推   FN为key[26]

};

#elif
const uint8_t keytale_8b[] =                    //默认的键码表
{
//        R0   R1   R2   R3   R4   R5
        0x00, 0x29, 0x35, 0x2b, 0x39, 0x02, 0x01, //c0                 //esc           ~`              tab         caps-lock  shift-L      ctr-l
        0x00, 0x3a, 0x1e, 0x14, 0x04, 0x1d, 0x08, //c1                 //F1            1!              Q           A          Z            win-l
        0x00, 0x3b, 0x1f, 0x1a, 0x16, 0x1b, 0x04, //c2                 //f2            2@              W           S          X            alt-l
        0x00, 0x3c, 0x20, 0x08, 0x07, 0x06, 0x2c, //c3                 //F3            3#              E           D          C            Space
        0x00, 0x3d, 0x21, 0x15, 0x09, 0x19, 0x40, //c4                 //F4            4$              R           F          V            alt-r
        0x00, 0x3e, 0x22, 0x17, 0x0a, 0x05, 0xfe, //c5                 //F5            5%              T           G          B            Fn

        0x00, 0x3f, 0x23, 0x1c, 0x0b, 0x11, 0x10, //c6                 //F6            6^              Y           H          N
        0x00, 0x40, 0x24, 0x18, 0x0d, 0x10, 0x80, //c7                 //F7            7&              U           J          M            win-r
        0x00, 0x41, 0x25, 0x0c, 0x0e, 0x36, 0x10, //c8                 //F8            8*              I           K          <,           ctr-r
        0x00, 0x42, 0x26, 0x12, 0x0f, 0x37, 0x00, //c9                 //F9            9               O           L          >.
        0x00, 0x43, 0x27, 0x13, 0x33, 0x38, 0x00, //c10                //F10           0               P           ;:         /?
        0x00, 0x44, 0x2d, 0x2f, 0x34, 0x20, 0x00, //c11                //F11           -_              [{          '"         shift-r
        0x00, 0x45, 0x2e, 0x30, 0x31, 0x00, 0x00, //c12                //F12           =+              ]}          \|

        0x00, 0x00, 0x2a, 0x00, 0x28, 0x00, 0x00, //c13                //0x00          Backspace                   Enter-R
        0x00, 0x46, 0x49, 0x4c, 0x00, 0x00, 0x50, //c14                //Print-screen  Insert          Delete      0x00,       0x00,       左
        0x00, 0x47, 0x4a, 0x4d, 0x00, 0x52, 0x51, //c15                //Scroll-Lock   Home            End         0x00,       上                           下
        0x00, 0x48, 0x4b, 0x4e, 0x00, 0x00, 0x4f, //c16                //Pause         Page-Up         Page-Down   0x00        0x00        右
        0x00, 0x00, 0x53, 0x5f, 0x5c, 0x59, 0x00, //c17                //Backlight     Num-lock        7HOME       4(小键盘)   1End       0x00
        0x00, 0x00, 0x54, 0x60, 0x5d, 0x5a, 0x62, //c18                //Locking       /               8(小键盘)   5(小键盘)   2(小键盘)   0Ins
        0x00, 0x00, 0x55, 0x61, 0x5e, 0x5b, 0x63, //c19                //0x00          *               9Pgup       6(小键盘)   3PgDn       =del
        0x00, 0x00, 0x56, 0x57, 0x00, 0x00, 0x58 //c20                 //0x00          -               +           0x00        0x00        Enter-R2
};


#endif


void index2keyVal_8(uint8_t *index, uint8_t *keyVal, uint8_t len);
void index2keyVal_16(uint8_t *index, uint8_t *keyVal, uint8_t len);

#ifdef my_keyboard
void keyInit(void) //适配丐17   用了四列六行  1-4列对应PB4-PB7  1-6列对应PA0-PA5  二极管行指向列   拉低列依次读行
{
    GPIOB_ModeCfg(
        GPIO_Pin_0| GPIO_Pin_2 | GPIO_Pin_18 | GPIO_Pin_6 | GPIO_Pin_8,  //这个顺序是从上到下
        GPIO_ModeIN_PU );//行
    GPIOB_ModeCfg(
        GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_9 | GPIO_Pin_16 | GPIO_Pin_17,GPIO_ModeOut_PP_5mA);//列

    // GPIOB_ModeCfg( GPIO_Pin_22, GPIO_ModeIN_PU);//BOOT
    // GPIOA_ModeCfg( GPIO_Pin_14, GPIO_ModeOut_PP_20mA);//RGB
    // GPIOA_ModeCfg(GPIO_Pin_13|GPIO_Pin_15,GPIO_ModeOut_PP_20mA);//LED指示灯
        // GPIOA_ResetBits(GPIO_Pin_13);
        // GPIOA_ResetBits(GPIO_Pin_15);

        if(device_mode == MODE_BLE){
                GPIOA_SetBits(GPIO_Pin_13);
            }

            else if(device_mode == MODE_RF24){
                GPIOA_SetBits(GPIO_Pin_15);
            }

            else if(device_mode == MODE_USB){
                GPIOA_SetBits(GPIO_Pin_15);
                GPIOA_SetBits(GPIO_Pin_13);
            }
            else {
                GPIOA_ResetBits(GPIO_Pin_15);
                GPIOA_ResetBits(GPIO_Pin_13);
            }
}


const uint32_t IOmap[] = {1<<1, 1<<3, 1<<5, 1<<7, 1<<9, 1<<16, 1<<17};  //这个顺序是从左到右

void RstAllPins(void)
{
    for(uint8_t i = 0; i < COL; i++){
          GPIOB_ResetBits(IOmap[i]);
    }
}

#elif
void keyInit(void)
{
    GPIOA_ModeCfg(
    GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5,
            GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeOut_PP_5mA);
    GPIOB_ModeCfg(
             GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 |
            GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_16 | GPIO_Pin_17 | GPIO_Pin_18 |
            GPIO_Pin_19 | GPIO_Pin_20 | GPIO_Pin_21 | GPIO_Pin_22 | GPIO_Pin_23
            , GPIO_ModeOut_PP_5mA);
}

const uint32_t IOmap[] = {1<<4, 1<<5, 1<<6, 1<<7, 1<<14, 1<<15, 1<<16, 1<<17, 1<<8, 1<<9, 1<<8, 1<<18, 1<<19, 1<<20, 1<<21, 1<<22, 1<<23};
                             //数组排序x为CLx第几列的扫描，左移多少位就是第几个IO口
                             //该扫描逻辑为拉低某列依次读行,读到低电平为该行该列被摁下
void RstAllPins(void)
{
    for(uint8_t i = 0; i < 17; i++){
          i == 10 ? GPIOA_ResetBits(IOmap[i]): GPIOB_ResetBits(IOmap[i]);
    }
}
#endif


#ifdef my_keyboard
__HIGH_CODE
void keyScan(uint8_t *pbuf, uint8_t *key_num)//fristbuf 为这次扫描的按键情况   是配于丐17版本
{
    static uint8_t temp_colour = 0;
    uint8_t r,g,b;
    uint8_t i = 0;
    uint8_t KeyNum;
    uint8_t Sataion;
    static uint8_t secbuf[120];//secbuf 为上一次扫描的按键情况

    uint8_t firstbuf[120] = { 0 };  //每一次扫描 firstbuf复位为0
    if(temp_colour<200)
    {
        temp_colour ++;
        if(temp_colour%3 == 0)
        {
            r=255;
            g=0;
            b=0;
        }
        else if(temp_colour%3 == 1)
        {
            r=0;
            g=255;
            b=0;
        }
        else {
            r=0;
            g=0;
            b=255;
        }
    }
    else
    {
        temp_colour = 0;
    }
    KeyNum = 0;

    for( i = 0; i < COL; i++){ //4次列扫描，每次5个行按键，从左到右，从上到下
        GPIOB_ResetBits(IOmap[i]);//下拉列，读行
        __nop();__nop();  //由于上拉输入拉低需要一定的时间，所以必须延时一段时间再读IO
        {
            if (Key_S0 == 0) {
                Sataion = i * 6 + 1;  //firstbuf[KeyNum++]括号内KeyNum先调用后++,且是该键摁下才会++
                firstbuf[KeyNum++] = Sataion;
                // PRINT("Key_S0.\n");
            }
            if (Key_S1 == 0) {
                Sataion = i * 6 + 2;
                firstbuf[KeyNum++] = Sataion;
            }
            if (Key_S2 == 0) {
                Sataion = i * 6 + 3;
                firstbuf[KeyNum++] = Sataion;
            }
            if (Key_S3 == 0) {
                Sataion = i * 6 + 4;
                firstbuf[KeyNum++] = Sataion;
            }
            if (Key_S4 == 0) {
                Sataion = i * 6 + 5;
                firstbuf[KeyNum++] = Sataion;
            }
            // if (Key_S5 == 0) {
            //     Sataion = i * 7 + 6;
            //    firstbuf[KeyNum++] = i * 7 + 6;
            // }
        }
        GPIOB_SetBits(IOmap[i]);//与前面拉低IO相对，拉高IO

        while(!(Key_S0 && Key_S1 && Key_S2 && Key_S3 && Key_S4 )) {  //等待列IO被完全上拉，实际上没有continue应该也一样
            continue;
        }
    }
    //完全扫描完，生成一个firstbuf[120]数组，前102个元素键拥有键值（firstbuf[0]~firstbuf[101]）
    //若对应键摁下则元素值为firstbuf[0]=1顺序接下去但是跳过七的倍数，没有摁下则该键值为0

    //这一次与上一次键值相等 去抖动作用
    if (tmos_memcmp(firstbuf, secbuf, sizeof(firstbuf)) == true)
            {
        tmos_memcpy(pbuf, secbuf, sizeof(firstbuf));
        *key_num = KeyNum;//在完全扫描完按键后，这个值应该是固定的24
        LED_Table_Value[LED_Table[Sataion]-1][0]=r;
        LED_Table_Value[LED_Table[Sataion]-1][1]=g;
        LED_Table_Value[LED_Table[Sataion]-1][2]=b;
    }

    tmos_memcpy(secbuf, firstbuf, sizeof(firstbuf));
    KEY_led_Show();
}
#elif
__HIGH_CODE
void keyScan(uint8_t *pbuf, uint8_t *key_num)//fristbuf 为这次扫描的按键情况
{
    uint8_t KeyNum;
    static uint8_t secbuf[120];//secbuf 为上一次扫描的按键情况

    uint8_t firstbuf[120] = { 0 };  //每一次扫描 firstbuf复位为0

    KeyNum = 0;

    for(uint8_t i = 0; i < 17; i++){ //17次列扫描，每次6个行按键，从左到右，从上到下
        i == 10 ? GPIOA_ResetBits(IOmap[i]): GPIOB_ResetBits(IOmap[i]);//PA8是唯一一个PA口做列扫描的，故可以有这一行
        __nop();__nop();  //由于上拉输入拉低需要一定的时间，所以必须延时一段时间再读IO
        {
            if (Key_S0 == 0) {
                firstbuf[KeyNum++] = i * 7 + 1;  //firstbuf[KeyNum++]括号内先调用后++
            }
            if (Key_S1 == 0) {
                firstbuf[KeyNum++] = i * 7 + 2;
            }
            if (Key_S2 == 0) {
                firstbuf[KeyNum++] = i * 7 + 3;
            }
            if (Key_S3 == 0) {
                firstbuf[KeyNum++] = i * 7 + 4;
            }
            if (Key_S4 == 0) {
                firstbuf[KeyNum++] = i * 7 + 5;
            }
            if (Key_S5 == 0) {
                firstbuf[KeyNum++] = i * 7 + 6;
            }
        }
        i == 10 ? GPIOA_SetBits(IOmap[i]): GPIOB_SetBits(IOmap[i]);//与前面拉低IO相对，拉高IO

        while(!(Key_S0 && Key_S1 && Key_S2 && Key_S3 && Key_S4 && Key_S5)) {  //等待列IO被完全上拉，实际上没有continue应该也一样
            continue;
        }
    }
    //完全扫描完，生成一个firstbuf[120]数组，前102个元素键拥有键值（firstbuf[0]~firstbuf[101]）
    //若对应键摁下则元素值为firstbuf[0]=1顺序接下去但是跳过七的倍数，没有摁下则该键值为0

    //这一次与上一次键值相等 去抖动作用
    if (tmos_memcmp(firstbuf, secbuf, sizeof(firstbuf)) == true)
            {
        tmos_memcpy(pbuf, secbuf, sizeof(firstbuf));
        *key_num = KeyNum;//在完全扫描完按键后，这个值应该是固定的102
    }

    tmos_memcpy(secbuf, firstbuf, sizeof(firstbuf));

}
#endif


unsigned char flag_fn=0;
void index2keyVal_8(uint8_t *index, uint8_t *keyVal, uint8_t len)
{
    for (int b = 0, idx = 0; b < len; b++) {
        if(!keytale_8b[index[b]]) continue;
        keyVal[2 + idx++] = keytale_8b[index[b]];

        if(keytale_8b[index[b]]==0xfe){//BOOT被摁下
                    flag_fn = true;
                //    while(!GPIOB_ReadPortPin(GPIO_Pin_22));
                }
    }
}

void index2keyVal_16(uint8_t *index, uint8_t *keyVal, uint8_t len)
{

    for (int b = 0; b < len; b++) {
        if (!(index[b] && keytale_8b[index[b]]))
            continue;

        uint8_t general_code = keytale_8b[index[b]] - 4;
        uint8_t code_num = general_code / 8 + 1;
        uint8_t keyval = 1 << (general_code % 8);
//         LOG_INFO("index=%d gc=%#x, cn=%d, kv=%#x ",index[b], general_code, code_num, keyval);

        keyVal[code_num] |= keyval;
    }
}


#ifdef my_keyboard
bool hotkeydeal(uint8_t *index, uint8_t *keys, uint8_t nums)//index对应键值的指针位置而不是键值
{
    bool ret = false;
    for (int i = 0; i < nums; i++) {
       if (index[i] == 0X26)             //shift-l
               {
           keys[0] |= keytale_8b[index[i]];
           index[i] = 0;
           ret = true;
       }
//        if (index[i] == 82)             //shift-r
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 6)             //ctrl-l
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 13)             //winl
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 20)             //altl
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 34)             //altr
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 55)             //winr
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }
//        if (index[i] == 62)             //ctrr
//                {
//            keys[0] |= keytale_8b[index[i]];
//            index[i] = 0;
//            ret = true;
//        }

        // if (index[i] == 0X25)             //Fn
        //         {
        //     keys[0] = keytale_8b[index[i]];
        //     index[i] = 0;
        // }
    }

    return ret;

}

#elif
bool hotkeydeal(uint8_t *index, uint8_t *keys, uint8_t nums)//index对应键值的指针位置而不是键值
{
    bool ret = false;
    for (int i = 0; i < nums; i++) {
        if (index[i] == 5)             //shift-l
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 82)             //shift-r
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 6)             //ctrl-l
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 13)             //winl
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 20)             //altl
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 34)             //altr
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 55)             //winr
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }
        if (index[i] == 62)             //ctrr
                {
            keys[0] |= keytale_8b[index[i]];
            index[i] = 0;
            ret = true;
        }

        if (index[i] == 41)             //Fn
                {
            keys[0] = keytale_8b[index[i]];
            index[i] = 0;
        }
    }

    return ret;

}
#endif

uint8_t Volume_Decre_Flag;
uint8_t Volume_Incre_Flag;
uint8_t Volume_Mute_Flag;

uint8_t LED_Enhance_flag;
uint8_t LED_Weaken_flag;
uint8_t LED_Switch_flag;

uint8_t Fn_1_Flag;
uint8_t Fn_2_Flag;
uint8_t Fn_3_Flag;
uint8_t Fn_4_Flag;

#ifdef my_keyboard
bool SpecialKey(uint8_t *keyval)             //Fn键
{
    if ( flag_fn == 1 ) {
        switch ( keyval[2] ) {
        case 0x60:      //FN+8  开始2.4G搜索
            PRINT("FN+8+\n");
            if (device_mode == MODE_RF24) {
                PRINT("Pair function enable.\n");
                OnBoard_SendMsg(RFtaskID, RF_PAIR_MESSAGE, 1, NULL);
            }
            flag_fn=0;
            break;

        case 0X05:      //FN+*   切换蓝牙模式
            PRINT("FN+Print-screen\n");
            if (device_mode == MODE_BLE) {
                break;
            }
            device_mode = MODE_BLE;
            SaveDeviceInfo("device_mode");
            flag_fn=0;

            SYS_ResetExecute();   //软复位

            break;

        case 0x56:      //FN+-  切换2.4G模式
            PRINT("FN+Scroll-Lock\n");
            if (device_mode == MODE_RF24) {
                break;
            }
            flag_fn=0;

            device_mode = MODE_RF24;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位

            break;

        case 0X04:      //FN+/    切换有线模式
            PRINT("FN+Pause\n");

            if (device_mode == MODE_USB) {
                break;
            }
            flag_fn=0;

            device_mode = MODE_USB;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位

            break;

        case 0x59://FN+1   蓝牙一号设备搜索
            PRINT("Fn+1\n");
            if (device_bond.ID_Num == 0) {
                break;
            } else {
                device_bond.ID_Num = 0;
                device_bond.ID[0].Direct_flag = 1;
                SaveDeviceInfo("device_bond");
                flag_fn=0;

                mDelaymS(10);
                SYS_ResetExecute();   //软复位

            }

            break;
        case 0x5a://FN+2   蓝牙二号设备搜索
            PRINT("Fn+2\n");
            if (device_bond.ID_Num == 1) {
                break;
            } else {
                device_bond.ID_Num = 1;

                device_bond.ID[1].Direct_flag = 1;
                SaveDeviceInfo("device_bond");
                flag_fn=0;

                mDelaymS(10);
                SYS_ResetExecute();   //软复位
            }
            break;

        case 0x5b://FN+3   蓝牙三号设备搜索
            PRINT("Fn+3\n");
            if (device_bond.ID_Num == 2) {
                break;
            } else {
                device_bond.ID_Num = 2;
                device_bond.ID[2].Direct_flag = 1;
                SaveDeviceInfo("device_bond");
                flag_fn=0;

                mDelaymS(10);
                SYS_ResetExecute();   //软复位

            }
            break;

        case 0x5c://FN+4   蓝牙一号设备重置
            PRINT("Fn+4\n");
            device_bond.ID_Num = 0;
            device_bond.ID[0].Direct_flag = 0;
            SaveDeviceInfo("device_bond");
            flag_fn=0;

            mDelaymS(10);
            SYS_ResetExecute();   //软复位

            break;

        case 0x57://FN+ +   蓝牙四号设备重置
            PRINT("Fn+ +\n");
            device_bond.ID_Num = 3;
            device_bond.ID[3].Direct_flag = 0;
            SaveDeviceInfo("device_bond");
            flag_fn=0;

            mDelaymS(10);
            SYS_ResetExecute();   //软复位

            break;

        case 0x58://FN+enter   蓝牙四号设备搜索
            PRINT("Fn+enter\n");
            if (device_bond.ID_Num == 3) {
                break;
            } else {
                device_bond.ID_Num = 3;
                device_bond.ID[3].Direct_flag = 1;
                SaveDeviceInfo("device_bond");
                flag_fn=0;

                mDelaymS(10);
                SYS_ResetExecute();   //软复位
            }
            break;

        case 0x5d://FN+5   蓝牙二号设备重置
            PRINT("Fn+5\n");
            device_bond.ID_Num = 1;
            device_bond.ID[1].Direct_flag = 0;
            SaveDeviceInfo("device_bond");
            flag_fn=0;

            mDelaymS(10);
            SYS_ResetExecute();   //软复位

            break;

        case 0x5e://FN+6   蓝牙三号设备重置
            PRINT("Fn+6\n");
            device_bond.ID_Num = 2;
            device_bond.ID[2].Direct_flag = 0;
            SaveDeviceInfo("device_bond");
            flag_fn=0;

            mDelaymS(10);
            SYS_ResetExecute();   //软复位
            break;

        case 0:
            PRINT("clear\n");
            Volume_Decre_Flag = 0;
            Volume_Mute_Flag = 0;
            Volume_Incre_Flag = 0;
            LED_Enhance_flag = 0;
            LED_Weaken_flag = 0;
            LED_Switch_flag = 0;
            Fn_1_Flag = 0;
            Fn_2_Flag = 0;
            Fn_3_Flag = 0;
            Fn_4_Flag = 0;
            flag_fn=0;
            break;
        default:
            break;

        }
        return true;
    }
    return false;
}

#elif
bool SpecialKey(uint8_t *keyval)             //Fn键
{
    if ( keyval[0] == 0xfe ) {
        switch ( keyval[2] ) {
        case 0x22:      //FN+5  开始2.4G搜索
            PRINT("FN+5\n");
            if (device_mode == MODE_RF24) {
                PRINT("Pair function enable.\n");
                OnBoard_SendMsg(RFtaskID, RF_PAIR_MESSAGE, 1, NULL);
            }
            break;
        case 0x27:      //FN+0   //重启
            PRINT("FN+0\n");
            if (device_mode == MODE_TSET) {  //just for test, should not use hotkey
                break;
            }
            device_mode = MODE_TSET;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位
            break;

        case 0x46:      //FN+Print-screen   切换蓝牙模式
            PRINT("FN+Print-screen\n");
            if (device_mode == MODE_BLE) {
                break;
            }
            device_mode = MODE_BLE;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位

            break;

        case 0x47:      //FN+Scroll-Lock   切换2.4G模式
            PRINT("FN+Scroll-Lock\n");
            if (device_mode == MODE_RF24) {
                break;
            }

            device_mode = MODE_RF24;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位

            break;

        case 0x48:      //FN+Pause    切换有线模式
            PRINT("FN+Pause\n");

            if (device_mode == MODE_USB) {
                break;
            }

            device_mode = MODE_USB;
            SaveDeviceInfo("device_mode");

            SYS_ResetExecute();   //软复位

            break;

        case 0x3a:
            PRINT("Fn+F1\n");
            Volume_Mute_Flag = 1;

            break;

        case 0x3b:
            PRINT("Fn+F2\n");
            Volume_Decre_Flag = 1;

            break;

        case 0x3c:
            PRINT("Fn+F3\n");
            Volume_Incre_Flag = 1;
            break;

        case 0x3d:
            PRINT("Fn+F4\n");

            break;
        case 0x3e:      //Fn+F5
            PRINT("Fn+F5\n");

            break;

        case 0x3f:      //Fn+F6
            PRINT("Fn+F6\n");
            LED_Enhance_flag = 1;
            break;

        case 0x40:      //Fn+F7
            PRINT("Fn+F7\n");
            LED_Weaken_flag = 1;
            break;
        case 0x41:
            PRINT("Fn+F8\n");
            LED_Switch_flag = 1;
            break;

        case 0x1e:
            PRINT("Fn+1\n");
            Fn_1_Flag = 1;
            break;
        case 0x1f:
            PRINT("Fn+2\n");
            Fn_2_Flag = 1;
            break;

        case 0x20:
            PRINT("Fn+3\n");
            Fn_3_Flag = 1;
            break;

        case 0x21:
            PRINT("Fn+4\n");
            Fn_4_Flag = 1;
            break;

        case 0:
            PRINT("clear\n");
            Volume_Decre_Flag = 0;
            Volume_Mute_Flag = 0;
            Volume_Incre_Flag = 0;
            LED_Enhance_flag = 0;
            LED_Weaken_flag = 0;
            LED_Switch_flag = 0;
            Fn_1_Flag = 0;
            Fn_2_Flag = 0;
            Fn_3_Flag = 0;
            Fn_4_Flag = 0;
            break;
        default:
            break;

        }
        return true;
    }
    return false;
}
#endif

bool SpecialKey_Deal(void) {
    static uint32_t Fn_1_time = 0;
    static uint32_t Fn_2_time = 0;
    static uint32_t Fn_3_time = 0;
    static uint32_t Fn_4_time = 0;

    static uint8_t flag = 0;
    bool ret = false;
    if (device_mode == MODE_BLE) {
        static UINT8 state = 0;

        if (Fn_1_Flag == 1) {
            state = 1;
            flag = 1;
            flag_fn=0;
        }
        if (Fn_2_Flag == 1) {
            state = 2;
            flag = 2;
            flag_fn=0;
        }
        if (Fn_3_Flag == 1) {
            state = 3;
            flag = 3;
            flag_fn=0;
        }
        if (Fn_4_Flag == 1) {
            state = 4;
            flag = 4;
            flag_fn=0;
        }

        if (flag == 1 && Fn_1_Flag == 0) {
            state = 5;
            flag = 0;
            flag_fn=0;
        }
        if (flag == 2 && Fn_2_Flag == 0) {
            state = 6;
            flag = 0;
            flag_fn=0;
        }
        if (flag == 3 && Fn_3_Flag == 0) {
            state = 7;
            flag = 0;
            flag_fn=0;
        }
        if (flag == 4 && Fn_4_Flag == 0) {
            state = 8;
            flag = 0;
            flag_fn=0;
        }

        LOG_INFO("special state:%d", state);

        switch (state) {
        case 0: {

        }
            break;

        case 1: {
            Fn_1_time = get_current_time();
        }
            break;

        case 2: {
            Fn_2_time = get_current_time();
        }
            break;

        case 3: {
            Fn_3_time = get_current_time();
        }
            break;

        case 4: {
            Fn_4_time = get_current_time();
        }
            break;

        case 5: {
                state = 0;
                uint32_t current_time = get_current_time() - Fn_1_time;
                if (current_time >= LONGKEY_TIME) {   //长摁进入蓝牙配对
                    Fn_1_time = 0;
                    PRINT("Long\n");
                    device_bond.ID_Num = 0;
                    device_bond.ID[0].Direct_flag = 0;
                    SaveDeviceInfo("device_bond");
                    flag_fn=0;

                    mDelaymS(10);
                    SYS_ResetExecute();   //软复位

                } else if (current_time > SHORTKEY_TIME) {  //短摁定向广播，配对之前的蓝牙
                    Fn_1_time = 0;
                    PRINT(" Short\n");
                    if (device_bond.ID_Num == 0) {
                        break;
                    } else {
                        device_bond.ID_Num = 0;
                        device_bond.ID[0].Direct_flag = 1;
                        SaveDeviceInfo("device_bond");
                        flag_fn=0;

                        mDelaymS(10);
                        SYS_ResetExecute();   //软复位

                    }
                } else{
                    PRINT("Fn+1 time error: %d\n", current_time);
                }
        }
            break;

        case 6: {
                state = 0;
                uint32_t current_time = get_current_time() - Fn_2_time;
                if (current_time >= LONGKEY_TIME) {
                    Fn_2_time = 0;
                    PRINT("Long\n");
                    device_bond.ID_Num = 1;
                    device_bond.ID[1].Direct_flag = 0;
                    SaveDeviceInfo("device_bond");
                    flag_fn=0;

                    mDelaymS(10);
                    SYS_ResetExecute();   //软复位

                } else if (current_time > SHORTKEY_TIME) {
                    Fn_2_time = 0;
                    PRINT(" Short\n");
                    if (device_bond.ID_Num == 1) {
                        break;
                    } else {
                        device_bond.ID_Num = 1;
                        device_bond.ID[1].Direct_flag = 1;
                        SaveDeviceInfo("device_bond");
                        flag_fn=0;

                        mDelaymS(10);
                        SYS_ResetExecute();   //软复位

                    }
                } else {
                    PRINT("Fn+2 time error: %d\n", current_time);
                }
        }
            break;

        case 7: {
                state = 0;
                uint32_t current_time = get_current_time() - Fn_3_time;
                if (current_time >= LONGKEY_TIME) {
                    Fn_3_time = 0;
                    PRINT("Long\n");
                    device_bond.ID_Num = 2;
                    device_bond.ID[2].Direct_flag = 0;
                    SaveDeviceInfo("device_bond");
                    flag_fn=0;

                    mDelaymS(10);
                    SYS_ResetExecute();   //软复位

                } else if (current_time > SHORTKEY_TIME) {
                    Fn_3_time = 0;
                    PRINT(" Short\n");
                    if (device_bond.ID_Num == 2) {
                        break;
                    } else {
                        device_bond.ID_Num = 2;
                        device_bond.ID[2].Direct_flag = 1;
                        SaveDeviceInfo("device_bond");
                        flag_fn=0;

                        mDelaymS(10);
                        SYS_ResetExecute();   //软复位

                    }
                } else {
                    PRINT("Fn+3 time error: %d\n", current_time);
                }
        }
            break;

        case 8: {
                state = 0;
                uint32_t current_time = get_current_time() - Fn_4_time;
                if (current_time >= LONGKEY_TIME) {
                    Fn_4_time = 0;
                    PRINT("Long\n");
                    device_bond.ID_Num = 3;
                    device_bond.ID[3].Direct_flag = 0;
                    SaveDeviceInfo("device_bond");
                    flag_fn=0;

                    mDelaymS(10);
                    SYS_ResetExecute();   //软复位

                } else if (current_time > SHORTKEY_TIME) {
                    Fn_4_time = 0;
                    PRINT(" Short\n");
                    if (device_bond.ID_Num == 3) {
                        break;
                    } else {
                        device_bond.ID_Num = 3;
                        device_bond.ID[3].Direct_flag = 1;
                        SaveDeviceInfo("device_bond");
                        flag_fn=0;

                        mDelaymS(10);
                        SYS_ResetExecute();   //软复位
                    }
                } else {
                    PRINT("Fn+4 time error: %d\n", current_time);
                }
        }
            break;

        default:
            break;

        }

    }


    {
        if(LED_Enhance_flag){
            LED_Enhance_flag = 0;
            if(enhance_bk(BK_LINEALL))
                SaveDeviceInfo("device_led");
        }

        if(LED_Weaken_flag){
            LED_Weaken_flag = 0;
            if(weaken_bk(BK_LINEALL))
                SaveDeviceInfo("device_led");
        }

        if(LED_Switch_flag){
            LED_Switch_flag = 0;
            if(device_led.led_en){
                device_led.led_en = false;
                disbale_bk(BK_LINEALL);
                SaveDeviceInfo("device_led");
            } else{
                device_led.led_en = true;
                set_bk(BK_LINEALL, device_led.led_level);
                SaveDeviceInfo("device_led");
            }
        }
    }


    {
        static UINT8 state;
        uint8_t BUF[2];
        uint8_t report_id = CONSUME_ID;
        switch (state) {
        case 0: {

            if (Volume_Incre_Flag == 1) {
                state = 1;

                PRINT("Volume_Incre\n");

                BUF[0] = 0xe9;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

            if (Volume_Decre_Flag == 1) {
                state = 2;
                PRINT("Volume_Decre\n");
                BUF[0] = 0xea;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

            if (Volume_Mute_Flag == 1) {
                state = 3;
                PRINT("Volume_Mute\n");

                BUF[0] = 0xe2;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

        }
            break;

        case 1: {
            if (Volume_Incre_Flag == 0) {
                state = 0;
                BUF[0] = 0;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

        }
            break;

        case 2: {
            if (Volume_Decre_Flag == 0) {
                state = 0;
                BUF[0] = 0;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

        }
            break;

        case 3: {
            if (Volume_Mute_Flag == 0) {
                state = 0;
                BUF[0] = 0;
                BUF[1] = 0;
                lwrb_write(&KEY_buff, &report_id, 1);
                size_t wl = lwrb_write(&KEY_buff, BUF, 2);
                if(wl != 2)
                    lwrb_skip(&KEY_buff, wl);
                ret = true;
            }

        }
            break;

        default: {
            state = 0;
        }
            break;
        }

    }
    return ret;
}

bool readKeyVal(void) {

    static uint8_t current_key_index[120] = { 0 };
    static uint8_t last_key_index[120] = { 0 };
    uint8_t key_num = 0;
    uint8_t save_key16[16] = { 0 };
    uint8_t save_key8[8] = { 0 };

    keyScan(current_key_index, &key_num);

    if (tmos_memcmp(current_key_index, last_key_index,
            sizeof(current_key_index)) == true) { //无键值变化
        return false;
    }
    tmos_memcpy(last_key_index, current_key_index, sizeof(current_key_index));

#define  KEY_MODE    8

#if KEY_MODE==8
    //hotkey deal
    //  PRINT("key=[");
    // for(int i = 0; i < 8; i++){
    //     if(i) PRINT(" ");
    //     PRINT("%#x", current_key_index[i]);
    // }PRINT("]\n");
    // PRINT("key=[");
    // for(int i = 0; i < 8; i++){
    //     if(i) PRINT(" ");
    //     PRINT("%#x", save_key8[i]);
    // }PRINT("]\n");
    // PRINT("KEY NUM=%d\n",key_num);
    hotkeydeal(current_key_index, save_key8, key_num);//这个函数过后，save_key[0]存有特殊键的按下情况，shift  ctrl  fn
                                                      //save_key应该就是之后要上报的HID数据包

    index2keyVal_8(current_key_index, save_key8, key_num);//这个函数后，让save_key[2~7]填充普通按键，由正在按下的按键决定
                                             //总结：只用修改键值表和那个以特殊键在数组中位置判断是否摁下的函数的特殊键位置，就直接能够形成小键盘了，
                                             //电路拓扑直接只采用前四列六行就行了
    PRINT("key=[");
    for(int i = 0; i < 8; i++){
        if(i) PRINT(" ");
        PRINT("%#x", save_key8[i]);
    }PRINT("]\n");

    static bool isFnpress = false;



    if(SpecialKey(save_key8)){  //FN键后一个normal键一定是释放键
        isFnpress = true;
        return SpecialKey_Deal();
    } else{
        if(isFnpress) {
            isFnpress = false;
            return false;  // no keys change
        }
    }

    if(device_mode == MODE_BLE){
        GPIOA_ResetBits(GPIO_Pin_13);
//        GPIOA_SetBit14);
        GPIOA_SetBits(GPIO_Pin_15);
    }

    else if(device_mode == MODE_RF24){
        GPIOA_SetBits(GPIO_Pin_13);
//        GPIOB_ResetBits(GPIO_Pin_14);
        GPIOA_ResetBits(GPIO_Pin_15);
    }

    else if(device_mode == MODE_USB){
        GPIOA_SetBits(GPIO_Pin_13);
//        GPIOB_SetBits(GPIO_Pin_14);
        GPIOA_SetBits(GPIO_Pin_15);
    }
    else {
        GPIOB_SetBits(GPIO_Pin_13);
//        GPIOB_SetBits(GPIO_Pin_14);
        GPIOB_SetBits(GPIO_Pin_15);
    }


    uint8_t report_id = KEYNORMAL_ID;
    lwrb_write(&KEY_buff, &report_id, 1);
    size_t wl = lwrb_write(&KEY_buff, save_key8, 8);
    if(wl != 8)
        lwrb_skip(&KEY_buff, wl);


#elif KEY_MODE==16
    index2keyVal_16(current_key_index, save_key16, key_num);
    uint8_t report_id = KEYBIT_ID;
    lwrb_write(&KEY_buff, &report_id, 1);
    size_t wl = lwrb_write(&KEY_buff, save_key16, 16);
    if(wl != 16)
        lwrb_skip(&KEY_buff, wl);
#endif

    return true;
}

uint8_t RGB_Value_G = 1;
uint8_t RGB_Value_R = 0;
uint8_t RGB_Value_B = 0;

void ws2812b_rst(void)
{
    GPIOA_SetBits(GPIO_Pin_14);
    GPIOA_ResetBits(GPIO_Pin_14);
    mDelayuS(400);          //50us
}

void ws2812b_writebyte_byt(unsigned char dat)
{
    unsigned char i;
    for(i=0;i<8;i++)
    {
        if(dat & 0x80)
        {
            GPIOA_SetBits(GPIO_Pin_14);
            GPIOA_SetBits(GPIO_Pin_14);
            GPIOA_ResetBits(GPIO_Pin_14);
        }
        else
        {
            GPIOA_SetBits(GPIO_Pin_14);
            GPIOA_ResetBits(GPIO_Pin_14);
            GPIOA_ResetBits(GPIO_Pin_14);
        }
        dat = (dat << 1);
    }
}

void ws2812b_write_rgb_byte(VOID)
{
    ws2812b_writebyte_byt(RGB_Value_G);
    ws2812b_writebyte_byt(RGB_Value_R);
    ws2812b_writebyte_byt(RGB_Value_B);
}


void KEY_led_Show(void)
{
//    static uint8_t RGB_Value_G_Old = 0;
//    static uint8_t RGB_Value_R_Old = 0;
//    static uint8_t RGB_Value_B_Old = 0;
    uint8_t i = 0;
//
    static uint8_t reset = false;
    if (tmos_memcmp(LED_Table_Value, LED_Table_Value_Temp, 60) == false)
    {
        for(i=0;i<20;i++)
        {
            ws2812b_writebyte_byt(LED_Table_Value[i][0]);
            ws2812b_writebyte_byt(LED_Table_Value[i][1]);
            ws2812b_writebyte_byt(LED_Table_Value[i][2]);
            if(LED_Table_Value[i][0]>5) LED_Table_Value[i][0]-=5;
            else  LED_Table_Value[i][0] = 0;
            if(LED_Table_Value[i][1]>5) LED_Table_Value[i][1]-=5;
            else  LED_Table_Value[i][1] = 0;
            if(LED_Table_Value[i][2]>5) LED_Table_Value[i][2]-=5;
            else  LED_Table_Value[i][2] = 0;
        }
        ws2812b_rst();
        reset = true;
    }
    else
    {
        if(reset == true)
        {
            for(i=0;i<20;i++)
            {
                ws2812b_writebyte_byt(0);
                ws2812b_writebyte_byt(0);
                ws2812b_writebyte_byt(0);
            }
            reset = false;
        }
    }

}
