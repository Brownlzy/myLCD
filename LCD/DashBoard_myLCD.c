#include "DEV_Config.h"
#include "LCD_2inch.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include "myLCD.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>

#define MAC_SIZE 18
#define IP_SIZE 16

typedef struct cpu_occupy_ //定义一个cpu occupy的结构体
{
    char name[20];       //定义一个char类型的数组名name有20个元素
    unsigned int user;   //定义一个无符号的int类型的user
    unsigned int nice;   //定义一个无符号的int类型的nice
    unsigned int system; //定义一个无符号的int类型的system
    unsigned int idle;   //定义一个无符号的int类型的idle
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
} cpu_occupy_t;

char *NowTime(char *string);
double cal_cpuoccupy(cpu_occupy_t *o, cpu_occupy_t *n);
void get_cpuoccupy(cpu_occupy_t *cpust);
char *get_temperature(char *buff);
int get_IP(const char *eth_inf, char *ip);
char *get_sysinfo(int opt, char *string);
char *get_Rom(char *string);
void getCurrentDownloadRates(char *intface, int opt, long int *save_rate);
void getDefaultIface(char iface[]);
char *getWeekDay(char *weekday);
void setNetworkName(char *net1, char *net2, char *net3);
void setDefaultNetworkName();
char *get_model_name(char *buff);

char netName[3][50];
char modelName[64];
void setNetworkName(char *net1, char *net2, char *net3)
{
    strcpy(netName[0], net1);
    strcpy(netName[1], net2);
    strcpy(netName[2], net3);
}
void setDefaultNetworkName()
{
    strcpy(netName[0], "eth0");
    strcpy(netName[1], "wlan0");
    strcpy(netName[2], "usb0");
}
void DashBoard_myLCD(void)
{
    char string[64];
    cpu_occupy_t cpu_stat1;
    cpu_occupy_t cpu_stat2;
    double cpu;
    long int start_download_rates[3]; //保存结果时的流量计数
    long int end_download_rates[3];
    long int start_upload_rates[3]; //保存开始时的流量计数
    long int end_upload_rates[3];
    double download_rates[3];
    double upload_rates[3];
    char nup[3] = {'0', '0', '0'};

    get_model_name(modelName);
    int modelX=(strlen(modelName) * 11 <= 320) ? (320 - strlen(modelName) * 11) / 2 : 0;
    // clock_t start, finish;
    // double Total_time;

    // Exception handling:ctrl + c
    signal(SIGINT, Handler_2IN_LCD);

    /* Module Init */
    if (DEV_ModuleInit() != 0)
    {
        DEV_ModuleExit();
        exit(0);
    }

    /* LCD Init */
    printf("myLCD v1.0 By Brownlzy\r\n");
    LCD_2IN_Init();
    LCD_2IN_Clear(WHITE);
    LCD_SetBacklight(1010);

    UDOUBLE Imagesize = LCD_2IN_HEIGHT * LCD_2IN_WIDTH * 2;
    UWORD *BlackImage;
    if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    get_cpuoccupy((cpu_occupy_t *)&cpu_stat1);
    for (int i = 0; i < 3; i++)
    {
        getCurrentDownloadRates(netName[i], 0, &start_download_rates[i]); //获取当前流量，并保存在start_download_rates里
        getCurrentDownloadRates(netName[i], 1, &start_upload_rates[i]);   //获取当前流量，并保存在start_download_rates里
    }
    do
    {
        // start = clock();
        /*1.Create a new image cache named IMAGE_RGB and fill it with black*/
        Paint_NewImage(BlackImage, LCD_2IN_WIDTH, LCD_2IN_HEIGHT, 90, BLACK, 16);
        Paint_Clear(BLACK);
        Paint_SetRotate(ROTATE_90);

        /*2.Drawing on the image  8x5 12x7 16x11 20x15 14x17*/
        // Paint_DrawString_EN(35, 13, "Raspberry Pi 4B", &Font20, BLACK, WHITE);
        // Paint_DrawString_EN(255, 17, "rev1.4", &Font12, BLACK, WHITE);
        Paint_DrawString_EN(modelX, 15, modelName, &Font16, BLACK, WHITE);
        Paint_DrawString_EN(5, 36, "-----------myLCD v1.0 By Brownlzy-----------", &Font12, BLACK, GRAY);

        //显示时间
        Paint_DrawString_EN(5, 54, " TIME : ", &Font16, BLACK, WHITE);
        NowTime(string);
        Paint_DrawString_EN(96, 55, string, &Font12, BLACK, CYAN);
        getWeekDay(string);
        Paint_DrawString_EN(201, 55, string, &Font12, BLACK, GREEN);

        //计算cpu使用率
        Paint_DrawString_EN(5, 73, "  CPU : ", &Font16, BLACK, WHITE);
        get_cpuoccupy((cpu_occupy_t *)&cpu_stat2);
        cpu = cal_cpuoccupy((cpu_occupy_t *)&cpu_stat1, (cpu_occupy_t *)&cpu_stat2);
        sprintf(string, "%.2f%%", cpu);
        cpu_stat1 = cpu_stat2;
        if (cpu <= 40)
            Paint_DrawString_EN(96, 74, string, &Font12, BLACK, GREEN);
        else if (cpu > 70)
            Paint_DrawString_EN(96, 74, string, &Font12, BLACK, RED);
        else
            Paint_DrawString_EN(96, 74, string, &Font12, BLACK, YELLOW);
        //显示温度
        get_temperature(string);
        // Paint_DrawString_EN(154, 73, "TEMP : ", &Font16, BLACK, WHITE);
        if (string[0] < '4')
            Paint_DrawString_EN(164, 74, get_temperature(string), &Font12, BLACK, GREEN);
        else if (string[0] > '4')
            Paint_DrawString_EN(164, 74, get_temperature(string), &Font12, BLACK, RED);
        else
            Paint_DrawString_EN(164, 74, get_temperature(string), &Font12, BLACK, YELLOW);

        //显示开机时间
        get_sysinfo(2, string);
        Paint_DrawString_EN(232, 74, string, &Font12, BLACK, WHITE);

        //显示内存信息
        Paint_DrawString_EN(5, 95, "  RAM : ", &Font16, BLACK, WHITE);
        get_sysinfo(0, string);
        if (string[6] > '6' || (string[6] == '1' && string[7] == '0' && string[8] == '0'))
            Paint_DrawString_EN(96, 92, string, &Font12, BLACK, RED);
        else if (string[6] <= '3')
            Paint_DrawString_EN(96, 92, string, &Font12, BLACK, GREEN);
        else
            Paint_DrawString_EN(96, 92, string, &Font12, BLACK, YELLOW);
        get_sysinfo(1, string);
        if (string[6] > '6' || (string[6] == '1' && string[7] == '0' && string[8] == '0'))
            Paint_DrawString_EN(96, 104, string, &Font12, BLACK, RED);
        else if (string[6] <= '3')
            Paint_DrawString_EN(96, 104, string, &Font12, BLACK, GREEN);
        else
            Paint_DrawString_EN(96, 104, string, &Font12, BLACK, YELLOW);

        //获取硬盘空间
        Paint_DrawString_EN(5, 121, "  ROM : ", &Font16, BLACK, WHITE);
        get_Rom(string);
        if (string[0] < '8')
            Paint_DrawString_EN(96, 122, string, &Font12, BLACK, CYAN);
        else
            Paint_DrawString_EN(96, 122, string, &Font12, BLACK, RED);

        //显示IP
        // Paint_DrawString_EN(5, 103, "   IP : ", &Font16, BLACK, WHITE);
        Paint_DrawString_EN(5, 178, "  NET : ", &Font16, BLACK, WHITE);
        char iface[50];
        getDefaultIface(iface);
        for (int i = 0; i < 3; i++)
        {
            if (get_IP(netName[i], string) == 0)
            {
                if (strcmp(iface, netName[i]) == 0)
                    Paint_DrawString_EN(96, 149 + 31 * i, "*", &Font16, BLACK, GREEN);
                else
                    Paint_DrawString_EN(96, 149 + 31 * i, "+", &Font16, BLACK, WHITE);
                Paint_DrawString_EN(110, 145 + 31 * i, netName[i], &Font12, BLACK, WHITE);
                Paint_DrawString_EN(164, 145 + 31 * i, string, &Font12, BLACK, WHITE);
                nup[i] = '1';
            }
            else
            {
                Paint_DrawString_EN(96, 149 + 31 * i, "-", &Font16, BLACK, GRAY);
                Paint_DrawString_EN(110, 145 + 31 * i, netName[i], &Font12, BLACK, GRAY);
                Paint_DrawString_EN(164, 145 + 31 * i, "0.0.0.0", &Font12, BLACK, GRAY);
                nup[i] = '0';
            }
        }
        //计算网速
        // Paint_DrawString_EN(5, 204, "  NET : ", &Font16, BLACK, WHITE);
        for (int i = 0; i < 3; i++)
        {
            if (nup[i] == '1')
            {
                getCurrentDownloadRates(netName[i], 0, &end_download_rates[i]);
                getCurrentDownloadRates(netName[i], 1, &end_upload_rates[i]);
                download_rates[i] = end_download_rates[i] - start_download_rates[i];
                start_download_rates[i] = end_download_rates[i];
                char *rate;
                if (download_rates[i] >= 1024 && download_rates[i] < 1000000)
                {
                    download_rates[i] /= 1024;
                    rate = "KB/s";
                }
                else if (download_rates[i] > 1000000)
                {
                    download_rates[i] /= (1024 * 1024);
                    rate = "MB/s";
                }
                else
                {
                    rate = "B/s";
                }
                sprintf(string, "A %.2lf %s", download_rates[i], rate);
                Paint_DrawString_EN(110, 157 + 31 * i, string, &Font12, BLACK, WHITE);
                upload_rates[i] = (end_upload_rates[i] - start_upload_rates[i]) * 0.6;
                start_upload_rates[i] = end_upload_rates[i];
                if (upload_rates[i] >= 1000 && upload_rates[i] < 1000000)
                {
                    upload_rates[i] /= 1024;
                    rate = "KB/s";
                }
                else if (upload_rates[i] > 1000000)
                {
                    upload_rates[i] /= (1024 * 1024);
                    rate = "MB/s";
                }
                else
                {
                    rate = "B/s";
                }
                sprintf(string, "V %.2lf %s", upload_rates[i], rate);
                Paint_DrawString_EN(210, 157 + 31 * i, string, &Font12, BLACK, WHITE);
            }
            else
            {
                Paint_DrawString_EN(110, 157 + 31 * i, "A 0.00 B/s", &Font12, BLACK, GRAY);
                Paint_DrawString_EN(210, 157 + 31 * i, "V 0.00 B/s", &Font12, BLACK, GRAY);
            }
        }
        /*3.Refresh the picture in RAM to LCD*/
        LCD_2IN_Display((UBYTE *)BlackImage);
        // finish = clock();
        // Total_time = (double)(finish - start) / CLOCKS_PER_SEC * 1000;

        //	if (Total_time < 1000 && Total_time >0)
        //	    DEV_Delay_ms(1000-(int)Total_time);
        //	else
        DEV_Delay_ms(1000);
        //	printf("%lf\n",Total_time);
    } while (1);

    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
    DEV_ModuleExit();
}

char *NowTime(char *string)
{
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    sprintf(string, "CST %4d/%02d/%02d     %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
    return string;
}

double cal_cpuoccupy(cpu_occupy_t *o, cpu_occupy_t *n)
{
    double od, nd;
    double id, sd;
    double cpu_use;

    od = (double)(o->user + o->nice + o->system + o->idle + o->softirq + o->iowait + o->irq); //第一次(用户+优先级+系统+空闲)的时间再赋给od
    nd = (double)(n->user + n->nice + n->system + n->idle + n->softirq + n->iowait + n->irq); //第二次(用户+优先级+系统+空闲)的时间再赋给od

    id = (double)(n->idle); //用户第一次和第二次的时间之差再赋给id
    sd = (double)(o->idle); //系统第一次和第二次的时间之差再赋给sd
    if ((nd - od) != 0)
        cpu_use = 100.0 - ((id - sd)) / (nd - od) * 100.00; //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
    else
        cpu_use = 0;
    return cpu_use;
}

void get_cpuoccupy(cpu_occupy_t *cpust)
{
    FILE *fd;
    char buff[256];
    cpu_occupy_t *cpu_occupy;
    cpu_occupy = cpust;
    fd = fopen("/proc/stat", "r");
    if (fd == NULL)
    {
        perror("fopen:");
        exit(0);
    }
    fgets(buff, sizeof(buff), fd);

    sscanf(buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice, &cpu_occupy->system, &cpu_occupy->idle, &cpu_occupy->iowait, &cpu_occupy->irq, &cpu_occupy->softirq);

    fclose(fd);
}

char *get_temperature(char *buff)
{
    FILE *fd;
    fd = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fd == NULL)
    {
        perror("fopen:");
        exit(0);
    }
    fgets(buff, 64 * sizeof(char), fd);
    fclose(fd);
    float tmp;
    tmp = atoi(buff);
    sprintf(buff, "%4.1f'C", tmp / 1000.);
    return buff;
}
char *strrpc(char *str, char *oldstr, char *newstr)
{
    char bstr[strlen(str)]; //转换缓冲区
    memset(bstr, 0, sizeof(bstr));
    for (int i = 0; i < strlen(str); i++)
    {
        if (!strncmp(str + i, oldstr, strlen(oldstr)))
        { //查找目标字符串
            strcat(bstr, newstr);
            i += strlen(oldstr) - 1;
        }
        else
        {
            strncat(bstr, str + i, 1); //保存一字节进缓冲区
        }
    }
    strcpy(str, bstr);
    return str;
}
char *get_model_name(char *buff)
{
    FILE *fd;
    fd = fopen("/proc/device-tree/model", "r");
    if (fd == NULL)
    {
        perror("fopen:");
        exit(0);
    }
    fgets(buff, 64 * sizeof(char), fd);
    fclose(fd);
    strrpc(buff,"y P","yP");
    strrpc(buff," Model ","");
    strrpc(buff,"Rev ","rev");
    return buff;
}

int get_IP(const char *eth_inf, char *ip)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd)
    {
        // printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
    {
        // printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

char *get_sysinfo(int opt, char *string)
{
    struct sysinfo s_info;
    char info_buff[100];
    if (sysinfo(&s_info) == 0)
    {
        switch (opt)
        {
        case 0:
            sprintf(info_buff, "Real  %4.1lf%%  %ld M / %.ld M", ((s_info.totalram * 1. - s_info.freeram) / s_info.totalram * 100.), (s_info.totalram - s_info.freeram) / 1024 / 1024, s_info.totalram / 1024 / 1024);
            strcpy(string, info_buff);
            break;
        case 1:
            sprintf(info_buff, "Swap  %4.1lf%%  %ld M / %.ld M", ((s_info.totalswap * 1. - s_info.freeswap) / s_info.totalswap * 100.), (s_info.totalswap - s_info.freeswap) / 1024 / 1024, s_info.totalswap / 1024 / 1024);
            strcpy(string, info_buff);
            break;
        case 2:
            sprintf(info_buff, "%01ld:%02ld:%02ld", s_info.uptime / 60 / 60, s_info.uptime / 60 % 60, s_info.uptime % 60);
            strcpy(string, info_buff);
            break;
        }
    }
    return 0;
}

char *get_Rom(char *string)
{
    struct statfs sfs;
    statfs("/", &sfs);
    double percent = (sfs.f_blocks - sfs.f_bfree) * 100. / (sfs.f_blocks - sfs.f_bfree + sfs.f_bavail) + 1;
    sprintf(string, "%4.1lf%%  %ld M / %ld M", percent, 4 * (sfs.f_blocks - sfs.f_bfree) / 1024, 4 * sfs.f_blocks / 1024);
    return string;
}

void getCurrentDownloadRates(char *intface, int opt, long int *save_rate)
{
    FILE *net_dev_file;
    char buffer[1024];
    if ((net_dev_file = fopen("/proc/net/dev", "r")) == NULL)
    {
        printf("open file /proc/net/dev/ error!\n");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    long int a;
    while (i++ < 120)
    {
        if (fgets(buffer, sizeof(buffer), net_dev_file) != NULL)
        {
            if (strstr(buffer, intface) != NULL)
            {
                // printf("%d   %s\n",i,buffer);
                if (opt)
                    sscanf(buffer, "%s %ld", buffer, save_rate);
                else
                    sscanf(buffer, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld", buffer, &a, &a, &a, &a, &a, &a, &a, &a, save_rate);
                break;
            }
        }
    }
    if (i == 120)
        *save_rate = 0.01;
    fclose(net_dev_file); //关闭文件
    return;
}

void getDefaultIface(char iface[])
{
    char Cmd[100] = {0};
    char buffer[1024];
    char readline[100] = {0};
    memset(Cmd, 0, sizeof(Cmd));
    sprintf(Cmd, "netstat -r|grep default");
    FILE *fp = popen(Cmd, "r");

    if (NULL == fp)
    {
        strcpy(iface[0], "none");
        return;
    }

    memset(buffer, 0, sizeof(buffer));
    if (NULL == fgets(buffer, sizeof(buffer), fp))
    {
        pclose(fp);
        strcpy(iface[0], "none");
        return;
    }
    else
    {
        sscanf(buffer, "%s %s %s %s %s %s %s %s", readline, readline, readline, readline, readline, readline, readline, iface);
        pclose(fp);
        return;
    }
}
char *getWeekDay(char *weekday)
{
    time_t now;
    struct tm *timenow;
    int y, m, d, w;
    time(&now);
    timenow = localtime(&now);
    // sprintf(string, "%d/%d/%d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
    y = timenow->tm_year + 1900;
    m = timenow->tm_mon + 1;
    d = timenow->tm_mday;
    if (m == 1 || m == 2)
    {
        m += 12;
        y -= 1;
    }
    w = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    switch (w)
    {
    case 0:
        strcpy(weekday, "Mon");
        break;
    case 1:
        strcpy(weekday, "Tue");
        break;
    case 2:
        strcpy(weekday, "Wen");
        break;
    case 3:
        strcpy(weekday, "Thr");
        break;
    case 4:
        strcpy(weekday, "Fri");
        break;
    case 5:
        strcpy(weekday, "Sat");
        break;
    case 6:
        strcpy(weekday, "Sun");
        break;
    default:
        strcpy(weekday, "Err");
    }
    return weekday;
}