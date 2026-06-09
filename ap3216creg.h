#ifndef AP3216C_H
#define AP3216C_H

#define AP3216C_SYSTEMCONG  0X00    /*配置寄存器*/
#define AP3216C_INTSTATUS   0X01    /*中断状态寄存器*/
#define AP3216C_INTCLEAT    0X02    /*中断清除寄存器*/
#define AP3216C_IRDATALOW   0X0A    /*IR数据低字节*/
#define AP3216C_IRDATAHIGH  0X0B    /*IR数据高字节*/
#define AP3216C_ALSDATALOW  0X0C    /*ALS数据低字节*/
#define AP3216C_ALSDATAHIGH 0X0D    /*ALS数据高字节*/
#define AP3216C_PSDATALOW   0X0E    /*PS数据低字节*/
#define AP3216C_PSDATAHIGH  0X0F    /*PS数据高字节*/

#endif