#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_VIDEO_READY (WM_USER + 400)
#define WM_USER_START_CONVERT (WM_USER + 401)
#define WM_USER_CONVERT_PROGRESS (WM_USER + 402)
#define WM_USER_CONVERT_DONE (WM_USER + 403)
#define WM_USER_STOP_MONITOR (WM_USER + 102)
#define WM_USER_START_MONITOR (WM_USER + 103)

wchar_t szVideoPath[MAX_PATH] = { 0 };
HWND hVideoProgress = NULL;

struct ThreadParams
{
	std::wstring szVideoPath;
	std::wstring szImagePath;
	int format;
	int fps;
	int quality;
	HWND hDlg;
	HWND hPagePicture;
};

bool SaveFrameAsPNG(AVFrame* pFrameBGR, const wchar_t* filename)
{
	if (!pFrameBGR || !pFrameBGR->data[0]) return false;

	const AVCodec* pCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	if (!pCodec) return false;

	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) return false;

	pCodecCtx->width = pFrameBGR->width;
	pCodecCtx->height = pFrameBGR->height;
	pCodecCtx->time_base = AVRational{ 1, 25 };

	pCodecCtx->pix_fmt = AV_PIX_FMT_RGB24;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	AVFrame* pFrameRGBA = av_frame_alloc();
	pFrameRGBA->format = pCodecCtx->pix_fmt;
	pFrameRGBA->width = pCodecCtx->width;
	pFrameRGBA->height = pCodecCtx->height;
	av_frame_get_buffer(pFrameRGBA, 32);

	SwsContext* swsCtx = sws_getContext(
		pFrameBGR->width, pFrameBGR->height, AV_PIX_FMT_BGR24,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
		SWS_LANCZOS, NULL, NULL, NULL);

	sws_scale(swsCtx, (const uint8_t* const*)pFrameBGR->data, pFrameBGR->linesize,
		0, pFrameBGR->height, pFrameRGBA->data, pFrameRGBA->linesize);

	sws_freeContext(swsCtx);

	if (avcodec_send_frame(pCodecCtx, pFrameRGBA) != 0) {
		av_frame_free(&pFrameRGBA);
		avcodec_free_context(&pCodecCtx);
		return false;
	}
	AVPacket* pPacket = av_packet_alloc();
	if (avcodec_receive_packet(pCodecCtx, pPacket) != 0) {
		av_packet_free(&pPacket);
		av_frame_free(&pFrameRGBA);
		avcodec_free_context(&pCodecCtx);
		return false;
	}
	FILE* pFile = _wfopen(filename, L"wb");
	if (!pFile) {
		av_packet_free(&pPacket);
		av_frame_free(&pFrameRGBA);
		avcodec_free_context(&pCodecCtx);
		return false;
	}
	fwrite(pPacket->data, 1, pPacket->size, pFile);
	fclose(pFile);

	av_packet_free(&pPacket);
	av_frame_free(&pFrameRGBA);
	avcodec_free_context(&pCodecCtx);
	return true;
}

bool SaveFrameAsJPEG(AVFrame* pFrame, const wchar_t* filename, int quality)
{
	if (!pFrame || !pFrame->data[0]) return false;

	const AVCodec* pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	if (!pCodec) return false;

	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) return false;

	pCodecCtx->width = pFrame->width;
	pCodecCtx->height = pFrame->height;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->time_base = AVRational{ 1, 25 };

	pCodecCtx->qcompress = (float)quality / 100.0f;
	pCodecCtx->qmin = 2;
	pCodecCtx->qmax = 31;
	pCodecCtx->max_qdiff = 3;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	AVFrame* pEncodeFrame = av_frame_alloc();
	pEncodeFrame->format = pCodecCtx->pix_fmt;
	pEncodeFrame->width = pCodecCtx->width;
	pEncodeFrame->height = pCodecCtx->height;
	if (av_frame_get_buffer(pEncodeFrame, 32) < 0) {
		av_frame_free(&pEncodeFrame);
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	SwsContext* swsCtx = sws_getContext(
		pFrame->width, pFrame->height, AV_PIX_FMT_BGR24,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUVJ420P,
		SWS_LANCZOS, NULL, NULL, NULL);

	if (!swsCtx) {
		av_frame_free(&pEncodeFrame);
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	sws_scale(swsCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize,
		0, pFrame->height, pEncodeFrame->data, pEncodeFrame->linesize);

	sws_freeContext(swsCtx);

	if (avcodec_send_frame(pCodecCtx, pEncodeFrame) != 0) {
		av_frame_free(&pEncodeFrame);
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	AVPacket* pPacket = av_packet_alloc();
	if (avcodec_receive_packet(pCodecCtx, pPacket) != 0) {
		av_packet_free(&pPacket);
		av_frame_free(&pEncodeFrame);
		avcodec_free_context(&pCodecCtx);
		return false;
	}

	FILE* pFile = _wfopen(filename, L"wb");
	if (!pFile) {
		av_packet_free(&pPacket);
		av_frame_free(&pEncodeFrame);
		avcodec_free_context(&pCodecCtx);
		return false;
	}
	fwrite(pPacket->data, 1, pPacket->size, pFile);
	fclose(pFile);

	av_packet_free(&pPacket);
	av_frame_free(&pEncodeFrame);
	avcodec_free_context(&pCodecCtx);
	return true;
}

void DoSelectVideo(HWND hWnd)
{
	CoInitialize(NULL);
	IFileOpenDialog *pDialog = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
		IID_IFileOpenDialog, (void**)&pDialog
	);
	if (SUCCEEDED(hr))
	{
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"Supported Video Files", L"*.mp4;*.avi;*.mkv;*.mov;*.wmv;*.flv;*.webm;*.m4v;*.mpg;*.mpeg;*.3gp;*.ts;*.vob" },
			{ L"All Files (*.*)", L"*.*" }
		};
		pDialog->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pDialog->SetFileTypeIndex(1);

		DWORD opts;
		pDialog->GetOptions(&opts);
		opts = (opts & ~FOS_PICKFOLDERS) | FOS_FILEMUSTEXIST;
		pDialog->SetOptions(opts);
		hr = pDialog->Show(hWnd);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem = NULL;
			pDialog->GetResult(&pItem);
			if (pItem)
			{
				PWSTR pszPath = NULL;
				pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (pszPath)
				{
					wcscpy_s(szVideoPath, pszPath);
					PostMessage(hPageVideo, WM_USER_VIDEO_READY, 0, 0);
					CoTaskMemFree(pszPath);
				}
				pItem->Release();
			}
		}
		pDialog->Release();
	}
	CoUninitialize();
}

void DoSelectImageFolder(HWND hWnd)
{
	CoInitialize(NULL);
	IFileOpenDialog *pDialog = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, (void**)&pDialog
	);
	if (SUCCEEDED(hr))
	{
		DWORD opts;
		pDialog->GetOptions(&opts);
		pDialog->SetOptions(opts | FOS_PICKFOLDERS);
		hr = pDialog->Show(hWnd);
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem = NULL;
			pDialog->GetResult(&pItem);
			if (pItem)
			{
				PWSTR pszPath = NULL;
				pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (pszPath)
				{
					wcscpy_s(szFolderPath, pszPath);
					CoTaskMemFree(pszPath);
				}
				pItem->Release();
			}
		}
		pDialog->Release();
	}
	CoUninitialize();
}

void DoAnalyseVideo(const wchar_t* szFilePath)
{
	if (!szFilePath)
	{
		return;
	}
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, szFilePath, -1, NULL, 0, NULL, NULL);
	if (size_needed == 0)
	{
		return;
	}
	std::string utf8Path(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, szFilePath, -1, &utf8Path[0], size_needed, NULL, NULL);
	AVFormatContext* pFormatContext = avformat_alloc_context();
	if (avformat_open_input(&pFormatContext, utf8Path.c_str(), NULL, NULL) != 0)
	{
		MessageBox(NULL, L"FFmpeg failed to open the video", L"Error", NULL);
		return;
	}
	if (avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		MessageBox(NULL, L"FFmpeg failed to find stream info", L"Error", NULL);
		avformat_close_input(&pFormatContext);
		return;
	}
	int videoStreamIndex = -1;
	for (int i = 0; i < pFormatContext->nb_streams; ++i)
	{
		if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = i;
			break;
		}
	}
	if (videoStreamIndex == -1)
	{
		MessageBox(NULL, L"FFmpeg failed to find stream", L"Error", NULL);
		avformat_close_input(&pFormatContext);
		return;
	}
	AVStream* videoStream = pFormatContext->streams[videoStreamIndex];
	AVCodecParameters* pCodecParams = videoStream->codecpar;
	const char* codecName = avcodec_get_name(pCodecParams->codec_id);
	SetDlgItemTextA(hPageVideo, IDC_SOURCE_FORM, codecName);
	wchar_t sourceRes[20];
	StringCchPrintf(sourceRes, _countof(sourceRes), L"%dx%d", pCodecParams->width, pCodecParams->height);
	SetDlgItemText(hPageVideo, IDC_SOURCE_RES, (LPCWSTR)(sourceRes));
	AVRational frameRate = videoStream->avg_frame_rate;
	double fps = (frameRate.den != 0) ? (double)frameRate.num / frameRate.den : 0.0;
	wchar_t buffer[64];
	swprintf_s(buffer, _countof(buffer), L"%.2f", fps);
	SetDlgItemText(hPageVideo, IDC_SOURCE_FPS, (LPCWSTR)(buffer));
	avformat_close_input(&pFormatContext);
}

DWORD WINAPI DoConvertVideo(LPVOID lpParam)
{
	ThreadParams* params = (ThreadParams*)lpParam;
	if (params->szVideoPath.empty())
	{
		return 0;
	}
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, params->szVideoPath.c_str(), -1, NULL, 0, NULL, NULL);
	if (size_needed == 0)
	{
		return 0;
	}
	std::string utf8Path(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, params->szVideoPath.c_str(), -1, &utf8Path[0], size_needed, NULL, NULL);
	AVFormatContext* pFormatContext = avformat_alloc_context();
	if (avformat_open_input(&pFormatContext, utf8Path.c_str(), NULL, NULL) != 0)
	{
		MessageBox(NULL, L"FFmpeg failed to open the video", L"Error", NULL);
		return 0;
	}
	if (avformat_find_stream_info(pFormatContext, NULL) < 0)
	{
		MessageBox(NULL, L"FFmpeg failed to find stream info", L"Error", NULL);
		avformat_close_input(&pFormatContext);
		return 0;
	}
	int videoStreamIndex = -1;
	for (int i = 0; i < pFormatContext->nb_streams; ++i)
	{
		if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = i;
			break;
		}
	}
	if (videoStreamIndex == -1)
	{
		MessageBox(NULL, L"FFmpeg failed to find stream", L"Error", NULL);
		avformat_close_input(&pFormatContext);
		return 0;
	}
	AVStream* videoStream = pFormatContext->streams[videoStreamIndex];
	AVCodecParameters* pCodecParams = videoStream->codecpar;
	const AVCodec* pCodec = avcodec_find_decoder(pCodecParams->codec_id);
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pCodecParams);
	avcodec_open2(pCodecCtx, pCodec, NULL);

	AVFrame* pFrame = av_frame_alloc();
	AVPacket* pPacket = av_packet_alloc();

	struct SwsContext* swsCtx = sws_getContext(
		pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_LANCZOS, NULL, NULL, NULL
	);

	AVFrame* pFrameBGR = av_frame_alloc();
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height, 1);
	uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(pFrameBGR->data, pFrameBGR->linesize, buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height, 1);

	pFrameBGR->width = pCodecCtx->width;
	pFrameBGR->height = pCodecCtx->height;
	pFrameBGR->format = AV_PIX_FMT_BGR24;

	double totalDurationSec = pFormatContext->duration / (double)AV_TIME_BASE;
	double interval = 1.0 / (double)(params->fps);
	double nextSaveTime = 0.0;

	int frameCount = 0;
	int imageCount = 0;

	while (av_read_frame(pFormatContext, pPacket) >= 0)
	{
		if (pPacket->stream_index == videoStreamIndex)
		{
			if (avcodec_send_packet(pCodecCtx, pPacket) == 0)
			{
				while (avcodec_receive_frame(pCodecCtx, pFrame) == 0)
				{
					double currentTime = 0.0;
					if (pFrame->pts != AV_NOPTS_VALUE)
					{
						currentTime = pFrame->pts * av_q2d(videoStream->time_base);
					}
					else
					{
						currentTime = (double)frameCount / (double)params->fps;
					}
					if (currentTime >= nextSaveTime)
					{
						sws_scale(swsCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data, pFrameBGR->linesize);
						wchar_t outPath[MAX_PATH];
						if (params->format == 0)
						{
							swprintf_s(outPath, L"%ws\\frame_%05d.png", params->szImagePath.c_str(), imageCount);
							SaveFrameAsPNG(pFrameBGR, outPath);
							imageCount++;
						}
						else if (params->format == 1)
						{
							swprintf_s(outPath, L"%ws\\frame_%05d.jpg", params->szImagePath.c_str(), imageCount);
							SaveFrameAsJPEG(pFrameBGR, outPath, params->quality);
							imageCount++;
						}
						nextSaveTime += interval;
					}
					int progress = (int)((currentTime / totalDurationSec) * 100);
					if (progress > 100)
					{
						progress = 100;
					}
					PostMessage(params->hDlg, WM_USER_CONVERT_PROGRESS, progress, 0);
					av_frame_unref(pFrame);
				}
			}
		}
		av_packet_unref(pPacket);
	}
	av_frame_free(&pFrame);
	av_frame_free(&pFrameBGR);
	av_packet_free(&pPacket);
	avcodec_free_context(&pCodecCtx);
	sws_freeContext(swsCtx);
	av_free(buffer);
	avformat_close_input(&pFormatContext);
	PostMessage(params->hDlg, WM_USER_CONVERT_DONE, 0, 0);
	delete params;
	return 0;
}

INT_PTR CALLBACK DlgProc_VideoProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		hVideoProgress = hDlg;
		int baseWidth = 280;
		int baseHeight = 100;
		int scaledWidth = IDCForDpi(hDlg, baseWidth);
		int scaledHeight = IDCForDpi(hDlg, baseHeight);
		RECT rcParent;
		HWND hParent = GetParent(hDlg);
		hParent && GetWindowRect(hParent, &rcParent);
		int x = rcParent.left + (rcParent.right - rcParent.left - scaledWidth) / 2;
		int y = rcParent.top + (rcParent.bottom - rcParent.top - scaledHeight) / 2;
		SetWindowPos(hDlg, NULL, x, y, scaledWidth, scaledHeight, SWP_NOZORDER);
		ThreadParams* params = new ThreadParams();
		params->format = SendMessage(GetDlgItem(hPageVideo, IDC_EXPORT_FORM), CB_GETCURSEL, 0, 0);
		
		params->fps = GetDlgItemInt(hPageVideo, IDC_EXPORT_FPS, NULL, FALSE);
		params->quality = GetDlgItemInt(hPageVideo, IDC_EXPORT_QUALITY, NULL, FALSE);
		wchar_t srcBuffer[MAX_PATH] = { 0 };
		GetDlgItemText(hPageVideo, IDC_SOURCE, srcBuffer, MAX_PATH);
		params->szVideoPath = srcBuffer;
		wchar_t dirBuffer[MAX_PATH] = { 0 };
		GetDlgItemText(hPageVideo, IDC_EXPORT_DIR, dirBuffer, MAX_PATH);
		params->szImagePath = dirBuffer;
		params->hDlg = hVideoProgress;
		params->hPagePicture = hPagePicture;
		HANDLE hThread = CreateThread(NULL, 0, DoConvertVideo, params, 0, NULL);
		if (hThread)
		{
			CloseHandle(hThread);
		}
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		SetWindowPos(GetDlgItem(hDlg, IDC_PROGRESS), NULL, rcDlg.left + 2 * margin, rcDlg.top + 2 * margin, rcDlg.right - rcDlg.left - 4 * margin, rcDlg.bottom - rcDlg.top - 4 * margin, SWP_NOZORDER);
		return TRUE;
	}
	case WM_USER_CONVERT_PROGRESS:
	{
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
		return TRUE;
	}
	case WM_USER_CONVERT_DONE:
	{
		if (szFolderPath[0] != 0)
		{
			StartFolderMonitor(hPagePicture);
			SendMessage(hPagePicture, WM_USER_REFRESH_LIST, 0, 0);
		}
		EndDialog(hDlg, IDOK);
		return TRUE;
	}
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProc_Video(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"‘§¥¶¿Ì");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_FORM), CB_ADDSTRING, 0, (LPARAM)L".png");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_FORM), CB_ADDSTRING, 0, (LPARAM)L".jpg");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_FORM), CB_SETCURSEL, 0, 0);
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), CB_ADDSTRING, 0, (LPARAM)L"100");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), CB_ADDSTRING, 0, (LPARAM)L"75");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), CB_ADDSTRING, 0, (LPARAM)L"50");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), CB_ADDSTRING, 0, (LPARAM)L"25");
		SendMessage(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), CB_SETCURSEL, 0, 0);
		EnableWindow(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_ST_EXPORT_QUALITY), FALSE);

		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT fontHeight = IDCForDpi(hDlg, 19);

		UINT firstColumnLeft = 2 * margin;
		UINT secondColumnLeft = 17 * margin;
		UINT thirdColumnLeft = 32 * margin;
		UINT firstRowTop = 2 * margin;
		UINT secondRowTop = 5 * margin;
		UINT thirdRowTop = 8 * margin;
		UINT fourthRowTop = 11 * margin;
		UINT fifthRowTop = 14 * margin;
		UINT sixthRowTop = 17 * margin;
		UINT seventhRowTop = 21 * margin;
		UINT eighthRowTop = 24 * margin;

		HFONT hFont = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft Yahei UI"));

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE), NULL, firstColumnLeft, firstRowTop + 2 * minLen, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE), NULL, firstColumnLeft, secondRowTop, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SELECT_SOURCE), NULL, 51 * margin + 2 * minLen, secondRowTop, 5 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_INFO), NULL, firstColumnLeft, thirdRowTop + 2 * minLen, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_FORM), NULL, firstColumnLeft, fourthRowTop, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_FORM), NULL, firstColumnLeft + 5 * margin, fourthRowTop - 2 * minLen, 10 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_RES), NULL, secondColumnLeft + 1 * margin, fourthRowTop, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_RES), NULL, secondColumnLeft + 7 * margin, fourthRowTop - 2 * minLen, 12 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_FPS), NULL, thirdColumnLeft + 5 * margin, fourthRowTop, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_FPS), NULL, thirdColumnLeft + 10 * margin, fourthRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_DIR), NULL, firstColumnLeft, fifthRowTop + 2 * minLen, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_DIR), NULL, firstColumnLeft, sixthRowTop, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SELECT_EXPORT), NULL, 51 * margin + 2 * minLen, sixthRowTop, 5 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_FORM), NULL, firstColumnLeft, seventhRowTop, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_FORM), NULL, firstColumnLeft + 7 *margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_QUALITY), NULL, secondColumnLeft, seventhRowTop, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), NULL, secondColumnLeft + 7 * margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_FPS), NULL, thirdColumnLeft, seventhRowTop, 11 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_FPS), NULL, thirdColumnLeft + 11 * margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT), NULL, rcDlg.right - 9 * margin, eighthRowTop, 7 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_CALI), NULL, firstColumnLeft, eighthRowTop, 11 * margin, 3 * margin, SWP_NOZORDER);

		SendMessage(GetDlgItem(hDlg, IDC_ST_SOURCE), WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(GetDlgItem(hDlg, IDC_ST_SOURCE_INFO), WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(GetDlgItem(hDlg, IDC_ST_EXPORT_DIR), WM_SETFONT, (WPARAM)hFont, TRUE);
		return TRUE;
	}
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_SELECT_SOURCE:
		{
			DoSelectVideo(hDlg);
			DoAnalyseVideo(szVideoPath);
			return TRUE;
		}
		case IDC_EXPORT_FORM:
		{
			int selIndex = (int)SendMessage(GetDlgItem(hDlg, IDC_EXPORT_FORM), CB_GETCURSEL, 0, 0);
			if (selIndex == 1)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_ST_EXPORT_QUALITY), TRUE);
			}
			else if (selIndex == 0)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_ST_EXPORT_QUALITY), FALSE);
			}
			return TRUE;
		}
		case IDC_SELECT_EXPORT:
		{
			DoSelectImageFolder(hPageVideo);
			SetDlgItemText(hDlg, IDC_EXPORT_DIR, (LPWSTR)szFolderPath);
			return TRUE;
		}
		case IDC_EXPORT:
		{
			wchar_t srcBuffer[MAX_PATH] = { 0 };
			if (GetDlgItemText(hPageVideo, IDC_SOURCE, srcBuffer, MAX_PATH) == 0)
			{
				MessageBox(NULL, L"No source file selected", L"error", NULL);
				return 0;
			}
			if (GetDlgItemInt(hPageVideo, IDC_SOURCE_FPS, NULL, FALSE) <= 0)
			{
				MessageBox(NULL, L"Source file doesn't have an effective fps!", L"error", NULL);
				return 0;
			}
			int fps = GetDlgItemInt(hPageVideo, IDC_EXPORT_FPS, NULL, FALSE);
			if (fps <= 0 || fps > GetDlgItemInt(hPageVideo, IDC_SOURCE_FPS, NULL, FALSE))
			{
				MessageBox(NULL, L"Target FPS not satisfied", L"error", NULL);
				return 0;
			}
			wchar_t dirBuffer[MAX_PATH] = { 0 };
			if (GetDlgItemText(hPageVideo, IDC_EXPORT_DIR, dirBuffer, MAX_PATH) == 0)
			{
				MessageBox(NULL, L"Target directory not satisfied", L"error", NULL);
				return 0;
			}
			SendMessage(hPagePicture, WM_USER_STOP_MONITOR, 0, 0);
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEVIDEOPROGRESS), hDlg, DlgProc_VideoProgress);
			return TRUE;
		}
		}
		return TRUE;
	}
	case WM_USER_VIDEO_READY:
	{
		SetDlgItemText(hDlg, IDC_SOURCE, (LPWSTR)szVideoPath);
		int sourceFps = GetDlgItemInt(hDlg, IDC_SOURCE_FPS, NULL, FALSE);
		int targetFps = (int)(sourceFps / 2);
		SetDlgItemInt(hDlg, IDC_EXPORT_FPS, targetFps, FALSE);
		return TRUE;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hDlg);
		return TRUE;
	}
	}
	return FALSE;
}