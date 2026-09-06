# LabelEX API Documentation
> Version: 0.1.0(beta) Dev 00021  
> Language: Simplified Chinese

## Content
1. [Macro Definitions](#macro-definitions)
2. [Structs](#structs)
3. [Global Variables](#global-variables)
4. [Functions](#functions)

## Macro Definitions
#### main.cpp
```cpp
#define ID_OPEN_VIDEO 1001							//Menu ID for video opening
#define ID_OPEN_FOLDER 1002							//Menu ID for folder opening
#define ID_EXPORT_CALI 1003							//Menu ID for Caliberation Dataset exporting
#define ID_EXPORT_DATASET 1004						//Menu ID for Dataset exporting
#define ID_CONFIG_EXPORT 2001						//Menu ID for exporting configuration
#define ID_CONFIG_INTERFACE 2002					//Menu ID for ui configuration
#define ID_VERSION 3001								//Menu ID for version page
#define ID_MIT 3002									//Menu ID for license page
#define WM_USER_REFRESH_LIST (WM_USER + 100)		//Message for a thorough list refresh, when photos are added or removed
#define WM_USER_UPDATE_ITEM (WM_USER + 101)			//Message for item adding, when txt files are added or removed
```
#### PagePicture.cpp
```cpp
#define WM_USER_REFRESH_LIST (WM_USER + 100)		//Message for a thorough list refresh, when photos are added or removed
#define WM_USER_UPDATE_ITEM (WM_USER + 101)			//Message for item adding, when txt files are added or removed
#define WM_USER_STOP_MONITOR (WM_USER + 102)		//Message for stopping folder monitor
#define WM_USER_START_MONITOR (WM_USER + 103)		//(Aborted) Message for starting folder monitor
#define WM_USER_UPDATE_LISTVIEW (WM_USER + 200)		//Message for UI updating of IDC_LISTVIEW
#define WM_USER_UPDATE_PROGRESS (WM_USER + 201) 	//Message for UI updating of IDC_PROGRESS
#define WM_USER_STOP_MARQUEE (WM_USER + 301)		//Message for stopping MARQUEE of IDC_PROGRESS
#define TIMER_REFRESH_DEBOUNCE 1001					//Message for time-up of the timer 
```
#### PageAbout.cpp
```cpp
#define WM_USER_REFRESH_LIST (WM_USER + 100)		//Message for a thorough list refresh, when photos are added or removed
#define WM_USER_VIDEO_READY (WM_USER + 400)			//Message when a video is accepted by FFmpeg and is ready to convert
#define WM_USER_START_CONVERT (WM_USER + 401)		//Message when a convertion from video to photoset is starting
#define WM_USER_CONVERT_PROGRESS (WM_USER + 402)	//Message when a convertion from video to photoset is processing
#define WM_USER_CONVERT_DONE (WM_USER + 403)		//Message when a convertion from video to photoset is over
#define WM_USER_STOP_MONITOR (WM_USER + 102)		//Message for stopping folder monitor
#define WM_USER_START_MONITOR (WM_USER + 103)		//(Aborted) Message for starting folder monitor
```
#### PageExport.cpp
```cpp
#define WM_USER_UPDATE_PROGRESS (WM_USER + 403)		//Message for UI updating of IDC_VIDEOPROGRESS
#define WM_USER_BUILD_DONE (WM_USER + 404)			//Message when a Dataset is successfully built
```

## Structs
#### PagePicture.cpp
```cpp
struct BBox {
	int left, top, right, bottom;	//The basic specs of your drawn rectangle
	int classId;					//The Class ID of the circled object
};

struct ImageFileInfo {
	std::wstring fileName;			//The filename in the IDC_LISTVIEW
	BOOL status;					//The status in the IDC_LISTVIEW
};
```
#### PageVideo.cpp
```cpp
struct ThreadParams
{
	std::wstring szVideoPath;		//Video's path
	std::wstring szImagePath;		//Image's path
	int format;						//Image's format
	int fps;						//Frames extracted from video per second
	int quality;					//Image's quality (when converted to .jpg files)
	HWND hDlg;						//Parent dialog's handle
	HWND hPagePicture;				//PagePicture's handle
};
```
#### PageExport.cpp
```cpp
struct SplitParams
{
	std::vector<std::wstring> imgPaths;			//Image's path (in the current directory)
	std::vector<std::wstring> labelPaths;		//Label's path (in the current directory)
	std::wstring trainImgPath;					//paths for images that would be used for training
	std::wstring valImgPath;					//paths for images that would be used for validation
	std::wstring trainLabelPath;				//paths for labels that would be used for training
	std::wstring valLabelPath;					//paths for labels that would be used for validation
	double ratio;								//ratio of training images/labels
	HWND hDlg;									//Parent dialog's handle
	BOOL bType;									//type of the processing
};
```

## Global Variables
#### main.cpp
```cpp
static wchar_t szWindowClass[] = L"LEX";
static wchar_t szTitle[] = L"LabelEX";
HINSTANCE hInst;
HANDLE hExitEvent = NULL;					//退出事件句柄（用于结束子进程FolderMonitorThread）
HANDLE hMonitorThread = NULL;				//子进程FolderMonitorThread事件句柄
HWND hPagePicture;							//Picture页面句柄
```
#### PagePicture.cpp
```cpp
Bitmap* pCurrentImage = nullptr;					//打开的图片句柄
HWND hImageCtrl = nullptr;							//图片编辑区句柄（Button控件）
HWND hProgressDlg = nullptr;						//处理窗口句柄
std::vector<BBox> bboxes;							//矩形集合
int currentClassId = 0;								//当前的物体的类别
int selectedIndex = -1;								//选中的矩形索引
int threshold = 6;									//画笔与矩形宽度							
WNDPROC oldPicProc = NULL;							//子类化前原有的处理过程
enum DragMode {None, Moving, Resizing, Creating};	//鼠标拖拽模式
DragMode dragMode = None;
int resizeHandle = -1;								//手柄
POINT dragStart;									//起始拖拽坐标
POINT dragOffset;									//拖拽偏移量								
wchar_t szFolderPath[MAX_PATH] = { 0 };				//当前文件夹目录
std::wstring currentImagePath;						//当前图片目录
BOOL isProcessExist = false;						//是否正在处理
BOOL isPendingRefresh = false;						//处理过程中是否有新的
```
#### PageVideo.cpp
```cpp
wchar_t szVideoPath[MAX_PATH] = { 0 };				//视频路径
HWND hVideoProgress = NULL;							//视频处理窗口句柄
int videoCount;										//视频计数
```
#### PageExport.cpp
```cpp
HWND hDsProcessDlg;									//数据集处理窗口句柄
```

## Functions
#### main.cpp
```cpp
HWND DoCreateMenu(HWND hWnd)
```
菜单栏创建函数，此函数由IHC项目贡献
- 简介：创建菜单栏
- 参数：父窗口句柄`hWnd`
- 返回：任何时候都返回`0`
```cpp
BOOL DoCreateDialog(HWND hWnd, HWND* hPagePicture)
```
对话框（页面）创建函数，此函数由IHC项目贡献
- 简介：创建子页面句柄hPagePicture，以布置静态控件
- 参数：父窗口句柄`hWnd`，子窗口句柄指针`hPagePicture`
- 返回：创建失败时返回`FALSE`，创建成功时返回`TRUE`
```cpp
DWORD WINAPI FolderMonitorThread(LPVOID lpParam)
```
文件夹监视子进程
- 简介：通过`CreateFile()`打开选中的文件夹目录，通过线程循环监视文件夹内txt和图片文件，当txt出现变动时触发`UpdateSingleItemStatus()`，当图片出现变动时触发`RefreshList()`
- 参数：通用指针`lpParam`传递`StartFolderMonitor(HWND hWnd)`中的`hWnd`值
- 返回：任何时候都返回`0`
```cpp
BOOL StartFolderMonitor(HWND hDlg)
```
启动文件夹监视子进程
- 简介：先停止上一个文件夹监视子进程，然后再创建新的监视子进程
- 参数：父窗口句柄`hDlg`
- 返回：创建失败时返回`FALSE`，创建成功时返回`TRUE`
```cpp
void StopFolderMonitor()
```
关闭文件夹监视子进程
- 简介：当外界调用此函数时，设置hExitEvent为有信号使得FolderMonitorThread停止循环并退出，等待2秒后关闭句柄
```cpp
int IDCForDpi(HWND hWnd, int oldIDC)
```
按DPI设置控件大小，此函数由IHC项目贡献
- 简介：当外界调用此函数时，自动按dpi重新计算控件大小
- 参数：父窗口句柄`hWnd`，当前控件大小`oldIDC`
- 返回：新控件大小`newIDC`

#### PagePicture.cpp
```cpp
void DoSelectFolder(HWND hWnd)
```
通过打开的窗口选择文件夹，此函数来自Win32 SDK
- 简介：调用此函数，打开选择文件夹的窗口
- 参数：父窗口句柄`hWnd`
```cpp
DWORD WINAPI RefreshListThread(LPVOID lpParam)
```
全量刷新列表（文件）
- 简介：加载文件夹内的图片与标注到`fileList`
- 参数：父窗口上下文`lpParam`
- 返回：任何时候都返回`0`
```cpp
void RefreshListUI(HWND hList, const std::vector<ImageFileInfo>& fileList)
```
全量刷新列表（UI）
- 简介：将fileList中的图片与标注加载到PICTUREVIEW并更新
- 参数：LISTVIEW句柄，文件列表`fileList`
```cpp
void UpdateSingleItemStatus(HWND hDlg, LPCWSTR szBaseName, BOOL bExist)
```
刷新列表状态信息
- 简介：当标签文件发生改变时，修改列表状态
- 参数：窗口句柄`hDlg`，无拓展名文件路径`szBaseName`，文件状态`bExist`
```cpp
BOOL IsImageFile(LPCWSTR szExt)
```
判断是否是图片
- 简介：通过比较拓展名，判断是否是图片
- 参数：文件拓展名`szExt`
- 返回：是图片则返回`TRUE`，不是则返回`FALSE`
```cpp
BOOL DoCreateListView(HWND hWnd)
```
创建ListView组件（创建列表）
- 简介：创建一个列表，此处为文件夹内图片的列表
- 参数：父窗口句柄`hWnd`
- 返回：创建失败时返回`FALSE`，创建成功时返回`TRUE`
```cpp
void LoadImageToDisplay(LPCWSTR szFilePath)
```
加载图片与标签文件
- 简介：通过Bitmap从路径szFilePath加载图片，并调用`LoadBBoxesFromFile()`加载标签
- 参数：图片路径`szFilePath`
```cpp
void SelectImageByIndex(HWND hList, int index)
```
从索引加载列表图片
- 简介：直接指定图片在列表中的索引，并使得`ListView`显示对应索引的强调色
- 参数：列表句柄`hList`，索引`index`
```cpp
void SelectNextImage(HWND hList)
```
加载下一张图片
- 简介：调用`SelectImageByIndex()`当图片为最后一张时，恢复索引为第1张图片（`Index = 0`）；其余情况加载下一张图片
- 参数：列表句柄`hList`
```cpp
POINT ControlToImage(POINT ptCtrl)
```
控件-图片坐标转换
- 简介：将在控件区的绘制坐标转换为实际图片上的坐标，并判断结果大于0的同时小于图片最大宽度/高度
- 参数：绘制坐标`ptCtrl`
- 返回：图片坐标`result`
```cpp
RECT ImageToControl(const BBox& box)
```
图片-控件坐标转换
- 简介：将存储在bboxes中的图片矩形坐标特征转换为控件区的绘制坐标
- 参数：图片矩形特征类`box`
- 返回：绘图矩形坐标特征`rc`
```cpp
void DrawHandles(Graphics& graphics, const RECT& rc)
```
绘制手柄
- 简介：在选中的矩形上绘制8个方向的手柄
- 参数：绘图类`graphics`，选中的矩形坐标特征`rc`
```cpp
void ClampRect(BBox& box, int minX, int minY, int maxX, int maxY)
```
坐标限制
- 简介：将绘制坐标限制在图像边界内
- 参数：矩形特征类`box`，边界值的最小值与最大值
```cpp
int HitTestHandle(HWND hWnd, POINT ptCtrl)
```
手柄命中
- 简介：鼠标选中某个手柄，返回该手柄的索引
- 参数：父窗口句柄`hWnd`，绘制坐标`ptCtrl`
- 返回：手柄索引`ptCtrl`
```cpp
void SaveBBoxesToFile(HWND hWnd, const std::vector<BBox>& boxes, int imgWidth, int imgHeight, std::wstring& filePath) 
```
保存矩形特征类集合到标签文件
- 简介：保存当前屏幕的的矩形特征类集合到标签文件
- 参数：父窗口句柄`hWnd`，矩形特征类集合`boxes`，图片宽度`imgWidth`，图片高度`imgHeight`，文件路径`filePath`
```cpp
void LoadBBoxesFromFile(const std::wstring& filePath, int imgWidth, int imgHeight)
```
从标签文件提取矩形特征类集合
- 简介：从当前图片对应的标签文件提取矩形特征类集合
- 参数：文件路径`filePath`，图片宽度`imgWidth`，图片高度`imgHeight`
```cpp
Color GetClassColor(int classId)
```
选择颜色
- 简介：画笔选择颜色用以绘制不同的类的矩形
- 参数：类`classId`
- 返回：颜色类`

#### PageVideo.cpp
```cpp
bool SaveFrameAsPNG(AVFrame* pFrameBGR, const wchar_t* filename)
```
将帧转换为PNG文件
- 简介：采用`AV_PIX_FMT_RGB24`编码保存
- 参数：帧`pFrameBGR`，文件名称`filename`
- 返回：成功则返回`TRUE`，失败则返回`FALSE`
```cpp
bool SaveFrameAsJPEG(AVFrame* pFrame, const wchar_t* filename, int quality)
```
将帧转换为JPEG文件
- 简介：采用`AV_PIX_FMT_YUVJ420P`编码保存
- 参数：帧`pFrame`，文件名称`filename`，质量`quality`
- 返回：成功则返回`TRUE`，失败则返回`FALSE`
```cpp
void DoSelectVideo(HWND hWnd)
```
通过打开的窗口选择视频，此函数来自Win32 SDK
- 简介：调用此函数，打开选择视频的窗口
- 参数：父窗口句柄`hWnd`
```cpp
void DoSelectImageFolder(HWND hWnd)
```
通过打开的窗口选择图片集文件夹，此函数来自Win32 SDK
- 简介：调用此函数，打开选择图片的窗口
- 参数：父窗口句柄`hWnd`
```cpp
void DoAnalyseVideo(const wchar_t* szFilePath)
```
使用FFmpeg分析视频的信息
- 简介：通过FFmpeg动态链接库分析视频的格式，分辨率和刷新率，并展示在EDITCONTROL中
- 参数：文件路径`szFilePath`
```cpp
DWORD WINAPI DoConvertVideo(LPVOID lpParam)
```
使用FFmpeg将视频帧按照指定帧率抽出并转换为图片
- 简介：通过FFmpeg动态链接库解析视频，保存指定帧到指定的文件夹
- 参数：父窗口上下文`lpParam`
- 返回：任何时候都返回`0`

#### PageExport.cpp
```cpp
DWORD WINAPI BuildDataset(LPVOID lpParam)
```
将图片与标注文件构成数据集
- 简介：通过Fisher Yates算法，将图片与数据集按照指定比例分配到不同的数据集文件夹中，构建YOLO格式的数据集
- 参数：父窗口上下文`lpParam`
- 返回：任何时候都返回`0`