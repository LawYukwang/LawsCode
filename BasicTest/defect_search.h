// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DEFECT_SEARCH_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DEFECT_SEARCH_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef DEFECT_SEARCH_EXPORTS
#define DEFECT_SEARCH_API __declspec(dllexport)
#else
#define DEFECT_SEARCH_API __declspec(dllimport)
#endif

void module_init(void);
void module_exit(void);

#define SUCCESS 0
#define FAIL	(-1)

#define IMAGE_SIZE_MAX    196608000 //8192 * 8000 * 3
#define IMAGE_COUNT       5

//以下定义需要导出的函数列表
extern "C" DEFECT_SEARCH_API int yb_defect_init(void);
extern "C" DEFECT_SEARCH_API void yb_defect_pic_set(char *pic_templet, char *pic_path);
extern "C" DEFECT_SEARCH_API void yb_defect_pic_skip_set(int left_skip, int right_skip, int up_skip, int bottm_skip);
extern "C" DEFECT_SEARCH_API void yb_defect_area_min(int pixel_area);
extern "C" DEFECT_SEARCH_API void yb_defect_phase_set(int phase);

/* 单个疵点信息结构,单字节对齐 */
#pragma pack(push, 1)
typedef struct
{
	int		defect_type;         /* 疵点类型，暂不用。后续需要使用 */
	int		defectX_onPic;       /* 疵点在图片上的横坐标x(像素) */
	int		defectY_onPic;       /* 疵点在图片上的纵坐标y(像素) */
	int		defect_width;        /* 疵点宽度(像素) */
	int		defect_height;       /* 疵点长度(像素) */
	time_t	time_t;               /*疵点检测出来的时间, time_t */
} yb_defect_s;
#pragma pack(pop)

//图像数据信息结构,单字节对齐
#pragma pack(push, 1)
typedef struct 
{
	int width;
	int height;
	int dataSize;
	unsigned char* pData;
} yb_image_data_s;
#pragma pack(pop)

//定义函数指针
//最后修改为回调图像指针地址
typedef void (__stdcall *defect_notify)(yb_image_data_s *image_data, yb_defect_s *defect_array, int defect_cnt);
extern "C" DEFECT_SEARCH_API void yb_defect_notify_register(defect_notify);
extern "C" DEFECT_SEARCH_API void yb_defect_start(void);
extern "C" DEFECT_SEARCH_API void yb_defect_stop(void);
extern "C" DEFECT_SEARCH_API void yb_defect_exit(void);



typedef void (__stdcall *image_data_notify)(yb_image_data_s *image_data);
extern "C" DEFECT_SEARCH_API void yb_defect_image_notify_register(image_data_notify);

//相机采集回调函数
typedef void (_stdcall *sj_notify)(int n,void* pData,int width,int heght,int DataSize);
//相机采集通告注册函数
typedef void (_stdcall *sj_notify_register)(sj_notify sjNotify);