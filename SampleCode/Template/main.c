/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "misc_config.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "cpu_utils.h"

#include "usbh_lib.h"
#include "ff.h"
#include "diskio.h"
/*_____ D E C L A R A T I O N S ____________________________________________*/

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_REVERSE0                              (flag_PROJ_CTL.bit0)
#define FLAG_PROJ_REVERSE1                              (flag_PROJ_CTL.bit1)
#define FLAG_PROJ_REVERSE2                 				(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_REVERSE3                              (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


/*_____ D E F I N I T I O N S ______________________________________________*/

// volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

#define mainFLOP_TASK_PRIORITY                          ( tskIDLE_PRIORITY )
#define mainNORMAL_TASK_PRIORITY           	            ( tskIDLE_PRIORITY + 1UL )
#define mainUSB_TASK_PRIORITY     	                    ( tskIDLE_PRIORITY + 2UL )

#define DELAY_MS					                    (1)


#define BUFF_SIZE       (4*1024)

static UINT blen = BUFF_SIZE;
DWORD acc_size;                         /* Work register for fs command */
WORD acc_files, acc_dirs;
FILINFO Finfo;

char Line[256];                         /* Console input buffer */
#if _USE_LFN
char Lfname[512];
#endif

#ifdef __ICCARM__
#pragma data_alignment=32
BYTE Buff_Pool[BUFF_SIZE] ;       /* Working buffer */
BYTE Buff_Pool2[BUFF_SIZE] ;      /* Working buffer 2 */
#else
BYTE Buff_Pool[BUFF_SIZE] __attribute__((aligned(32)));       /* Working buffer */
BYTE Buff_Pool2[BUFF_SIZE] __attribute__((aligned(32)));      /* Working buffer 2 */
#endif

BYTE  *Buff;
BYTE  *Buff2;


#ifdef __ICCARM__
#pragma data_alignment=4
uint8_t  buff1[BUFF_SIZE] ;       /* Working buffer */
uint8_t  buff2[BUFF_SIZE] ;       /* Working buffer */
#else
uint8_t  buff1[BUFF_SIZE] __attribute__((aligned(4)));       /* Working buffer */
uint8_t  buff2[BUFF_SIZE] __attribute__((aligned(4)));       /* Working buffer */
#endif


volatile int  int_in_cnt, int_out_cnt, iso_in_cnt, iso_out_cnt;

volatile uint32_t  g_tick_cnt;
uint32_t           g_t0;


/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void TIMER1_Init(void);

// unsigned int get_systick(void)
// {
// 	return (counter_systick);
// }

// void set_systick(unsigned int t)
// {
// 	counter_systick = t;
// }

// void systick_counter(void)
// {
// 	counter_systick++;
// }

// void SysTick_Handler(void)
// {

//     systick_counter();

//     if (get_systick() >= 0xFFFFFFFF)
//     {
//         set_systick(0);      
//     }

//     // if ((get_systick() % 1000) == 0)
//     // {
       
//     // }

//     #if defined (ENABLE_TICK_EVENT)
//     TickCheckTickEvent();
//     #endif    
// }

// void SysTick_delay(unsigned int delay)
// {  
    
//     unsigned int tickstart = get_systick(); 
//     unsigned int wait = delay; 

//     while((get_systick() - tickstart) < wait) 
//     { 
//     } 

// }

// void SysTick_enable(unsigned int ticks_per_second)
// {
//     set_systick(0);
//     if (SysTick_Config(SystemCoreClock / ticks_per_second))
//     {
//         /* Setup SysTick Timer for 1 second interrupts  */
//         printf("Set system tick error!!\n");
//         while (1);
//     }

//     #if defined (ENABLE_TICK_EVENT)
//     TickInitTickEvent();
//     #endif
// }

void delay_us(int usec)
{
    /*
     *  Configure Timer1, clock source from XTL_12M. Prescale 12
     */
    /* TIMER1 clock from HXT */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_TMR1SEL_Msk)) | CLK_CLKSEL1_TMR1SEL_HXT;
    CLK->APBCLK0 |= CLK_APBCLK0_TMR1CKEN_Msk;
    TIMER1->CTL = 0;        /* disable timer */
    TIMER1->INTSTS = 0x3;   /* write 1 to clear for safty */
    TIMER1->CMP = usec;
    TIMER1->CTL = (11 << TIMER_CTL_PSC_Pos) | TIMER_ONESHOT_MODE | TIMER_CTL_CNTEN_Msk;

    while (!TIMER1->INTSTS);
}

void enable_sys_tick(int ticks_per_second)
{
    g_tick_cnt = 0;
}

uint32_t get_ticks()
{
    g_tick_cnt = xTaskGetTickCount()/10;
    return g_tick_cnt;
}

void timer_init()
{
    g_t0 = get_ticks();
}

uint32_t get_timer_value()
{
    return (get_ticks() - g_t0);
}


uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void delay_ms(uint16_t ms)
{
	#if 1
    uint32_t tickstart = get_tick();
    uint32_t wait = ms;
	uint32_t tmp = 0;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000 -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
	
	#else
	TIMER_Delay(TIMER0, 1000*ms);
	#endif
}

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

#if 0   // under cpu_utils.c
void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}
#endif

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

#if 0   // under cpu_utils.c
void vApplicationTickHook( void )
{
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()).  */

#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0 )
    {
        /* In this case the tick hook is used as part of the queue set test. */
        vQueueSetAccessQueueSetFromISR();
    }
#endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY */
}
#endif
/*-----------------------------------------------------------*/


void  dump_buff_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02x ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}


/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */

/*----------------------------------------------*/
/* Get a value of the string                    */
/*----------------------------------------------*/
/*  "123 -5   0x3ff 0b1111 0377  w "
        ^                           1st call returns 123 and next ptr
           ^                        2nd call returns -5 and next ptr
                   ^                3rd call returns 1023 and next ptr
                          ^         4th call returns 15 and next ptr
                               ^    5th call returns 255 and next ptr
                                  ^ 6th call fails and returns 0
*/

int xatoi (         /* 0:Failed, 1:Successful */
    TCHAR **str,    /* Pointer to pointer to the string */
    long *res       /* Pointer to a variable to store the value */
)
{
    unsigned long val;
    unsigned char r, s = 0;
    TCHAR c;


    *res = 0;
    while ((c = **str) == ' ') (*str)++;    /* Skip leading spaces */

    if (c == '-')       /* negative? */
    {
        s = 1;
        c = *(++(*str));
    }

    if (c == '0')
    {
        c = *(++(*str));
        switch (c)
        {
        case 'x':       /* hexadecimal */
            r = 16;
            c = *(++(*str));
            break;
        case 'b':       /* binary */
            r = 2;
            c = *(++(*str));
            break;
        default:
            if (c <= ' ') return 1; /* single zero */
            if (c < '0' || c > '9') return 0;   /* invalid char */
            r = 8;      /* octal */
        }
    }
    else
    {
        if (c < '0' || c > '9') return 0;   /* EOL or invalid char */
        r = 10;         /* decimal */
    }

    val = 0;
    while (c > ' ')
    {
        if (c >= 'a') c -= 0x20;
        c -= '0';
        if (c >= 17)
        {
            c -= 7;
            if (c <= 9) return 0;   /* invalid char */
        }
        if (c >= r) return 0;       /* invalid char for current radix */
        val = val * r + c;
        c = *(++(*str));
    }
    if (s) val = 0 - val;           /* apply sign if needed */

    *res = val;
    return 1;
}


/*----------------------------------------------*/
/* Dump a block of byte array                   */

void put_dump (
    const unsigned char* buff,  /* Pointer to the byte array to be dumped */
    unsigned long addr,         /* Heading address value */
    int cnt                     /* Number of bytes to be dumped */
)
{
    int i;


    printf("%08x ", addr);

    for (i = 0; i < cnt; i++)
        printf(" %02x", buff[i]);

    printf(" ");
    for (i = 0; i < cnt; i++)
        putchar((TCHAR)((buff[i] >= ' ' && buff[i] <= '~') ? buff[i] : '.'));

    printf("\n");
}


/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */
/*--------------------------------------------------------------------------*/

static
FRESULT scan_files (
    char* path      /* Pointer to the path name working buffer */
)
{
    DIR dirs;
    FRESULT res;
    BYTE i;
    char *fn;


    if ((res = f_opendir(&dirs, path)) == FR_OK)
    {
        i = strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0])
        {
            if (FF_FS_RPATH && Finfo.fname[0] == '.') continue;
#if _USE_LFN
            fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
            fn = Finfo.fname;
#endif
            if (Finfo.fattrib & AM_DIR)
            {
                acc_dirs++;
                *(path+i) = '/';
                strcpy(path+i+1, fn);
                res = scan_files(path);
                *(path+i) = '\0';
                if (res != FR_OK) break;
            }
            else
            {
                /*              printf("%s/%s\n", path, fn); */
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}



void put_rc (FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");
    //FRESULT i;
    uint32_t i;
    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while(*p++) ;
    }
    printf(_T("rc=%d FR_%s\n"), (UINT)rc, p);
}

/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/

void get_line (char *buff, int len)
{
    TCHAR c;
    int idx = 0;
//  DWORD dw;


    for (;;)
    {
        c = getchar();
        putchar(c);
        if (c == '\r') break;
        if ((c == '\b') && idx) idx--;
        if ((c >= ' ') && (idx < len - 1)) buff[idx++] = c;
    }
    buff[idx] = 0;

    putchar('\n');

}

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
}


static FIL file1, file2;        /* File objects */


/*
 *  Interrupt-In transfer data delivery callback function.
 */
void int_xfer_read(uint8_t *data_buff, int *data_len)
{
    //  Receive interrupt in transfer data. The data len is (*data_len).
    //  NOTICE: This callback function is in USB Host interrupt context!

    int_in_cnt++;
}


/*
 *  Interrupt-Out transfer data request callback function.
 */
void int_xfer_write(uint8_t *data_buff, int *data_len)
{
    //  LBK request the interrupt out transfer data.
    //  NOTICE: This callback function is in USB Host interrupt context!
    int_out_cnt++;
    *data_len = 8;
    memset(data_buff, int_out_cnt & 0xff, 8);
}

/*
 *  Isochronous-Out transfer data request callback function.
 */
void iso_xfer_write(uint8_t *data_buff, int data_len)
{
    //  Application feeds Isochronous-Out data here.
    //  NOTICE: This callback function is in USB Host interrupt context!
    iso_out_cnt++;
    memset(data_buff, iso_out_cnt & 0xff, data_len);
}

/*
 *  Isochronous-In transfer data request callback function.
 */
void iso_xfer_read(uint8_t *data_buff, int data_len)
{
    //  Application gets Isochronous-In data here.
    //  NOTICE: This callback function is in USB Host interrupt context!
    iso_in_cnt++;
}


void vHSUSBHTask( void *pvParameters )
{	
    char        *ptr, *ptr2;
    long        p1, p2, p3;
    BYTE        *buf;
    FATFS       *fs;              /* Pointer to file system object */
    TCHAR       usb_path[] = { '3', ':', 0 };    /* USB drive started from 3 */
    FRESULT     res;

    DIR dir;                /* Directory object */
    UINT s1, s2, cnt, sector_no;
    static const BYTE ft[] = {0, 12, 16, 32};
    DWORD ofs = 0, sect = 0;

    /* The parameters are not used. */
    ( void ) pvParameters;


    enable_sys_tick(100);
    
    printf("\n\n");
    printf("+-----------------------------------------------+\n");
    printf("|                                               |\n");
    printf("|     USB Host Mass Storage sample program      |\n");
    printf("|                                               |\n");
    printf("+-----------------------------------------------+\n");

    Buff = (BYTE *)((uint32_t)&Buff_Pool[0]);
    Buff2 = (BYTE *)((uint32_t)&Buff_Pool2[0]);

    NVIC_SetPriority(USBH_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_SetPriority(HSUSBH_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);

    usbh_core_init();
    usbh_umas_init();
    usbh_pooling_hubs();

    printf("Init time: %d\n", get_ticks());

    int_in_cnt = int_out_cnt = iso_in_cnt = iso_out_cnt = 0;

    f_chdrive(usb_path);               /* set default path                                */


    for(;;)
    {
        usbh_pooling_hubs();

        usbh_memory_used();            /* print out UsbHostLib memory usage information   */

        printf(_T(">"));
        ptr = Line;

        get_line(ptr, sizeof(Line));

        switch (*ptr++)
        {

        case '5':
            for (sector_no = 0; sector_no < 128000; sector_no++)
            {
                if (sector_no % 1000 == 0)
                    printf("\nLBK transfer count: Ii:%d, Io: %d, Si:%d, So:%d\n", int_in_cnt, int_out_cnt, iso_in_cnt, iso_out_cnt);

                printf("Test sector %d\r", sector_no);

                memset(buff1, 87, 512);
                memset(buff2, 0x87, 512);

                res = (FRESULT)disk_read(3, buff1, sector_no, 1);
                if (res)
                {
                    printf("read failed at %d, rc=%d\n", sector_no, (WORD)res);
                    put_rc(res);
                    break;
                }

                res = (FRESULT)disk_read(3, buff2, sector_no, 1);
                if (res)
                {
                    printf("read failed at %d, rc=%d\n", sector_no, (WORD)res);
                    put_rc(res);
                    break;
                }

                if (memcmp(buff1, buff2, 512) != 0)
                {
                    printf("\nData compare failed!!\n");
                    break;
                }
            }
            break;

        case '4' :
        case '6' :
        case '7' :
            ptr--;
            *(ptr+1) = ':';
            *(ptr+2) = 0;
            put_rc(f_chdrive((TCHAR *)ptr));
            break;

        case 'd' :
            switch (*ptr++)
            {
            case 'd' :  /* dd [<lba>] - Dump sector */
                if (!xatoi(&ptr, &p2)) p2 = sect;
                res = (FRESULT)disk_read(3, Buff, p2, 1);
                if (res)
                {
                    printf("rc=%d\n", (WORD)res);
                    break;
                }
                sect = p2 + 1;
                printf("Sector:%d\n", p2);
                for (buf=(unsigned char*)Buff, ofs = 0; ofs < 0x200; buf+=16, ofs+=16)
                    put_dump(buf, ofs, 16);
                break;

            case 't' :  /* dt - raw sector read/write performance test */
                printf("Raw sector read performance test...\n");
                timer_init();
                for (s1 = 0; s1 < (0x800000/BUFF_SIZE); s1++)
                {
                    s2 = BUFF_SIZE/512;     /* sector count for each read  */
                    res = (FRESULT)disk_read(3, Buff, 10000 + s1 * s2, s2);
                    if (res)
                    {
                        printf("read failed at %d, rc=%d\n", 10000 + s1 * s2, (WORD)res);
                        put_rc(res);
                        break;
                    }
                    if (s1 % 256 == 0)
                        printf("%d KB\n", (s1*BUFF_SIZE)/1024);
                }
                p1 = get_timer_value();
                printf("time = %d.%02d\n", p1/100, p1 % 100);
                printf("Raw read speed: %d KB/s\n", ((0x800000 * 100) / p1)/1024);

                printf("Raw sector write performance test...\n");
                timer_init();
                for (s1 = 0; s1 < (0x800000/BUFF_SIZE); s1++)
                {
                    s2 = BUFF_SIZE/512;     /* sector count for each read  */
                    res = (FRESULT)disk_write(3, Buff, 20000 + s1 * s2, s2);
                    if (res)
                    {
                        printf("write failed at %d, rc=%d\n", 20000 + s1 * s2, (WORD)res);
                        put_rc(res);
                        break;
                    }
                    if (s1 % 256 == 0)
                        printf("%d KB\n", (s1*BUFF_SIZE)/1024);
                }
                p1 = get_timer_value();
                printf("time = %d.%02d\n", p1/100, p1 % 100);
                printf("Raw write speed: %d KB/s\n", ((0x800000 * 100) / p1)/1024);
                break;

            case 'z' :  /* dz - file read/write performance test */
#if 0
                printf("File write performance test...\n");
                res = f_open(&file1, "3:\\tfile", FA_CREATE_ALWAYS | FA_WRITE);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                timer_init();
                for (s1 = 0; s1 < (0x800000/BUFF_SIZE); s1++)
                {
                    res = f_write(&file1, Buff, BUFF_SIZE, &cnt);
                    if (res || (cnt != BUFF_SIZE))
                    {
                        put_rc(res);
                        break;
                    }
                    if (s1 % 128 == 0)
                        printf("%d KB\n", (s1*BUFF_SIZE)/1024);
                }
                p1 = get_timer_value();
                f_close(&file1);
                printf("time = %d.%02d\n", p1/100, p1 % 100);
                printf("File write speed: %d KB/s\n", ((0x800000 * 100) / p1)/1024);
#endif
                printf("File read performance test...\n");
                res = f_open(&file1, "3:\\tfile", FA_READ);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                timer_init();
                for (s1 = 0; s1 < (0x800000/BUFF_SIZE); s1++)
                {
                    res = f_read(&file1, Buff, BUFF_SIZE, &cnt);
                    if (res || (cnt != BUFF_SIZE))
                    {
                        put_rc(res);
                        break;
                    }
                    if (s1 % 128 == 0)
                        printf("%d KB\n", (s1*BUFF_SIZE)/1024);
                }
                p1 = get_timer_value();
                f_close(&file1);
                printf("time = %d.%02d\n", p1/100, p1 % 100);
                printf("File read speed: %d KB/s\n", ((0x800000 * 100) / p1)/1024);
                break;
            }
            break;

        case 'b' :
            switch (*ptr++)
            {
            case 'd' :  /* bd <addr> - Dump R/W buffer */
                if (!xatoi(&ptr, &p1)) break;
                for (ptr=(char*)&Buff[p1], ofs = p1, cnt = 32; cnt; cnt--, ptr+=16, ofs+=16)
                    put_dump((BYTE*)ptr, ofs, 16);
                break;

            case 'e' :  /* be <addr> [<data>] ... - Edit R/W buffer */
                if (!xatoi(&ptr, &p1)) break;
                if (xatoi(&ptr, &p2))
                {
                    do
                    {
                        Buff[p1++] = (BYTE)p2;
                    }
                    while (xatoi(&ptr, &p2));
                    break;
                }
                for (;;)
                {
                    printf("%04X %02X-", (WORD)p1, Buff[p1]);
                    get_line(Line, sizeof(Line));
                    ptr = Line;
                    if (*ptr == '.') break;
                    if (*ptr < ' ')
                    {
                        p1++;
                        continue;
                    }
                    if (xatoi(&ptr, &p2))
                        Buff[p1++] = (BYTE)p2;
                    else
                        printf("???\n");
                }
                break;

            case 'r' :  /* br <sector> [<n>] - Read disk into R/W buffer */
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) p3 = 1;
                printf("rc=%d\n", disk_read(3, Buff, p2, p3));
                break;

            case 'w' :  /* bw <sector> [<n>] - Write R/W buffer into disk */
                if (!xatoi(&ptr, &p2)) break;
                if (!xatoi(&ptr, &p3)) p3 = 1;
                printf("rc=%d\n", disk_write(3, Buff, p2, p3));
                break;

            case 'f' :  /* bf <n> - Fill working buffer */
                if (!xatoi(&ptr, &p1)) break;
                memset(Buff, (int)p1, BUFF_SIZE);
                break;

            }
            break;



        case 'f' :
            switch (*ptr++)
            {

            case 's' :  /* fs - Show logical drive status */
                res = f_getfree("", (DWORD*)&p2, &fs);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("FAT type = FAT%d\nBytes/Cluster = %d\nNumber of FATs = %d\n"
                       "Root DIR entries = %d\nSectors/FAT = %d\nNumber of clusters = %d\n"
                       "FAT start (lba) = %d\nDIR start (lba,clustor) = %d\nData start (lba) = %d\n\n...",
                       ft[fs->fs_type & 3], fs->csize * 512UL, fs->n_fats,
                       fs->n_rootdir, fs->fsize, fs->n_fatent - 2,
                       fs->fatbase, fs->dirbase, fs->database
                      );
                acc_size = acc_files = acc_dirs = 0;
#if _USE_LFN
                Finfo.lfname = Lfname;
                Finfo.lfsize = sizeof(Lfname);
#endif
                res = scan_files(ptr);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("\r%d files, %d bytes.\n%d folders.\n"
                       "%d KB total disk space.\n%d KB available.\n",
                       acc_files, acc_size, acc_dirs,
                       (fs->n_fatent - 2) * (fs->csize / 2), p2 * (fs->csize / 2)
                      );
                break;
            case 'l' :  /* fl [<path>] - Directory listing */
                while (*ptr == ' ') ptr++;
                res = f_opendir(&dir, ptr);
                if (res)
                {
                    put_rc(res);
                    break;
                }
                p1 = s1 = s2 = 0;
                for(;;)
                {
                    res = f_readdir(&dir, &Finfo);
                    if ((res != FR_OK) || !Finfo.fname[0]) break;
                    if (Finfo.fattrib & AM_DIR)
                    {
                        s2++;
                    }
                    else
                    {
                        s1++;
                        p1 += Finfo.fsize;
                    }
                    printf("%c%c%c%c%c %d/%02d/%02d %02d:%02d    %9d  %s",
                           (Finfo.fattrib & AM_DIR) ? 'D' : '-',
                           (Finfo.fattrib & AM_RDO) ? 'R' : '-',
                           (Finfo.fattrib & AM_HID) ? 'H' : '-',
                           (Finfo.fattrib & AM_SYS) ? 'S' : '-',
                           (Finfo.fattrib & AM_ARC) ? 'A' : '-',
                           (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
                           (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize, Finfo.fname);
#if _USE_LFN
                    for (p2 = strlen(Finfo.fname); p2 < 14; p2++)
                        printf(" ");
                    printf("%s\n", Lfname);
#else
                    printf("\n");
#endif
                }
                printf("%4d File(s),%10d bytes total\n%4d Dir(s)", s1, p1, s2);
                if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK)
                    printf(", %10d bytes free\n", p1 * fs->csize * 512);
                break;


            case 'o' :  /* fo <mode> <file> - Open a file */
                if (!xatoi(&ptr, &p1)) break;
                while (*ptr == ' ') ptr++;
                put_rc(f_open(&file1, ptr, (BYTE)p1));
                break;

            case 'c' :  /* fc - Close a file */
                put_rc(f_close(&file1));
                break;

            case 'e' :  /* fe - Seek file pointer */
                if (!xatoi(&ptr, &p1)) break;
                res = f_lseek(&file1, p1);
                put_rc(res);
                if (res == FR_OK)
                    printf("fptr=%d(0x%lX)\n", file1.fptr, file1.fptr);
                break;

            case 'd' :  /* fd <len> - read and dump file from current fp */
                if (!xatoi(&ptr, &p1)) break;
                ofs = file1.fptr;
                while (p1)
                {
                    if ((UINT)p1 >= 16)
                    {
                        cnt = 16;
                        p1 -= 16;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_read(&file1, Buff, cnt, &cnt);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    if (!cnt) break;
                    put_dump(Buff, ofs, cnt);
                    ofs += 16;
                }
                break;

            case 'r' :  /* fr <len> - read file */
                if (!xatoi(&ptr, &p1)) break;
                p2 = 0;
                timer_init();
                while (p1)
                {
                    if ((UINT)p1 >= blen)
                    {
                        cnt = blen;
                        p1 -= blen;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_read(&file1, Buff, cnt, &s2);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    p2 += s2;
                    if (cnt != s2) break;
                }
                p1 = get_timer_value();
                if (p1)
                    printf("%d bytes read with %d kB/sec.\n", p2, ((p2 * 100) / p1)/1024);
                break;

            case 'w' :  /* fw <len> <val> - write file */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
                memset(Buff, (BYTE)p2, blen);
                p2 = 0;
                timer_init();
                while (p1)
                {
                    if ((UINT)p1 >= blen)
                    {
                        cnt = blen;
                        p1 -= blen;
                    }
                    else
                    {
                        cnt = p1;
                        p1 = 0;
                    }
                    res = f_write(&file1, Buff, cnt, &s2);
                    if (res != FR_OK)
                    {
                        put_rc(res);
                        break;
                    }
                    p2 += s2;
                    if (cnt != s2) break;
                }
                p1 = get_timer_value();
                if (p1)
                    printf("%d bytes written with %d kB/sec.\n", p2, ((p2 * 100) / p1)/1024);
                break;

            case 'n' :  /* fn <old_name> <new_name> - Change file/dir name */
                while (*ptr == ' ') ptr++;
                ptr2 = strchr(ptr, ' ');
                if (!ptr2) break;
                *ptr2++ = 0;
                while (*ptr2 == ' ') ptr2++;
                put_rc(f_rename(ptr, ptr2));
                break;

            case 'u' :  /* fu <name> - Unlink a file or dir */
                while (*ptr == ' ') ptr++;
                put_rc(f_unlink(ptr));
                break;

            case 'v' :  /* fv - Truncate file */
                put_rc(f_truncate(&file1));
                break;

            case 'k' :  /* fk <name> - Create a directory */
                while (*ptr == ' ') ptr++;
                put_rc(f_mkdir(ptr));
                break;

            case 'a' :  /* fa <atrr> <mask> <name> - Change file/dir attribute */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2)) break;
                while (*ptr == ' ') ptr++;
                put_rc(f_chmod(ptr, p1, p2));
                break;

            case 't' :  /* ft <year> <month> <day> <hour> <min> <sec> <name> - Change timestamp */
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                Finfo.fdate = (WORD)(((p1 - 1980) << 9) | ((p2 & 15) << 5) | (p3 & 31));
                if (!xatoi(&ptr, &p1) || !xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                Finfo.ftime = (WORD)(((p1 & 31) << 11) | ((p1 & 63) << 5) | ((p1 >> 1) & 31));
                put_rc(f_utime(ptr, &Finfo));
                break;

            case 'x' : /* fx <src_name> <dst_name> - Copy file */
                while (*ptr == ' ') ptr++;
                ptr2 = strchr(ptr, ' ');
                if (!ptr2) break;
                *ptr2++ = 0;
                while (*ptr2 == ' ') ptr2++;
                printf("Opening \"%s\"", ptr);
                res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_READ);
                printf("\n");
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("Creating \"%s\"", ptr2);
                res = f_open(&file2, ptr2, FA_CREATE_ALWAYS | FA_WRITE);
                putchar('\n');
                if (res)
                {
                    put_rc(res);
                    f_close(&file1);
                    break;
                }
                printf("Copying...");
                p1 = 0;
                for (;;)
                {
                    res = f_read(&file1, Buff, BUFF_SIZE, &s1);
                    if (res || s1 == 0) break;   /* error or eof */
                    res = f_write(&file2, Buff, s1, &s2);
                    p1 += s2;
                    if (res || s2 < s1) break;   /* error or disk full */

                    if ((p1 % 0x10000) == 0)
                        printf("\n%d KB copyed.", p1/1024);
                    printf(".");
                }
                printf("\n%d bytes copied.\n", p1);
                f_close(&file1);
                f_close(&file2);
                break;

            case 'y' : /* fz <src_name> <dst_name> - Compare file */
                while (*ptr == ' ') ptr++;
                ptr2 = strchr(ptr, ' ');
                if (!ptr2) break;
                *ptr2++ = 0;
                while (*ptr2 == ' ') ptr2++;
                printf("Opening \"%s\"", ptr);
                res = f_open(&file1, ptr, FA_OPEN_EXISTING | FA_READ);
                printf("\n");
                if (res)
                {
                    put_rc(res);
                    break;
                }
                printf("Opening \"%s\"", ptr2);
                res = f_open(&file2, ptr2, FA_OPEN_EXISTING | FA_READ);
                putchar('\n');
                if (res)
                {
                    put_rc(res);
                    f_close(&file1);
                    break;
                }
                printf("Compare...");
                p1 = 0;
                for (;;)
                {
                    res = f_read(&file1, Buff, BUFF_SIZE, &s1);
                    if (res || s1 == 0)
                    {
                        printf("\nRead file %s terminated. (%d)\n", ptr, res);
                        break;     /* error or eof */
                    }

                    res = f_read(&file2, Buff2, BUFF_SIZE, &s2);
                    if (res || s2 == 0)
                    {
                        printf("\nRead file %s terminated. (%d)\n", ptr2, res);
                        break;     /* error or eof */
                    }

                    p1 += s2;
                    if (res || s2 < s1) break;   /* error or disk full */

                    if (memcmp(Buff, Buff2, s1) != 0)
                    {
                        printf("Compre failed!! %d %d\n", s1,s2);
                        break;
                    }

                    if ((p1 % 0x10000) == 0)
                        printf("\n%d KB compared.", p1/1024);
                    printf(".");
                }
                if (s1 == 0)
                    printf("\nPASS. \n ");
                f_close(&file1);
                f_close(&file2);
                break;

#if _FS_RPATH
            case 'g' :  /* fg <path> - Change current directory */
                while (*ptr == ' ') ptr++;
                put_rc(f_chdir(ptr));
                break;

            case 'j' :  /* fj <drive#> - Change current drive */
                while (*ptr == ' ') ptr++;
                dump_buff_hex((uint8_t *)&p1, 16);
                put_rc(f_chdrive((TCHAR *)ptr));
                break;
#endif
#if _USE_MKFS
            case 'm' :  /* fm <partition rule> <sect/clust> - Create file system */
                if (!xatoi(&ptr, &p2) || !xatoi(&ptr, &p3)) break;
                printf("The memory card will be formatted. Are you sure? (Y/n)=");
                get_line(ptr, sizeof(Line));
                if (*ptr == 'Y')
                    put_rc(f_mkfs(0, (BYTE)p2, (WORD)p3));
                break;
#endif
            case 'z' :  /* fz [<rw size>] - Change R/W length for fr/fw/fx command */
                if (xatoi(&ptr, &p1) && p1 >= 1 && (size_t)p1 <= BUFF_SIZE)
                    blen = p1;
                printf("blen=%d\n", blen);
                break;
            }
            break;
        case '?':       /* Show usage */
            printf(
                _T("n: - Change default drive (USB drive is 3~7)\n")
                _T("dd [<lba>] - Dump sector\n")
                //_T("ds <pd#> - Show disk status\n")
                _T("\n")
                _T("bd <ofs> - Dump working buffer\n")
                _T("be <ofs> [<data>] ... - Edit working buffer\n")
                _T("br <pd#> <sect> [<num>] - Read disk into working buffer\n")
                _T("bw <pd#> <sect> [<num>] - Write working buffer into disk\n")
                _T("bf <val> - Fill working buffer\n")
                _T("\n")
                _T("fs - Show volume status\n")
                _T("fl [<path>] - Show a directory\n")
                _T("fo <mode> <file> - Open a file\n")
                _T("fc - Close the file\n")
                _T("fe <ofs> - Move fp in normal seek\n")
                //_T("fE <ofs> - Move fp in fast seek or Create link table\n")
                _T("fd <len> - Read and dump the file\n")
                _T("fr <len> - Read the file\n")
                _T("fw <len> <val> - Write to the file\n")
                _T("fn <object name> <new name> - Rename an object\n")
                _T("fu <object name> - Unlink an object\n")
                _T("fv - Truncate the file at current fp\n")
                _T("fk <dir name> - Create a directory\n")
                _T("fa <atrr> <mask> <object name> - Change object attribute\n")
                _T("ft <year> <month> <day> <hour> <min> <sec> <object name> - Change timestamp of an object\n")
                _T("fx <src file> <dst file> - Copy a file\n")
                _T("fg <path> - Change current directory\n")
                _T("fj <ld#> - Change current drive. For example: <fj 4:>\n")
                _T("fm <ld#> <rule> <cluster size> - Create file system\n")
                _T("\n")
            );
            break;
        }

    }

}

void vTask_led2( void *pvParameters )
{	
 	// static uint32_t cnt = 0;   
	uint32_t millisec = DELAY_MS*500;
	portTickType xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

    taskENTER_CRITICAL();
    printf("%s is running ...\r\n",__FUNCTION__);          
    taskEXIT_CRITICAL();

	(void) pvParameters;
	for( ;; )
	{
        taskENTER_CRITICAL();

        vTaskDelayUntil( &xLastWakeTime, ( ( portTickType ) millisec / portTICK_RATE_MS));

        #if 0 // debug
        taskENTER_CRITICAL();
        printf("vTask_led :%4d\r\n" ,cnt++);          
        taskEXIT_CRITICAL();
        #endif
		
        PH0 ^= 1;

        taskEXIT_CRITICAL();
	}  
}


void vTask_led( void *pvParameters )
{	
 	// static uint32_t cnt = 0;   
	uint32_t millisec = DELAY_MS*2000;
	portTickType xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

    taskENTER_CRITICAL();
    printf("%s is running ...\r\n",__FUNCTION__);          
    taskEXIT_CRITICAL();

	(void) pvParameters;
	for( ;; )
	{
        taskENTER_CRITICAL();

        vTaskDelayUntil( &xLastWakeTime, ( ( portTickType ) millisec / portTICK_RATE_MS));

        #if 0 // debug
        taskENTER_CRITICAL();
        printf("vTask_led :%4d\r\n" ,cnt++);          
        taskEXIT_CRITICAL();
        #endif
		
        PH2 ^= 1;

        taskEXIT_CRITICAL();
	}  
}

void vTask_logger( void *pvParameters )
{	
 	// static uint32_t cnt = 0;   
	uint32_t millisec = DELAY_MS*1000;
	portTickType xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

    taskENTER_CRITICAL();
    printf("%s is running ...\r\n",__FUNCTION__);          
    taskEXIT_CRITICAL();

	(void) pvParameters;
	for( ;; )
	{
        taskENTER_CRITICAL();

        vTaskDelayUntil( &xLastWakeTime, ( ( portTickType ) millisec / portTICK_RATE_MS));

        #if 0 // debug
        taskENTER_CRITICAL();
        printf("vTask_logger :%4d (heap:%3dbytes ,CPU:%3d)\r\n" ,cnt++,xPortGetFreeHeapSize(),osGetCPUUsage());          
        taskEXIT_CRITICAL();
        #endif
		
        PH1 ^= 1;

        taskEXIT_CRITICAL();
	}  
}


//
// check_reset_source
//
uint8_t check_reset_source(void)
{
    uint32_t src = SYS_GetResetSrc();

    if ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1)    // M48xGCAE
    {
		printf("PN : M48xGCAE\r\n");
    }
    else    // M48xIDAE
    {
		printf("PN : M48xIDAE\r\n");
    }

    SYS->RSTSTS |= 0x1FF;
    printf("Reset Source <0x%08X>\r\n", src);

    #if 1   //DEBUG , list reset source
    if (src & BIT0)
    {
        printf("0)POR Reset Flag\r\n");       
    }
    if (src & BIT1)
    {
        printf("1)NRESET Pin Reset Flag\r\n");       
    }
    if (src & BIT2)
    {
        printf("2)WDT Reset Flag\r\n");       
    }
    if (src & BIT3)
    {
        printf("3)LVR Reset Flag\r\n");       
    }
    if (src & BIT4)
    {
        printf("4)BOD Reset Flag\r\n");       
    }
    if (src & BIT5)
    {
        printf("5)System Reset Flag \r\n");       
    }
    if (src & BIT6)
    {
        printf("6)HRESET Reset Flag \r\n");       
    }
    if (src & BIT7)
    {
        printf("7)CPU Reset Flag\r\n");       
    }
    if (src & BIT8)
    {
        printf("8)CPU Lockup Reset Flag\r\n");       
    }
    #endif
    
    if (src & SYS_RSTSTS_PORF_Msk) {
        SYS_ClearResetSrc(SYS_RSTSTS_PORF_Msk);
        
        printf("power on from POR\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_PINRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_PINRF_Msk);
        
        printf("power on from nRESET pin\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_WDTRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_WDTRF_Msk);
        
        printf("power on from WDT Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_LVRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_LVRF_Msk);
        
        printf("power on from LVR Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_BODRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_BODRF_Msk);
        
        printf("power on from BOD Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_SYSRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_SYSRF_Msk);
        
        printf("power on from System Reset\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_CPURF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPURF_Msk);

        printf("power on from CPU reset\r\n");
        return FALSE;         
    }    
    else if (src & SYS_RSTSTS_CPULKRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPULKRF_Msk);
        
        printf("power on from CPU Lockup Reset\r\n");
        return FALSE;
    }   
    
    printf("power on from unhandle reset source\r\n");
    return FALSE;
}

void TMR1_IRQHandler(void)
{	
	// UBaseType_t uxSavedInterruptState;

    // FreeRTOS
	// uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{		
            // PH0 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
    
    // FreeRTOS     
    // taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

// void loop(void)
// {
// 	static uint32_t LOG1 = 0;
// 	// static uint32_t LOG2 = 0;

//     if ((get_systick() % 1000) == 0)
//     {
//         // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
//     }


// }

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{        
		printf("press : %c\r\n" , res); 

		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			// UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	// UART_SetTimeoutCnt(UART0, 20);

	// UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	// UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	// UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	// NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());    	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif	

    #if 0
    printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif

}

void GPIO_Init (void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    uint32_t volatile i;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

   CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
   CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Switch HCLK clock source to HXT */
    // CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT,CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /***********************************/
    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    /***********************************/
    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
	

    /***********************************/

    CLK_EnableModuleClock(USBH_MODULE);

    /* USB Host desired input clock is 48 MHz. Set as PLL divided by 4 (192/4 = 48) */
    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);

    /* Enable USBD and OTG clock */
    CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk | CLK_APBCLK0_OTGCKEN_Msk;

    /* Set OTG as USB Host role */
    SYS->USBPHY = SYS_USBPHY_HSUSBEN_Msk | (0x1 << SYS_USBPHY_HSUSBROLE_Pos) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk | (0x1 << SYS_USBPHY_USBROLE_Pos);
    delay_us(20);
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;


    /* USB_VBUS_EN (USB 1.1 VBUS power enable pin) multi-function pin - PB.15     */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB15MFP_Msk) | SYS_GPB_MFPH_PB15MFP_USB_VBUS_EN;

    /* USB_VBUS_ST (USB 1.1 over-current detect pin) multi-function pin - PC.14   */
    SYS->GPC_MFPH = (SYS->GPC_MFPH & ~SYS_GPC_MFPH_PC14MFP_Msk) | SYS_GPC_MFPH_PC14MFP_USB_VBUS_ST;

    /* HSUSB_VBUS_EN (USB 2.0 VBUS power enable pin) multi-function pin - PB.10   */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB10MFP_Msk) | SYS_GPB_MFPH_PB10MFP_HSUSB_VBUS_EN;

    /* HSUSB_VBUS_ST (USB 2.0 over-current detect pin) multi-function pin - PB.11 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB11MFP_Msk) | SYS_GPB_MFPH_PB11MFP_HSUSB_VBUS_ST;

    /* USB 1.1 port multi-function pin VBUS, D+, D-, and ID pins */
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk |
                       SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |= SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N |
                     SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA15MFP_USB_OTG_ID;

    PH->MODE = (PH->MODE & ~(GPIO_MODE_MODE0_Msk | GPIO_MODE_MODE1_Msk | GPIO_MODE_MODE2_Msk)) |
               (GPIO_MODE_OUTPUT << GPIO_MODE_MODE0_Pos) |
               (GPIO_MODE_OUTPUT << GPIO_MODE_MODE1_Pos) |
               (GPIO_MODE_OUTPUT << GPIO_MODE_MODE2_Pos);  // Set to output mode


    /***********************************/
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
	TIMER1_Init();
  
    check_reset_source();

    // NVIC_SetPriorityGrouping(3);

    xTaskCreate(vTask_logger    ,"logger"   ,configMINIMAL_STACK_SIZE*2  ,NULL,mainNORMAL_TASK_PRIORITY,NULL);
    xTaskCreate(vTask_led       ,"led"      ,configMINIMAL_STACK_SIZE*2  ,NULL,mainNORMAL_TASK_PRIORITY,NULL);
    xTaskCreate(vTask_led2      ,"led2"      ,configMINIMAL_STACK_SIZE*2  ,NULL,mainNORMAL_TASK_PRIORITY,NULL);

    xTaskCreate(vHSUSBHTask     ,"HSUSBH"   ,configMINIMAL_STACK_SIZE*8 , NULL,mainNORMAL_TASK_PRIORITY,NULL);


    vTaskStartScheduler();

    for( ;; );

}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
