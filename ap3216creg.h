#ifndef AP3216C_H
#define AP3216C_H

#define AP3216C_SYSTEMCONFIG 0x00    /* 配置寄存器         */
#define AP3216C_INTSTATUS    0x01    /* 中断状态寄存器     */
#define AP3216C_INTCLEAR     0x02    /* 中断清除寄存器     */
#define AP3216C_IRDATALOW    0x0A    /* IR 数据低字节      */
#define AP3216C_IRDATAHIGH   0x0B    /* IR 数据高字节      */
#define AP3216C_ALSDATALOW   0x0C    /* ALS 数据低字节     */
#define AP3216C_ALSDATAHIGH  0x0D    /* ALS 数据高字节     */
#define AP3216C_PSDATALOW    0x0E    /* PS 数据低字节      */
#define AP3216C_PSDATAHIGH   0x0F    /* PS 数据高字节      */

#endif
