这是一个linuxi2c驱动，用来学习Linux的i2c的控制
下面是vscode中c语言插件的配置文件
需要修改linux内核源代码路径
```sh
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/linux/linux_hy/include",
                "${workspaceFolder}/linux/linux_hy/arch/arm/include",
                "${workspaceFolder}/linux/linux_hy/arch/arm/include/generated/"
            ],
            "defines": ["__KERNEL__"],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c11",
            "cppStandard": "c++17", 
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}

```