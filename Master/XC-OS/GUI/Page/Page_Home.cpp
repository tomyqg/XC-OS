#include "Basic/FileGroup.h"
#include "DisplayPrivate.h"
#include "Basic/TasksManage.h"
#include "APP_Type.h"
#include "BSP/BSP.h"

static lv_obj_t * appWindow;

/*主菜单视图*/
static lv_obj_t * tabviewHome;

/*容器，用于页面跳转动画*/
static lv_obj_t * contAppSw;
/*动画时间*/
#define AnimCloseTime_Default 200
static void AppSwitch_AnimClose(bool close = true, lv_coord_t x = 0, lv_coord_t y = 0,uint16_t time = AnimCloseTime_Default);

/*存放APP组的Tab*/
typedef struct
{
    const char* text;
    lv_obj_t * tab;
} AppTab_TypeDef;

/*主tab*/
static uint8_t tabHomeIndex = 1;
/*一个Tabview放三个Tab，不设定标签名*/
static AppTab_TypeDef AppTab_Grp[] =
{
    {""},
    {""},
    {""}
};

/*图片文件*/
LV_IMG_DECLARE(ImgSettings);
LV_IMG_DECLARE(ImgCode);
LV_IMG_DECLARE(ImgGame);
LV_IMG_DECLARE(ImgSubApps);
LV_IMG_DECLARE(ImgFiles);
LV_IMG_DECLARE(ImgEditor);
LV_IMG_DECLARE(ImgVideo);
LV_IMG_DECLARE(ImgMusic);
LV_IMG_DECLARE(ImgUSB);

/*APP组*/
static APP_TypeDef APP_Grp[] =
{
    {&ImgSettings, "Settings", TYPE_PageJump, PAGE_Settings},
    {&ImgCode,     "IDE",      TYPE_PageJump, PAGE_LuaScript},
    {&ImgGame,     "Game",     TYPE_PageJump, PAGE_GameSelect},
    {&ImgSubApps,  "APPs",     TYPE_PageJump, PAGE_LuaAppSel},
    {&ImgFiles,    "Files",    TYPE_PageJump, PAGE_FileExplorer},
    {&ImgVideo,    "Video",    TYPE_PageJump, PAGE_BvPlayer},
    {&ImgEditor,   "Editor",   TYPE_PageJump, PAGE_TextEditor},
    {&ImgMusic,    "Music",    TYPE_PageJump, PAGE_WavPlayer},
    {&ImgUSB,      "USB",      TYPE_PageJump, PAGE_Shell},
};

/**
  * @brief  APP图片事件
  * @param  obj:对象
  * @param  event:事件类型
  * @retval None
  */
static void ImgbtnEvent_Handler(lv_obj_t * obj, lv_event_t event)
{
    /*单击事件*/
    if(event == LV_EVENT_CLICKED)
    {
        /*扫描APP组*/
        for(int i = 0; i < __Sizeof(APP_Grp); i++)
        {
            if(obj == APP_Grp[i].imgbtn && APP_Grp[i].type == TYPE_PageJump)
            {
                /*页面跳转*/
                page.PagePush(APP_Grp[i].param);
                /*播放跳转动画*/
                AppSwitch_AnimClose(
                    false,
                    lv_obj_get_x_center(APP_Grp[i].cont),
                    lv_obj_get_y_center(APP_Grp[i].cont),
                    AnimCloseTime_Default
                );
            }
        }
    }
    if(event == LV_EVENT_LONG_PRESSED)
    {
        Motor_Vibrate(1, 20);
    }
}

/**
  * @brief  生成APP图标
  * @param  app:APP引用
  * @param  parent:放置APP图标的父对象
  * @param  index:索引0~9
  * @retval None
  */
void Creat_APP(APP_TypeDef &app, lv_obj_t * parent, uint8_t index, lv_event_cb_t imgbtn_event_handler)
{
    /*防止数组越界*/
    if(index > 8)
        return;
    
    /*创建APP图标容器*/
    app.cont = lv_cont_create(parent, NULL);
    /*样式设置为透明*/
    lv_cont_set_style(app.cont, LV_CONT_STYLE_MAIN, &lv_style_transp);

    /*创建图标按钮控件*/
    app.imgbtn = lv_imgbtn_create(app.cont, NULL);
    lv_imgbtn_set_src(app.imgbtn, LV_BTN_STATE_REL, app.img_dsc);
    lv_imgbtn_set_src(app.imgbtn, LV_BTN_STATE_PR, app.img_dsc);
    lv_imgbtn_set_toggle(app.imgbtn, false);
    /*图片被按下样式*/
    static lv_style_t style_pr = *lv_imgbtn_get_style(app.imgbtn, LV_IMGBTN_STYLE_PR);
    style_pr.image.color = LV_COLOR_GRAY;
    style_pr.image.intense = LV_OPA_70;
    lv_imgbtn_set_style(app.imgbtn, LV_IMGBTN_STYLE_PR, &style_pr);
    /*关联事件*/
    lv_obj_set_event_cb(app.imgbtn, imgbtn_event_handler);

    /*在图标下创建标签*/
    app.label = lv_label_create(app.cont, NULL);
    lv_label_set_text(app.label, app.lable_text);
    /*居中对齐*/
    lv_obj_align(app.label, app.imgbtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /*容器大小自适应*/
    lv_cont_set_fit(app.cont, LV_FIT_TIGHT);
    lv_cont_set_layout(app.cont, LV_LAYOUT_COL_M);

    /*APP容器布局*/
    #define MOD_OFFSET 20//边缘偏移量
    const lv_coord_t mod_grp[3] = {MOD_OFFSET, 0, -MOD_OFFSET};
    /*对齐模式组*/
    const lv_align_t align_grp[] = {
        LV_ALIGN_IN_TOP_LEFT,
        LV_ALIGN_IN_TOP_MID,
        LV_ALIGN_IN_TOP_RIGHT,
        LV_ALIGN_IN_LEFT_MID,
        LV_ALIGN_CENTER,
        LV_ALIGN_IN_RIGHT_MID,
        LV_ALIGN_IN_BOTTOM_LEFT,
        LV_ALIGN_IN_BOTTOM_MID,
        LV_ALIGN_IN_BOTTOM_RIGHT
    };
    lv_obj_align(
        app.cont, 
        parent, 
        align_grp[index], 
        mod_grp[index % 3], 
        mod_grp[index / 3]
    );
}

/**
  * @brief  生成存放APP组的Tab
  * @param  tabview:父对象
  * @param  apptab:APP容器引用
  * @retval None
  */
static void Creat_Tab(lv_obj_t* tabview, AppTab_TypeDef &apptab)
{
    /*添加Tab*/
    apptab.tab = lv_tabview_add_tab(tabview, apptab.text);
}

/**
  * @brief  生成Tabview页面
  * @param  tabview: tabview对象地址
  * @retval None
  */
static void Creat_Page(lv_obj_t** tabview)
{
    /*创建Tabview*/
    *tabview = lv_tabview_create(appWindow, NULL);
    
    /*设置大小为应用程序区大小*/
    lv_obj_set_size(*tabview, APP_WIN_WIDTH, APP_WIN_HEIGHT);
    
    /*与顶部对齐*/
    lv_obj_align(*tabview, appWindow, LV_ALIGN_IN_TOP_MID, 0, 0);
    
    /*设定按钮方向为在下方*/
    lv_tabview_set_btns_pos(*tabview, LV_TABVIEW_BTNS_POS_BOTTOM);
    //lv_tabview_set_btns_hidden(*tabview, true);
}

/**
  * @brief  APP切换动画控制
  * @param  close: 是否为关闭动画
  * @param  x:起始坐标X(开启动画有效)
  * @param  y:起始坐标Y(开启动画有效)
  * @param  time:动画时间(ms)
  * @retval None
  */
static void AppSwitch_AnimClose(bool close , lv_coord_t x , lv_coord_t y ,uint16_t time)
{
    /*创建动画容器*/
    if(!contAppSw)
    {
        contAppSw = lv_cont_create(appWindow, NULL);
    }
    
    /*置于顶层*/
    lv_obj_set_top(contAppSw, true);
    
    /*关闭隐藏*/
    lv_obj_set_hidden(contAppSw, false);
    
    /*动画组*/
    enum{A_X, A_Y, A_W, A_H, A_OPA, A_MAX};
    static lv_anim_t anim_grp[A_MAX];
    
    /*动画组初始化，只执行一次*/
    __ExecuteOnce((  
        /*允许调整透明度*/
        lv_obj_set_opa_scale_enable(contAppSw, true),
        
        /*对象*/
        anim_grp[0].var = contAppSw,
        /*路径*/
        anim_grp[0].path_cb  = lv_anim_path_ease_in_out,
        /*时间*/
        anim_grp[0].time = time,
        /*拷贝相同参数*/
        anim_grp[1] = anim_grp[0],
        anim_grp[2] = anim_grp[0],
        anim_grp[3] = anim_grp[0],
        anim_grp[4] = anim_grp[0],
        
        /*动画回调设定*/
        anim_grp[A_X].exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_x,
        anim_grp[A_Y].exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_y,
        anim_grp[A_W].exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_width,
        anim_grp[A_H].exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_height,
        anim_grp[A_OPA].exec_cb = (lv_anim_exec_xcb_t)lv_obj_set_opa_scale
    ));

    /*是否为关闭动画*/
    if(!close)
    {
        /*起点终点设定*/
        anim_grp[A_OPA].start = LV_OPA_TRANSP;
        anim_grp[A_OPA].end = LV_OPA_COVER;
        
        anim_grp[A_W].start = anim_grp[A_H].start = 0;
        anim_grp[A_W].end = lv_obj_get_width(appWindow);
        anim_grp[A_H].end = lv_obj_get_height(appWindow);
        
        anim_grp[A_X].start = x;
        anim_grp[A_Y].start = y;
        
        anim_grp[A_X].end = 0;
        anim_grp[A_Y].end = 0;
    }
    else
    {
        /*反转起点终点设定，倒放*/
#define ANIM_SWAP(anim) {int temp;temp=anim.start,anim.start=anim.end,anim.end=temp;}
        __LoopExecute(ANIM_SWAP(anim_grp[i]), __Sizeof(anim_grp));
    }
    
    /*应用动画组*/
    __LoopExecute(lv_anim_create(&(anim_grp[i])), __Sizeof(anim_grp));
}

static void FirstInit()
{
    /*初始化主Tabview页面*/
    Creat_Page(&tabviewHome);
    
    /*创建Tab*/
    __LoopExecute(Creat_Tab(tabviewHome, AppTab_Grp[i]), __Sizeof(AppTab_Grp));
    
    /*在主Tab创建APP组*/
    __LoopExecute(Creat_APP(APP_Grp[i], AppTab_Grp[tabHomeIndex].tab, i, ImgbtnEvent_Handler), __Sizeof(APP_Grp));
    
    /*Tab滑动至主页面*/
    lv_tabview_set_tab_act(tabviewHome, tabHomeIndex, LV_ANIM_ON);
}

/**
  * @brief  页面初始化事件
  * @param  无
  * @retval 无
  */
static void Setup()
{
    /*显示appWindow*/
    lv_obj_move_foreground(appWindow);
    lv_obj_set_hidden(appWindow, false);
    
    /*创建Tab组，只初始化一次*/
    __ExecuteOnce(FirstInit());

    /*为上一个APP退出播放动画，但是第一次不运行*/
    static bool first = true;
    if(!first)
    {
        AppSwitch_AnimClose();
        vTaskDelay(AnimCloseTime_Default);
        lv_obj_set_hidden(contAppSw, true);
    }
    first = false;//这行别改
}

/**
  * @brief  页面退出事件
  * @param  无
  * @retval 无
  */
static void Exit()
{
    /*为APP开启动画延时*/
    vTaskDelay(AnimCloseTime_Default);
    
    lv_obj_set_hidden(appWindow, true);
}

/**
  * @brief  页面事件
  * @param  event:事件编号
  * @param  param:事件参数
  * @retval 无
  */
static void Event(int event, void* param)
{
    /*获取导航栏按钮*/
    lv_obj_t * btn = (lv_obj_t*)param;
    /*单击事件*/
    if(event == LV_EVENT_CLICKED)
    {
        /*切换到主Tab*/
        if(btn == btnHome)
        {
            lv_tabview_set_tab_act(tabviewHome, tabHomeIndex, LV_ANIM_ON);
        }
    }
}

/**
  * @brief  页面注册
  * @param  pageID:为此页面分配的ID号
  * @retval 无
  */
void PageRegister_Home(uint8_t pageID)
{
    appWindow = AppWindow_GetCont(pageID);
    page.PageRegister(pageID, Setup, NULL, Exit, Event);
}
