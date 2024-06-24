#include "lvgl/examples/lv_examples.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char show_path[1024] = {0};     // music名字
char all_pic[50][100] = {0};    // 路径
static int in_dex=0;    //记录当前歌曲位置
int loc=0;  
FILE *fp=NULL;
int fd = 0;
static lv_obj_t *slider_label1;     // 音量滑块
int volum_size = 10; // 初始音量
static lv_obj_t * list1;    // 音乐列表
static lv_obj_t *slider2;   // 进度条
static lv_obj_t *wp;    // 图片对象
uint32_t cnt;    // 暂停 播放标识符
lv_obj_t *label;
int i = 0;  // 暂停播放按钮标志
int j = 3;  // 播放按钮标志
int k = 0;
static lv_style_t style;    // 按钮样式
int get_time;   // 播放时间百分比
int get_length; // 音乐总时长
int get_pos;    // 音乐时长
pthread_t time_tid;   // 读取时间线程
lv_obj_t * dd;       // 倍速播放列表
static const char *opts;    // 倍数播放标签
char length_time[1024] = {0};    // 总时长
char pos_time[1024] = {0};    // 时长
static lv_obj_t *length_label;  // 总时长标签
static lv_obj_t *length_label1;  // 时长标签
static lv_style_t style1;   // 背景样式


int find_music_loc(char *arg)
{
    for(int j=0;j<in_dex;j++)
    {
        if(strcmp(all_pic[j],arg) == 0)
        {
             loc = j;
             return  loc;  //返回当前的位置
        }
    }

}

void my_style()
{
    lv_style_init(&style);
    lv_style_set_radius(&style, 25);

    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_VER;
    grad.stops_count = 1;
    grad.stops[0].color = lv_palette_lighten(LV_PALETTE_YELLOW,1);

    grad.stops[0].frac  = 128;
 
    lv_style_set_bg_grad(&style, &grad);
}

void lv_example_freetype_2(char buf[100])
{
    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_radius(&style2, 25);

    lv_style_set_bg_opa(&style2, LV_OPA_COVER);
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_VER;
    grad.stops_count = 1;
    grad.stops[0].color = lv_palette_lighten(LV_PALETTE_BLUE,3);

    grad.stops[0].frac  = 128;
 
    lv_style_set_bg_grad(&style2, &grad);

    //创建一个对象
    lv_obj_t * img1 = lv_obj_create(lv_scr_act());

    //设置对象的大小
    lv_obj_set_size(img1, 350, 50);

    lv_obj_align(img1, LV_ALIGN_CENTER, 0, -200);

    lv_obj_add_style(img1, &style2, 0);

    //创建一个字体
    static lv_ft_info_t info;
    // 设置字体的路径
    info.name = "/font/simkai.ttf";
    // 设置字体的宽度
    info.weight = 20;
    // 设置字体的样式
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    // 初始化字体
    if(!lv_ft_font_init(&info)) 
    {
        LV_LOG_ERROR("create failed.");
    }

    static lv_style_t style1;
    lv_style_init(&style1);
    // 设置字体的样式
    lv_style_set_text_font(&style1, info.font);
    lv_style_set_text_align(&style1, LV_TEXT_ALIGN_CENTER);

    // 创建一个标签
    lv_obj_t * label = lv_label_create(img1);
    // 把样式添加到标签中
    lv_obj_add_style(label, &style1, 0);
    lv_label_set_text(label, buf);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}


// 获取时间任务
void *time_task(void * arg)
{
    while(1)
    {          
        char buf3[1024] = {"get_time_pos\n"};
        write(fd,buf3,strlen(buf3));       
        char buf2[1024] = {"get_percent_pos\n"};
        write(fd,buf2,strlen(buf2)); 
        sleep(1);
    }     
} 

// 播放音乐列表任务
void *music_task(void * arg)
{
    k = 1;
    char buf1[100] = {0};
    sprintf(buf1,"volume %d 1\n",volum_size); 
    write(fd,buf1,strlen(buf1));    

    char buf[1024] = {0};
    sprintf(buf, "mplayer  -slave -quiet -input  file=/pipe %s", (char *)arg);
    fp=popen(buf,"r");
    if(fp == NULL)
    {
        perror("popen fail:");
    }
    else
    {
        printf("popen OK:\n");
    }

    char line[1024]={0};
       
    pthread_create(&time_tid, NULL, time_task, NULL);       
    pthread_detach(time_tid);

    char buf2[1024] = {"get_time_length\n"};
    write(fd,buf2,strlen(buf2)); 
    
    while (1)
    {
        fgets(line, 1024, fp); //读取进程返回的内容
        if(strstr(line,"ANS_PERCENT_POSITION") )
        {
            char num[1024];
            sscanf(line,"ANS_PERCENT_POSITION=%s",num);
            get_time = atof(num);
            lv_slider_set_value(slider2,get_time,LV_ANIM_ON);
        }
        if(strstr(line,"ANS_LENGTH") )
        {
            char time_num[1024];
            sscanf(line,"ANS_LENGTH=%s",time_num);
            get_length = atof(time_num);
            get_length_time();
            lv_label_set_text(length_label, length_time);   // 总时长标签
        }
        if(strstr(line,"ANS_TIME_POSITION") )
        {
            char time_num[1024];
            sscanf(line,"ANS_TIME_POSITION=%s",time_num);
            get_pos = atof(time_num);
            get_length_time();
            lv_label_set_text(length_label1, pos_time);   // 时长标签
        }
    }
}

// 显示图片
void show_photo()
{
    //创建一个对象
    lv_obj_t * img = lv_obj_create(lv_scr_act());

    //设置对象的大小
    lv_obj_set_size(img, 250, 250);

    lv_obj_align(img, LV_ALIGN_CENTER,0, -20);

    lv_obj_add_style(img, &style1, 0);

    //创建图片对象
    wp = lv_img_create(img);
                
    // 显示图片、信息
    if(strcmp(show_path,"music/sxyz.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/sxyz.jpg"); 
        lv_example_freetype_2("盛夏已至   刘宇");
    }

    if(strcmp(show_path,"music/hldwm.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/hldwm.jpg"); 
        lv_example_freetype_2("后来的我们   五月天");
    }
    if(strcmp(show_path,"music/jx.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/jx.jpg"); 
        lv_example_freetype_2("剑心   张杰");
    }
    if(strcmp(show_path,"music/ry.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/ry.jpg"); 
        lv_example_freetype_2("如愿   王菲");
    }
    if(strcmp(show_path,"music/zz.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/zz.jpg");         
        lv_example_freetype_2("知足   五月天");
    }
    if(strcmp(show_path,"music/sjyn.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/sjyn.jpg"); 
        lv_example_freetype_2("四季予你   程响");
    }
    if(strcmp(show_path,"music/cwsk.mp3")==0)
    {
        lv_img_set_src(wp,"S:/mplayer/photo/cwsk.jpg"); 
        lv_example_freetype_2("错位时空   艾辰");
    }
}

// 功能按钮事件
static void play_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    //获取传递的参数
    char * bt = lv_event_get_user_data(e);

    static uint32_t cnt1;    // 播放功能标识符

    if(code == LV_EVENT_CLICKED)
    {
        // 播放暂停
        if(strcmp(bt,"Play") == 0) 
        {     
            char  cmd[1024]={"pause\n"};
            write(fd,cmd,strlen(cmd));  
           
            switch(i)
            {
                case 0:
                    cnt = LV_SYMBOL_PLAY;
                    pthread_cancel(time_tid);
                    i = 1;
                    break;
                case 1:
                    cnt = LV_SYMBOL_PAUSE;    
                    pthread_create(&time_tid, NULL, time_task, NULL); 
                    pthread_detach(time_tid);
                    i = 0;
                    break;               
            }

            lv_obj_t *btn1 = lv_event_get_target(e);
            label = lv_obj_get_child(btn1, 0);
            lv_label_set_text_fmt(label, cnt); 
        }

        // 循环播放
        if(strcmp(bt,"Loop") == 0)
        {
            char cmd[1024] = {0};
            pthread_t tid;
            switch(j)
            {
                case 1:
                    printf("开始随机播放\n");
                    cnt1 = LV_SYMBOL_SHUFFLE;   //随机播放   
                    loc = rand() % in_dex;    
                    system("killall -9 mplayer");
                    pthread_create(&tid, NULL, music_task, &all_pic[loc]);
                    pthread_detach(tid);                        
                    strcpy(show_path,all_pic[loc]);
                    show_photo();     
                    j = 2;
                    break;
                case 2:
                    printf("开始按顺序播放\n");
                    cnt1 = LV_SYMBOL_REFRESH;   //顺序播放
                    loc = loc+1;
                    if(loc == in_dex)
                    {
                        loc = 0;
                    }
                    system("killall -9 mplayer");                    
                    pthread_create(&tid, NULL, music_task, &all_pic[loc]);  
                    pthread_detach(tid);                        
                    strcpy(show_path,all_pic[loc]);
                    show_photo(); 
                    j = 3;
                    break;       
                case 3:
                    printf("开始单曲循环播放\n");
                    cnt1 = LV_SYMBOL_LOOP;      //循环播放
                    char  cmd[1024]={"loop 1\n"};
                    write(fd,cmd,strlen(cmd)); 
                    j = 1;                
                    break;
            }
            lv_obj_t *btn2 = lv_event_get_target(e);
            lv_obj_t *labe2 = lv_obj_get_child(btn2, 0);
            lv_label_set_text_fmt(labe2, cnt1); 
        } 

        // 快进10s
        if(strcmp(bt,"Plus") == 0)
        {
            printf("快进10s\n");
            char  cmd[1024]={"seek +10\n"};
            write(fd,cmd,strlen(cmd));
        } 

        // 后退10s
        if(strcmp(bt,"Minus") == 0)
        {
            printf("后退10s\n");
            char  cmd[1024]={"seek -10\n"};
            write(fd,cmd,strlen(cmd));
        } 

    }
}

// 音乐列表事件
static void list_handler(lv_event_t * e)
{
    if(i == 1)
    {
        cnt = LV_SYMBOL_PAUSE;
        lv_label_set_text_fmt(label, cnt); 
        i = 0;
    }

    lv_obj_t *obj = lv_event_get_target(e);

    // 播放音乐列表
    lv_dropdown_get_selected_str(obj, show_path, sizeof(show_path));
    LV_LOG_USER("'%s' is selected", show_path); 
    find_music_loc(show_path);
    show_photo(); 

    if(k!=0)
    {
        pthread_cancel(time_tid);        
    }

    pthread_t tid; 
    system("killall -9 mplayer");   // 杀死进程    
    pthread_create(&tid, NULL, music_task, show_path);  
    pthread_detach(tid);
    // 倍速播放 
    lv_dropdown_set_options_static(dd, opts);
}

// 倍速播放事件
static void Speed_handler(lv_event_t * e)
{
    lv_obj_t * dd = lv_event_get_target(e);
    char buf[64];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));

    if(strcmp(buf,"X  1.0") == 0)
    {
        printf("正常倍速播放\n");
        char  cmd[1024]={"speed_set 1\n"};
        write(fd,cmd,strlen(cmd));
    } 
    if(strcmp(buf,"X  0.5") == 0)
    {
        printf("0.5倍速播放\n");
        char  cmd[1024]={"speed_set 0.5\n"};
        write(fd,cmd,strlen(cmd));
    } 
    if(strcmp(buf,"X  1.5") == 0)
    {
        printf("1.5倍速播放\n");
        char  cmd[1024]={"speed_set 1.5\n"};
        write(fd,cmd,strlen(cmd));
    } 
    if(strcmp(buf,"X  2.0") == 0)
    {
        printf("2.0倍速播放\n");
        char  cmd[1024]={"speed_set 2\n"};
        write(fd,cmd,strlen(cmd));
    } 
}

// 切换上下首事件
static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    //获取传递的参数 
    char *bt = lv_event_get_user_data(e);
    if(code == LV_EVENT_CLICKED) 
    {
        if(i == 1)
        {
            cnt = LV_SYMBOL_PAUSE;
            lv_label_set_text_fmt(label, cnt); 
            i = 0;
        }
        if(strcmp(bt,"Last") == 0)
        {
            loc--;  //上一首
            if(loc < 0)
            {
                loc = in_dex-1;
            }
            sprintf(show_path,"%s",all_pic[loc]);
            pthread_t tid;   
            pthread_cancel(time_tid);  
            system("killall -9 mplayer");   // 杀死进程
            pthread_create(&tid, NULL, music_task, show_path);  
            pthread_detach(tid);
            show_photo();   
            // 倍速播放 
            lv_dropdown_set_options_static(dd, opts);     
        }
        
        if(strcmp(bt,"Next") == 0)
        {
            loc++;  //下一首
            if(loc >= in_dex)
            {
                loc=0;
            }

            sprintf(show_path,"%s",all_pic[loc]);

            pthread_t tid;   
            pthread_cancel(time_tid); 
            system("killall -9 mplayer");   // 杀死进程
            pthread_create(&tid, NULL, music_task, show_path); 
            pthread_detach(tid);
            show_photo();     
            // 倍速播放 
            lv_dropdown_set_options_static(dd, opts);                             
        }

    }
}

// 音量调节事件
static void value_changed_cb(lv_event_t * e)
{
    lv_obj_t *arc = lv_event_get_target(e);

    lv_obj_t *label = lv_event_get_user_data(e);

    volum_size = (int)lv_arc_get_value(arc);      // 获取滑动条的值

    lv_label_set_text_fmt(label, "%d%%", lv_arc_get_value(arc));

    char buf1[100];

    sprintf(buf1,"volume %d 1\n",volum_size); 

    write(fd,buf1,strlen(buf1));

    /*将标签旋转到弧的当前位置*/
    lv_arc_rotate_obj_to_angle(arc, label, 25);
}

// 音量调节弧线
void lv_volume(void)       
{
    lv_obj_t * label = lv_label_create(lv_scr_act());

    /*创建音量弧线*/
    lv_obj_t *arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc, 75, 75);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270); 
    lv_arc_set_value(arc, 10);
    lv_obj_align(arc, LV_ALIGN_BOTTOM_LEFT, 55, -10);
    lv_obj_add_event_cb(arc, value_changed_cb, LV_EVENT_VALUE_CHANGED, label);

    /*首次手动更新标签*/
    lv_event_send(arc, LV_EVENT_VALUE_CHANGED, NULL);
}

// 初始化音乐列表
void init_list()
{
    //打开一个目录
    DIR * dp = opendir("music");
    if(dp == NULL) {
        perror("");
        exit(0);
    } else {
        printf("打开music目录成功\n");
    }

    //读取文件名
    while(1) 
    {
        struct dirent * msg = readdir(dp);
        if(msg == NULL) 
        {
            break;
        }
        if(msg->d_name[0] == '.') //跳过隐藏文件
        {
            continue;
        }

        if(msg->d_type == DT_REG) 
        {
            char  music_path[1024]={0};
            //拼接完整的目录名称
            sprintf(music_path, "%s/%s", "music", msg->d_name);      
            printf("music: %s\n",music_path);            

            strcpy(all_pic[in_dex],music_path);
            in_dex++; 
        }
    }

    char music[1024] = {0};
    char MUSIC[1024] = {0};

    int i = 0;
    for(i=0; i<in_dex; i++)
    {
        sprintf(MUSIC,"%s\n",all_pic[i]);
        sprintf(music,"%s%s",music,MUSIC);
    }

    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_radius(&style2, 25);

    lv_style_set_bg_opa(&style2, LV_OPA_COVER);
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_VER;
    grad.stops_count = 1;
    grad.stops[0].color = lv_palette_lighten(LV_PALETTE_BLUE,3);

    grad.stops[0].frac  = 128;
 
    lv_style_set_bg_grad(&style2, &grad);


    // 下拉菜单
    lv_obj_t *dropdown = lv_dropdown_create(lv_scr_act());
    lv_obj_align(dropdown, LV_ALIGN_TOP_RIGHT, -60, 30);
    lv_dropdown_set_options(dropdown, music);    
    lv_dropdown_set_text(dropdown, LV_SYMBOL_LIST);
    lv_obj_add_style(dropdown, &style2, 0);

    // 下拉箭头
    LV_IMG_DECLARE(img_caret_down)
    lv_dropdown_set_symbol(dropdown, &img_caret_down);
    lv_obj_set_style_transform_angle(dropdown, 1800, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_dropdown_set_selected_highlight(dropdown, false);
    lv_obj_add_event_cb(dropdown, list_handler, LV_EVENT_VALUE_CHANGED, NULL);   

    //关闭目录
    closedir(dp);
}

// 倍速播放列表
void Speed_Play()
{
    opts =  "X  1.0\n"
            "X  0.5\n"
            "X  1.5\n"
            "X  2.0";

    // lv_obj_t * dd;
    dd = lv_dropdown_create(lv_scr_act());
    lv_dropdown_set_options_static(dd, opts);
    lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
    lv_dropdown_set_symbol(dd, LV_SYMBOL_UP);
    
    lv_obj_align(dd, LV_ALIGN_BOTTOM_RIGHT, -70, -30);
    lv_obj_set_size(dd, 100, 40);
    lv_obj_add_style(dd, &style, 0);

    lv_obj_add_event_cb(dd, Speed_handler, LV_EVENT_VALUE_CHANGED, NULL);   

}

// 功能按钮
void mplayer_btn()
{
    // 播放
    lv_obj_t * label;
    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, play_handler, LV_EVENT_ALL, "Play");
    lv_obj_set_size(btn1, 50, 50);
    lv_obj_align(btn1, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_style(btn1, &style, 0);
    label = lv_label_create(btn1);
    lv_label_set_text(label, LV_SYMBOL_PAUSE);
    lv_obj_center(label);

    // 上一首  
    lv_obj_t * labe2;
    lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, "Last");
    lv_obj_set_size(btn2, 40, 40);
    lv_obj_align(btn2, LV_ALIGN_BOTTOM_MID, -100, -30);
    lv_obj_add_style(btn2, &style, 0);
    labe2 = lv_label_create(btn2);
    lv_label_set_text(labe2, LV_SYMBOL_PREV);
    lv_obj_center(labe2);

    // 下一首 
    lv_obj_t * labe3;
    lv_obj_t * btn3 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn3, event_handler, LV_EVENT_ALL, "Next");
    lv_obj_set_size(btn3, 40, 40);
    lv_obj_align(btn3, LV_ALIGN_BOTTOM_MID, 100, -30);
    lv_obj_add_style(btn3, &style, 0);
    labe3 = lv_label_create(btn3);
    lv_label_set_text(labe3, LV_SYMBOL_NEXT);
    lv_obj_center(labe3);

    // 音量键
    lv_obj_t * labe4;
    lv_obj_t * btn4 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn4, NULL, LV_EVENT_ALL, NULL);
    lv_obj_set_size(btn4, 35, 35);
    lv_obj_align(btn4, LV_ALIGN_BOTTOM_LEFT, 75, -30);
    lv_obj_add_style(btn4, &style, 0);
    labe4 = lv_label_create(btn4);
    lv_label_set_text(labe4, LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(labe4);   

    // 循环播放
    lv_obj_t * labe5;
    lv_obj_t * btn5 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn5, play_handler, LV_EVENT_ALL, "Loop");
    lv_obj_set_size(btn5, 40, 40);
    lv_obj_align(btn5, LV_ALIGN_BOTTOM_MID, -200, -30);
    lv_obj_add_style(btn5, &style, 0);
    labe5 = lv_label_create(btn5);
    lv_label_set_text(labe5, LV_SYMBOL_REFRESH);
    lv_obj_center(labe5);

    // 快进10s
    lv_obj_t * labe7;
    lv_obj_t * btn7 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn7, play_handler, LV_EVENT_ALL, "Plus");
    lv_obj_set_size(btn7, 25, 25);
    lv_obj_align(btn7, LV_ALIGN_BOTTOM_MID, 350, -90);
    lv_obj_add_style(btn7, &style, 0);
    labe7 = lv_label_create(btn7);
    lv_label_set_text(labe7, ">");
    lv_obj_center(labe7);

    // 后退10s
    lv_obj_t * labe8;
    lv_obj_t * btn8 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn8, play_handler, LV_EVENT_ALL, "Minus");
    lv_obj_set_size(btn8, 25, 25);
    lv_obj_align(btn8, LV_ALIGN_BOTTOM_MID, -350, -90);
    lv_obj_add_style(btn8, &style, 0);
    labe8 = lv_label_create(btn8);
    lv_label_set_text(labe8, "<");
    lv_obj_center(labe8);
}

// 进度条调节事件
static void progress_handler(lv_event_t * e)
{
    static lv_obj_t * slider;
    slider = lv_event_get_target(e);    

    int music_time = (int)lv_slider_get_value(slider);

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));

    char cmd[1024];
    sprintf(cmd,"seek %d 1\n",music_time);
    write(fd,cmd,strlen(cmd));
} 

// 获取时间
void get_length_time()
{
    int m, s;       // 分：秒
    m = get_length / 60;
    s = get_length % 60;
    sprintf(length_time,"%d:%d",m,s);

    int m1, s1;       // 分：秒
    m1 = get_pos / 60;
    s1 = get_pos % 60;
    sprintf(pos_time,"%d:%d",m1,s1);
}

// 进度条
void progress_label()
{
    // 进度条
    slider2 = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider2, 650, 5);
    lv_obj_align(slider2, LV_ALIGN_BOTTOM_MID, 0, -100);
    lv_obj_add_style(slider2, &style, 0);
    lv_obj_add_event_cb(slider2, progress_handler, LV_EVENT_VALUE_CHANGED, NULL);

    length_label = lv_label_create(lv_scr_act());   // 总时长
    lv_label_set_text(length_label, length_time);
    lv_obj_align(length_label, LV_ALIGN_BOTTOM_RIGHT, -70, -120); 

    length_label1 = lv_label_create(lv_scr_act());  // 时长
    lv_label_set_text(length_label1, pos_time);
    lv_obj_align(length_label1, LV_ALIGN_BOTTOM_LEFT, 70, -120); 


}

// 背景样式
void lv_style()
{
    lv_style_init(&style1);
    lv_style_set_radius(&style1, 0);

    lv_style_set_bg_opa(&style1, LV_OPA_COVER);
    static lv_grad_dsc_t grad;
    grad.dir = LV_GRAD_DIR_VER;
    grad.stops_count = 2;
    grad.stops[0].color = lv_palette_lighten(LV_PALETTE_BLUE, 4);
    grad.stops[1].color = lv_palette_lighten(LV_PALETTE_PINK,4);

    grad.stops[0].frac  = 128;
    grad.stops[1].frac  = 192;

    lv_style_set_bg_grad(&style1, &grad);

    lv_obj_t * obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(obj, &style1, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_center(obj);
}


//我的播放器
void my_play()
{
    fd=open("/pipe",O_RDWR);

    lv_style();         // 背景样式

    progress_label();  // 进度条
    my_style();       // 颜色样式
    init_list();      // 创建音乐列表
    Speed_Play();     // 倍速播放列表
    lv_volume();      // 音量调节弧线
    mplayer_btn();    // 功能按钮



}
