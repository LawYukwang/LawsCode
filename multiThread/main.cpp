#include <windows.h> // Include all the windows headers.
#include <windowsx.h> // Include useful macros.
#include <vector>
#include <stack>
#include <thread>
#include <mutex>
#include <cmath>
#include "resource.h"
#define WINDOW_CLASS_NAME L"WINCLASS1"
//ADD ability to change color type

std::vector<HBITMAP> bitmapsToDraw; //Holds bitmap handles
std::vector<std::thread> threads; //Holds threads
std::stack<std::wstring> fileNamesBit; //Holds filesnames for bitmaps
std::stack<std::wstring> fileNamesSounds; //Holds filenames for sounds
unsigned int _iNumThreads; 
int _iStretchColor = HALFTONE;

std::mutex g_Mutex;
void LoadBitmaps();
void LoadSounds();
void Play();
void Load();

LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	// This is the main message handler of the system.
	PAINTSTRUCT ps; // Used in WM_PAINT.
	HDC hdc; // Handle to a device context.
	// What is the message?
	switch (_msg)
	{
	case WM_CREATE:
	{
		_iNumThreads = std::thread::hardware_concurrency();				
		// Do initialization stuff here.
		// Return Success.
		return (0);
	}
		break;
	case WM_PAINT:
	{
		// Simply validate the window.
		hdc = BeginPaint(_hwnd, &ps);
		if (!bitmapsToDraw.empty())
		{
			//Starting coordinates
			int x = 0;
			int y = 0;
			
			//Used to nicely arrange the images
			int _iNum = int(ceil(sqrt(bitmapsToDraw.size())));
			int _iWidth = (1000 / _iNum);
			int _iHeight = _iWidth;

			//Drawing the images
			for (unsigned int i = 0; i < bitmapsToDraw.size(); i++)
			{
				BITMAP bitmap;
				GetObject(bitmapsToDraw[i], sizeof(BITMAP), &bitmap);
				HDC hdcMem = CreateCompatibleDC(hdc);
				HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, bitmapsToDraw[i]));
				SetStretchBltMode(hdc, _iStretchColor);
				StretchBlt(hdc, x, y, _iWidth, _iHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
				SelectObject(hdcMem, oldBitmap);
				DeleteDC(hdcMem);
				x += _iWidth;
				//Adding new row
				if (x >= 1000)
				{
					y += _iWidth;
					x = 0;
				}
			}
		}	
		EndPaint(_hwnd, &ps);
		// Return Success.
		return (0);
	}
		break;
	case WM_COMMAND:
	{
		switch (LOWORD(_wparam))
		{
		case ID_FILE_LOADIMAGES:
		{
			LoadBitmaps();
			InvalidateRect(_hwnd, NULL, TRUE);
			break;
		}
		case ID_FILE_LOADSOUNDS:
		{
			LoadSounds();
			break;
		}
		case ID_COLORTYPE_HALFTONE:
		{
			_iStretchColor = HALFTONE;
			InvalidateRect(_hwnd, NULL, TRUE);
			break;
		}
		case ID_COLORTYPE_BLACKONWHITE:
		{
			_iStretchColor = BLACKONWHITE;
			InvalidateRect(_hwnd, NULL, TRUE);
			break;
		}
		case ID_COLORTYPE_COLORONCOLOR:
		{
			_iStretchColor = COLORONCOLOR;
			InvalidateRect(_hwnd, NULL, TRUE);
			break;
		}
		case ID_COLORTYPE_WHITEONBLACK:
		{
			_iStretchColor = STRETCH_ORSCANS;
			InvalidateRect(_hwnd, NULL, TRUE);
			break;
		}
		case ID_EXIT:
		{
			//Deleting bitmaps upon exiting
			for (unsigned int i = 0; i < bitmapsToDraw.size(); i++)
			{
				DeleteObject(bitmapsToDraw[i]);
			}
			PostQuitMessage(0);
			// Return success.
			return (0);
			break;
		}

		default:
			break;
		}
		return(0);
	
	}
	
		
	case WM_DESTROY:
	{	//Delete all bitmaps here
		// Kill the application, this sends a WM_QUIT message.
		for (unsigned int i = 0; i < bitmapsToDraw.size(); i++)
		{
			DeleteObject(bitmapsToDraw[i]);
		}
		
		PostQuitMessage(0);
		// Return success.
		return (0);
	}
		break;
	default:break;
	} // End switch.
	// Process any messages that we did not take care of...
	return (DefWindowProc(_hwnd, _msg, _wparam, _lparam));
}
		

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _lpCmdLine, int _nCmdShow)
{
	WNDCLASSEX winclass; // This will hold the class we create.
	HWND hwnd; // Generic window handle.
	MSG msg; // Generic message.
	// First fill in the window class structure.
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = _hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	winclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);


	// register the window class
	if (!RegisterClassEx(&winclass))
	{
		return (0);
	}
	hwnd = CreateWindowEx(NULL, // Extended style.
		WINDOW_CLASS_NAME, // Class.
		L"Parallel Loader - Joshua Tanner", // Title.
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, // Initial x,y.
		1000, 1000, // Initial width, height.
		NULL, // Handle to parent.
		NULL, // Handle to menu.
		_hInstance, // Instance of this application.
		NULL); // Extra creation parameters.
	// Check the window was created successfully.
	if (!hwnd)
	{
		return (0);
	}
		
	// Enter main event loop.
	while ((GetMessage(&msg, NULL, 0, 0))> 0)
	{
		// Translate any accelerator keys...
		TranslateMessage(&msg);
		// Send the message to the window proc.
		DispatchMessage(&msg);
	}
	// Return to Windows like this...
	return (static_cast<int>(msg.wParam));
}


void LoadBitmaps()
{
	//Getting the address(es) of bitmaps
	OPENFILENAME ofn;
	char szFile[10000];//Large allocation for selection of many files
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = LPWSTR(szFile);
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"Bitmap file (*.bmp)\0*.bmp\0\0";//Filter to only bitmap files
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	GetOpenFileName(&ofn);
	
	//Newly loaded bitmaps will replace existing ones
	for (unsigned int i = 0; i < bitmapsToDraw.size(); i++)
	{
		DeleteObject(bitmapsToDraw[i]);
	}
	bitmapsToDraw.clear();


	//To break up the files into each individual name
	wchar_t* _wsfileStr = ofn.lpstrFile;
	std::wstring directory = _wsfileStr;
	_wsfileStr += (directory.length() + 1);
	//Used if one file is selected
	if (!*_wsfileStr)
	{
		fileNamesBit.push(ofn.lpstrFile);
		threads.push_back(std::thread(Load));
	}
	else
	{
		//Continues to go through until all bitmaps have been loaded
		while (*_wsfileStr)
		{			
			std::wstring filename = _wsfileStr;
			_wsfileStr += (filename.length() + 1);			
			fileNamesBit.push(filename);
		}
		//Only necessary number of threads created
		for (unsigned int i = 0; i < _iNumThreads; i++)
		{
			threads.push_back(std::thread(Load));
		}
	}

	//Joining threads
	for (unsigned int i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}
	threads.clear();
}

void Load()
{
	while (!fileNamesBit.empty())
	{
		//Mutex lock to prevent race conditions
		g_Mutex.lock();
		std::wstring filename = fileNamesBit.top();
		fileNamesBit.pop();
		g_Mutex.unlock();
		HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, filename.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		bitmapsToDraw.push_back(hBitmap);//Adding to bitmap vector
	}	
}

void LoadSounds()
{
	//Getting addresses of sound(s) to play
	OPENFILENAME ofn;
	char szFile[1028];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = LPWSTR(szFile);
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"WAV file (*.wav)\0*.wav\0\0";//Restricted to WAV files only
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	GetOpenFileName(&ofn);	
	
	//Breaking up into individual addresses
	wchar_t* _wsfileStr = ofn.lpstrFile;
	std::wstring directory = _wsfileStr;

	_wsfileStr += (directory.length() + 1);	

	//Used for an individual sound
	if (!*_wsfileStr)
	{		
		fileNamesSounds.push(directory);
		threads.push_back(std::thread(Play));
	}
	else
	{
		while (*_wsfileStr)
		{
			std::wstring filename = _wsfileStr;
			_wsfileStr += (filename.length() + 1);
			fileNamesSounds.push(filename);			
		}
		//Only creating necessary number of threads
		for (unsigned int i = 0; i < _iNumThreads; i++)
		{
			threads.push_back(std::thread(Play));
		}

	}
	for (unsigned int i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}

	threads.clear();
}

void Play()
{	
	//Will loop through 
	while (!fileNamesSounds.empty())
	{
		//Mutex lock to prevent race conditions
		g_Mutex.lock();
		std::wstring filename = fileNamesSounds.top();
		fileNamesSounds.pop();
		g_Mutex.unlock();

		//Sending command to open the file
		std::wstring fileOpenCommand = L"open " + filename + L" type WAV";
		LPCWSTR Open = fileOpenCommand.c_str();
		int _iOpen = mciSendString(Open, NULL, 0, 0);

		//Sending command to play the file
		std::wstring filePlayCommand = L"play " + filename;
		LPCWSTR Play = filePlayCommand.c_str();
		int _iPlay = mciSendString(Play, NULL, 0, 0);
	}
		
}