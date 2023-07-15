# stm32 固件更新总结
## 参考链接
1. [stm32 ROM openbootloader源码](https://github.com/STMicroelectronics/stm32-mw-openbl)
2. [stm32跳转到ROM bootloader的方法](https://stm32world.com/wiki/STM32_Jump_to_System_Memory_Bootloader)

## 说明
1. 第一层文件夹：按MCU型号命名，eg: stm32h743iit6, stm32g473rct6;
2. 第二层文件夹：按固件升级类型命名, eg:

    update_by_ROM: 指通过MCU芯片ROM里面的openbootloader更新userApp，所以boot loader不需要用户自己实现，只需要在userApp里通过一定的条件跳转到ROM的bootloader就可以了。
    
    update_by_DIY：指通过自己的方式定制bootloader，所以此文件夹里的每个子类都有bootloader和application两个工程。
3. 第三层文件夹：按固件升级的接口协议命名：eg: ROM_USART, DIY_FDCAN;



