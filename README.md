# M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage
 M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage

udpate @ 2024/08/05

1. migrate FreeRTOS with HSUSBH_USBH_MassStorage , use M487 EVB HS USB port , to test USB disc

2. there are 4 task

- task 1 : logger , with 1000 ms 

- task 2 : led1 , with 2000ms

- task 3 : led2 , with 500ms

- task 4 : HS USB host , polling


3. under KEIL option , modify stack and heap

![image](https://github.com/released/M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage/blob/main/Keil_option_Asm_stack_heap_size.jpg)

4. under driver , msc_driver.c ,

modify get_max_lun

![image](https://github.com/released/M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage/blob/main/msc_driver_get_max_lun.jpg)


modify usbh_umas_reset_disk

![image](https://github.com/released/M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage/blob/main/msc_driver_usbh_umas_reset_disk.jpg)


5. below is log capture

![image](https://github.com/released/M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage/blob/main/log.jpg)


below is USB command line test : fs

![image](https://github.com/released/M480BSP_FreeRTOS_HSUSBH_USBH_MassStorage/blob/main/log_fs.jpg)

