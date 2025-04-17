
#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <string.h>
#include "myUtil.h"
using namespace ns_my_std;

/* 115200, 8, N, 1 */
int uart_setup(int fd)
{
    struct termios options;

    // 获取原有串口配置
    if (tcgetattr(fd, &options) < 0)
    {
        return -1;
    }

    // 修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;

    // 修改控制模式，能够从串口读取数据
    options.c_cflag |= CREAD;

    // 不使用流控制
    options.c_cflag &= ~CRTSCTS;

    // 设置数据位
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    //设置奇偶校验位
    thelog << "无校验" << endi;
    options.c_iflag &= ~INPCK;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~PARODD;

    //thelog << "偶校验even" << endi;
    //options.c_iflag |= INPCK;
    //options.c_cflag |= PARENB;
    //options.c_cflag &= ~PARODD;
  
    //thelog << "奇校验odd" << endi;
    //options.c_iflag &= ~INPCK;
    //options.c_cflag &= ~PARENB;
    //options.c_cflag |= PARODD;

    // 设置停止位
    options.c_cflag &= ~CSTOPB;

    // 设置最少字符和等待时间
    options.c_cc[VMIN] = 1;     // 读数据的最小字节数
    options.c_cc[VTIME] = 0;   //等待第1个数据，单位是10s

    // 修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // 设置波特率
    cfsetispeed(&options, B1200);//B115200
    cfsetospeed(&options, B1200);//B115200

    // 清空终端未完成的数据
    tcflush(fd, TCIFLUSH);

    // 设置新属性
    if (tcsetattr(fd, TCSANOW, &options) < 0)
    {
        return -1;
    }

    return 0;
}
int openSerialPort(int* fd)
{
    *fd = open("/dev/ttySE1", O_RDWR | O_NOCTTY | O_NDELAY);//5G网关
    //*fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);//ubuntu虚拟机
    if (*fd < 0)
    {
        printf("open dev fail!\n");
        return -1;
    }
    else
    {
        fcntl(*fd, F_SETFL, 0);
    }
    return 0;
}
int serial_port_test()
{
    int fd;
    int ret;
    char ch;

	/* 打开串口 */
	if (0 != openSerialPort(&fd))
	{
		printf("open dev fail!\n");
		return -1;
	}

    /* 设置串口 */
    ret = uart_setup(fd);
    if (ret < 0)
    {
        printf("uart setup fail!\n");
        close(fd);
        return -1;
    }

    /* 串口回传实验 */
    int i = 0;
    while (++i<10)
    {
        ret = read(fd, &ch, 1);
        if (ret < 1)
        {
            printf("read fail, ret is %d\r\n", ret);
        }
        else
        {
            printf("recv a char:[0x%02x][%c]\r\n", ch, ch);
			ch = 'a' + i - 1;
			ret = write(fd, &ch, 1);
			printf("write [%c] , ret is %d!\r\n", ch, ret);
        }
    }

    close(fd);
    return 0;
}

/*
termios结构体中，该结构体一般包括如下的成员：
tcflag_t       c_iflag;
tcflag_t       c_oflag;
tcflag_t       c_cflag;
tcflag_t       c_lflag;
cc_t            c_cc[NCCS];

 其具体意义如下

c_iflag：输入模式标志，控制终端输入方式，具体参数如下所示。

c_iflag参数表
键值说明
IGNBRK       忽略BREAK键输入
BRKINT       如果设置了IGNBRK，BREAK键的输入将被忽略，如果设置了BRKINT ，将产生SIGINT中断
IGNPAR       忽略奇偶校验错误
PARMRK     标识奇偶校验错误
INPCK        允许输入奇偶校验
ISTRIP       去除字符的第8个比特
INLCR        将输入的NL（换行）转换成CR（回车）
IGNCR       忽略输入的回车
ICRNL        将输入的回车转化成换行（如果IGNCR未设置的情况下）
IUCLC        将输入的大写字符转换成小写字符（非POSIX）
IXON         允许输入时对XON/XOFF流进行控制
IXANY        输入任何字符将重启停止的输出
IXOFF        允许输入时对XON/XOFF流进行控制
IMAXBEL   当输入队列满的时候开始响铃，Linux在使用该参数而是认为该参数总是已经设置



c_oflag：       输出模式标志，控制终端输出方式，具体参数如下所示。
c_oflag参数
键值说明
OPOST       处理后输出
OLCUC      将输入的小写字符转换成大写字符（非POSIX）
ONLCR      将输入的NL（换行）转换成CR（回车）及NL（换行）
OCRNL      将输入的CR（回车）转换成NL（换行）
ONOCR      第一行不输出回车符
ONLRET      不输出回车符
OFILL         发送填充字符以延迟终端输出
OFDEL       以ASCII码的DEL作为填充字符，如果未设置该参数，填充字符将是NUL（‘/0’）（非POSIX）
NLDLY       换行输出延时，可以取NL0（不延迟）或NL1（延迟0.1s）
CRDLY       回车延迟，取值范围为：CR0、CR1、CR2和 CR3
TABDLY     水平制表符输出延迟，取值范围为：TAB0、TAB1、TAB2和TAB3
BSDLY       空格输出延迟，可以取BS0或BS1
VTDLY       垂直制表符输出延迟，可以取VT0或VT1
FFDLY       换页延迟，可以取FF0或FF1

c_cflag：控制模式标志，指定终端硬件控制信息，具体参数如下所示。
c_oflag参数
键值说明
CBAUD             波特率（4+1位）（非POSIX）
CBAUDEX        附加波特率（1位）（非POSIX）
CSIZE              字符长度，取值范围为CS5、CS6、CS7或CS8
CSTOPB         设置两个停止位
CREAD           使用接收器
PARENB         使用奇偶校验
PARODD        对输入使用奇偶校验，对输出使用偶校验
HUPCL           关闭设备时挂起
CLOCAL         忽略调制解调器线路状态
CRTSCTS         使用RTS/CTS流控制


c_lflag：本地模式标志，控制终端编辑功能，具体参数如下所示。
c_lflag参数
键值说明
ISIG                  当输入INTR、QUIT、SUSP或DSUSP时，产生相应的信号
ICANON           使用标准输入模式
XCASE            在ICANON和XCASE同时设置的情况下，终端只使用大写。如果只设置了XCASE，则输入字符将被转换为小写字符，除非字符使用了转义字符（非POSIX，且Linux不支持该参数）
ECHO               显示输入字符
ECHOE            如果ICANON同时设置，ERASE将删除输入的字符，WERASE将删除输入的单词
ECHOK            如果ICANON同时设置，KILL将删除当前行
ECHONL          如果ICANON同时设置，即使ECHO没有设置依然显示换行符
ECHOPRT        如果ECHO和ICANON同时设置，将删除打印出的字符（非POSIX）
TOSTOP           向后台输出发送SIGTTOU信号

在串口编程模式下，open未设置O_NONBLOCK或O_NDELAY的情况下。
c_cc[VTIME]和c_cc[VMIN]映像read函数的返回。

VTIME定义等待的时间，单位是百毫秒(通常是一个8位的unsigned char变量，取值不能大于cc_t)。

VMIN定义了要求等待的最小字节数，这个字节数可能是0。

如果VTIME取0，VMIN定义了要求等待读取的最小字节数。函数read()只有在读取了VMIN个字节的数据或者收到一个信号的时候才返回。

如果VMIN取0，VTIME定义了即使没有数据可以读取，read()函数返回前也要等待几百毫秒的时间量。这时，read()函数不需要像其通常情况那样要遇到一个文件结束标志才返回0。

如果VTIME和VMIN都不取0，VTIME定义的是当接收到第一个字节的数据后开始计算等待的时间量。如果当调用read函数时可以得到数据，计时器 马上开始计时。如果当调用read函数时还没有任何数据可读，则等接收到第一个字节的数据后，计时器开始计时。函数read可能会在读取到VMIN个字节 的数据后返回，也可能在计时完毕后返回，这主要取决于哪个条件首先实现。不过函数至少会读取到一个字节的数据，因为计时器是在读取到第一个数据时开始计时 的。

如果VTIME和VMIN都取0，即使读取不到任何数据，函数read也会立即返回。同时，返回值0表示read函数不需要等待文件结束标志就返回了。
 */
