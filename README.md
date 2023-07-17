# stm32 固件更新总结
## 参考资料
1. [stm32 ROM openbootloader源码](https://github.com/STMicroelectronics/stm32-mw-openbl)
2. [stm32跳转到ROM bootloader的方法](https://stm32world.com/wiki/STM32_Jump_to_System_Memory_Bootloader)
3. [STM32 microcontroller system memory boot mode](https://www.st.com/resource/en/application_note/an2606-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf)
4. 请一定要在相应MCU的官方FW固件包中找一找，是否有相似的IAP或者bootloader例程，比如`STM32Cube_FW_H7_V1.11.0\Projects\STM32H743I-EVAL\Applications\IAP`
5. 目标`MCU`的`Reference manual`，比如[RM0433: stm32h743xx Reference manual](https://www.st.com/resource/en/reference_manual/dm00314099-stm32h742-stm32h743-753-and-stm32h750-value-line-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)

## 说明
1. 第一层文件夹：按`MCU型号`命名，eg: stm32h743iit6, stm32g473rct6;
2. 第二层文件夹：按固件`升级类型`命名, eg:

    `update_by_ROM`: 指通过`MCU`中`ROM`的`openbootloader`更新`userApp`，所以`bootloader`不需要用户自己实现，只需要在`userApp`里通过一定的条件跳转到`ROM`的`bootloader`就可以了。
    
    `update_by_DIY`：指通过自己的方式定制`bootloader`，所以此文件夹里的每个子类都有`bootloader`和`application`两个工程。
3. 第三层文件夹：按固件升级的接口协议命名：eg: ROM_USART, DIY_FDCAN;
4. 不管是`update_by_ROM`还是`update_by_DIY`，都涉及到以下三个值的定义，其意义解释如下：
```c
// 这只是一个自己随意定义的一个标志值，可以随机取
#define BOOTLOADER_FLAG_VALUE 			0xDEADBEEF

// 这是表明上面的随机值需要存放在距离栈顶多远，即栈顶的OFFSET；
// 对这个值有两个要求：
//     1. 这个值不能大于栈的总空间大小；(MDK工程在startup_xxx.s文件开头就定义了栈空间)
//     2. 要确保这个OFFSET够大，以至于不会在程序初始化时被覆盖；
// 综上，这个值尽量取不超过OFFSET的最大值最安全；实际上0x50,0x100都貌似都够了.
#define BOOTLOADER_FLAG_OFFSET 			0x100

/**
 *  对这个值还有点疑惑，这个值的意义就是指定栈顶位置（C语言中，栈是向下增长的）。
 *  理论上针对同一个编译器(MDK)来说，每个工程的栈空间分配都是在差不多的位置；
 *  实际上，我在使用stm32h743iit6时，栈顶就是0x20000400，即栈空间在片内SRAM的开头(栈空间为0x400 Bytes)
 *  而我在使用stm32g473rct6时，栈顶却在0x20020000，即片内SRAM的最末尾(此MCU的SRAM为0x20000=128KB)；
 *  而这个值的确定，究竟是根据哪里的信息决定的，我暂时还不知道！
 */
uint32_t endstack = 0x20000400;
```
5. 在`stm32`中，在发生跳转之前，如果需要跳转的`目标地址`是可以通过设置`BOOT引脚`设置的`默认跳转地址`，则不需要调用`__set_MSP();`设置`MSP`指针；而目标地址是其他的`自定义跳转地址`时，在跳转前就必须调用`__set_MSP();`设置`MSP`指针的，并在跳转后的代码一开始就需要`重定向中断向量表`:
```c
// 重定向中断向量表
SCB->VTOR = APP_ADDRESS;
```

## update_by_ROM——Jump between system memory(ROM) and application(flash)
1. 通过`ROM`中的`bootloader`进行固件升级是最简单有效的方式，但是一定要先在`AN2606`文档中确定一下，目标`MCU`的`bootloader`是否支持你使用的接口协议，比如`stm32g473`，其`ROM`中的`bootloader`就不支持通过`FDCAN`进行固件升级，如果需要使用`FDCAN`进行固件升级，就只能自己实现`bootloader`了；
2. 关于代码中`BOOTLOADER_ADDRESS`的值(即`bootloader`起始地址)也可以通过`AN2606`文档确定，eg:
![stm32g47xx bootloader address](./img/stm32g47xx%20bootloader%20address.png)
![stm32h74xx bootloader address](./img/stm32h74xx%20bootloader%20address.png)
3. 在`application`跳转到`ROM`中时，是不需要调用`__set_MSP();`设置`MSP`指针的。

## update_by_DIY——Jump between bootloader(flash) and application(flash)
1. 关于`APP_ADDRESS`的值(即`application`的起始地址)确定，首先肯定是要给`bootloader`的固件留下足够的空间，让`bootloader`能存储得下才行；再一个就是`stm32`的`Flash`擦除是`按扇区(Sector)擦除`的，所以这个`APP_ADDRESS`还必须是扇区大小的整数倍才行，比如`stm32h743iit6`的参考手册上就有如下信息：
![RM0433.Rev8 148/3353](./img/RM0433.Rev8%20148-3353.png)
所以即使`bootloader`只用了`20KB`左右的空间，`APP_ADDRESS`也必须要设置为`0x08020000(0x20000=128KB)`;
2. 如果`bootloader`的起始地址为默认的`0x08000000`的话，在从`application`跳转到`bootloader`前，是不需要调用`__set_MSP();`设置`MSP`指针的；
3. 而当`application`的起始地址不是一个默认起始地址时，在由`bootloader`跳转到`application`前就必须要调用`__set_MSP();`设置`MSP`指针了。

## 总结
1. [openBootloader](https://github.com/STMicroelectronics/stm32-mw-openbl)是写在STM32的ROM里面的，用户不能擦除修改；
2. `ROM`里面的`openBootloader`不仅可以通过`usart`进行升级，也可以通过`SPI`，`I2C`，`FDCAN`，`USB`等接口进行升级，但是每个MCU支持的接口可能不尽相同；
3. `ROM`里面的`openBootloader`不仅可以通过在上电时修改`BOOT0/BOOT1`引脚电平进入，也可以从`user_app`跳转进入（参考: AN2606.Rev59 31/431），
<img width="880" alt="5fccb478a87b4f84ec3798b645740d8" src="https://github.com/ShadowThree/stm32_update/assets/41199004/23ff073d-7d6c-45ee-b0a4-a4443f5b824f">

4. stm32g473rct6在跳转到ROM中的bootloader时，需要通过以下语句将ROM重映射到0x0地址：
```c
__HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();      // remap system memory to 0x00000000
```
但stm32h743iit6却不需要（且此MCU的HAL库中不存在这个接口）。
我猜这个区别和这两个`MCU`的`Reference Manuel`中`Boot configuration`部分的描述有关(但目前也说不太清楚)：

<img width="516" alt="3b24cca0cd8987756cd0448ccf638f8" src="https://github.com/ShadowThree/stm32_update/assets/41199004/760d844d-c28c-4ad4-80ee-a01d290108bb">
<img width="474" alt="a591d4c6664f62aeabe732eef54fc48" src="https://github.com/ShadowThree/stm32_update/assets/41199004/78bd074f-d8a1-4743-9344-0987fcc378b4">
