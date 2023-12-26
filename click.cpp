#include <iostream>

#define NOMINXMAX
#include <Windows.h>
#include <tlhelp32.h>

#include <stdint.h>
#include <vector>
#include <bitset>
#include <string>

#include <opencv2/opencv.hpp>

// Define DEBUG_PRINT if you want to include debug print statements
#define DEBUG_PRINT

// Custom macro for debug print
#ifdef DEBUG_PRINT
#define DEBUG(message) std::cout << message << std::endl
#define DEBUGW(message) std::wcout << message << std::endl
#else
#define DEBUG(message)
#define DEBUGW(message)
#endif

#define BOTTOM_BYTE(x) ((x) & (0x00ff))
#define TOP_BYTE(x) ((x) >> (8))

std::wstring launcher_path = L"D:\\games\\launcher\\launcher.exe";
std::wstring game_name = L"RotMG Exalt.exe";
std::string email_string = "";
std::string password_string = "";
std::string screenshot_path_string = "screenshots\\";

//========================================================================
//						OPENCV FOR IMAGE MATCHING
//========================================================================

// Function to capture a screenshot of the window
cv::Mat capture_screenshot(HWND hwnd) 
{
	DEBUG("Capturing screenshot");
	
	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	int x_offset = rect.left;
	int y_offset = rect.top;
	int width = rect.right - x_offset;
	int height = rect.bottom - y_offset;
	
	HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x_offset, y_offset, SRCCOPY);
    
    // Convert the captured image to an OpenCV Mat
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // Negative height to ensure top-down orientation
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    cv::Mat screenshot(height, width, CV_8UC3);
    GetDIBits(hMemoryDC, hBitmap, 0, height, screenshot.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Clean up resources
    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return screenshot;
}

// Displays an opencv2 matrix as an image
void display_image(const cv::Mat& image, const std::string& windowName = "Image") 
{
    // Check if the image is empty
    if (image.empty()) 
	{
        std::cerr << "Error: Empty image." << std::endl;
        return;
    }

    // Display the image
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);  // Create a resizable window
    cv::imshow(windowName, image);

    // Wait for a key event and close the window when a key is pressed
    cv::waitKey(0);

    // Destroy the window
    cv::destroyWindow(windowName);
}

int check_mat(const cv::Mat& image)
{
	if (image.empty()) 
	{
        std::cerr << "Error: Could not load the image." << std::endl;
        return -1;
    }
	return 0;
}

int find(HWND hwnd, std::string goal_string, POINT& centre, DWORD timeout, DWORD interval, double confidence = 0.95, const cv::Mat& mask = cv::Mat())
{
	cv::Mat result;
	
	goal_string = screenshot_path_string + goal_string + std::string(".png");
	
	cv::Mat needle = cv::imread(goal_string);
	
	if(check_mat(needle)) return -1;
	
	DWORD startTime = GetTickCount();
	DEBUG(timeout);
	DEBUG(startTime);
	while (GetTickCount() - startTime < timeout)
	{
		//DEBUG(GetTickCount());
		static DWORD lastTime = 0;
        if (GetTickCount() - lastTime >= interval) 
		{
            lastTime = GetTickCount();

			cv::Mat haystack = capture_screenshot(hwnd);
			
			if (!mask.empty()) cv::matchTemplate(haystack, needle, result, cv::TM_CCOEFF_NORMED, mask);
			else cv::matchTemplate(haystack, needle, result, cv::TM_CCOEFF_NORMED);
			
			// Find the location of the best match
			double minVal, maxVal;
			cv::Point minLoc, maxLoc;
			cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
			centre.x = maxLoc.x + needle.cols/2;
			centre.y = maxLoc.y + needle.rows/2;
			
			double confidence_result = result.at<float>(maxLoc.y,maxLoc.x);
			DEBUG("Confidence is: " << confidence_result);
			
			// Draw a rectangle around the detected region
			//cv::rectangle(haystack, maxLoc, cv::Point(maxLoc.x + needle.cols, maxLoc.y + needle.rows), cv::Scalar(0, 255, 0), 2);

			// Display the result
			//display_image(haystack);
			
			if(confidence_result >= confidence) 
			{DEBUG("0");return 0;}
        }
    }
	DEBUG(GetTickCount());
	DEBUG("-1");
	return -1;
}

int walk_towards(HWND hwnd,std::string goal_string)
{
	
	POINT goal_loc;
	POINT stats_loc;
	
	while(1)
	{
		if(find(hwnd,goal_string,goal_loc,5000,1000)) 
		{DEBUG("goal_loc wrong");return -1;}
		if(find(hwnd,"stats",stats_loc,5000,1000))
		{DEBUG("stats wrong");return -1;}
		
		POINT centre;
		centre.x = stats_loc.x/2;
		centre.y = stats_loc.y/2;
		
		int d_x = goal_loc.x - centre.x;
		int d_y = goal_loc.y - centre.y;
		DEBUG("d_x " << d_x << " d_y " << d_y);
	}
}

//========================================================================
//							PROGRAM FOREGROUND/BACKGROUND
//========================================================================

BOOL CALLBACK enum_windows_callback(HWND hwnd, LPARAM lParam)
{	
	auto params = reinterpret_cast<std::pair<DWORD, HWND>*>(lParam);
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    // Check if the window belongs to the desired process
    if (processId == params->first)
    {
        // This is the window we are looking for
        params->second = hwnd;

        // Stop enumerating
        return FALSE;
    }

    // Continue enumerating
    return TRUE;
}

HWND find_window_by_process_id(DWORD processId)
{
    DEBUG("Finding process by ID");
    std::pair<DWORD, HWND> params(processId, nullptr);
    EnumWindows(enum_windows_callback, reinterpret_cast<LPARAM>(&params));
    return params.second;
}

void bring_to_front(HWND hwnd)
{
	DEBUG("Bringing window to front");
	ShowWindow(hwnd, SW_RESTORE);

    SetForegroundWindow(hwnd);
}

//========================================================================
//								PROCESS ACTIONS
//========================================================================
PROCESS_INFORMATION* start_process(std::wstring path)
{
	DEBUG("Starting Process");
	
	STARTUPINFO si;
    PROCESS_INFORMATION* pi = new PROCESS_INFORMATION;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( pi, sizeof(*pi) );

	wchar_t* command_line = new wchar_t[path.size() + 1];
	std::size_t length = path.copy(command_line, path.size() + 1, 0);
	command_line[length] = '\0';

    // Start the child process. 
    if( !CreateProcess( 
		NULL,   // No module name (use command line)
        command_line,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
	
	delete[] command_line;
	
    return pi;
}

void end_process(PROCESS_INFORMATION* pi)
{
	DEBUG("Killing process");
	
	TerminateProcess(pi->hProcess, 0);
	
	CloseHandle(pi->hProcess);
	CloseHandle(pi->hThread);
	
	delete pi;
}

HWND find_window_by_process_name(const wchar_t* processName) 
{
    // Create a snapshot of the current processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    // Initialize the PROCESSENTRY32 structure
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Check if the snapshot was successfully created
    if (!Process32First(hSnapshot, &pe32))
	{
		// Close the snapshot handle and return false (process not found)
		CloseHandle(hSnapshot);
		DEBUG("Process is not running");
		return nullptr;
	}
		
	// Iterate through the processes in the snapshot
	do {
		// Compare the process name of the current process with the target process name
		if (_wcsicmp(pe32.szExeFile, processName) == 0) {
			// Close the snapshot handle and return window handle
			CloseHandle(hSnapshot);
			DEBUG("Process is running");
			HWND hwnd = find_window_by_process_id(pe32.th32ProcessID);
			if(hwnd == nullptr) DEBUGW("window handle is null " << hwnd << "from search for " << processName);
			return hwnd;
		}
	} while (Process32Next(hSnapshot, &pe32));
	
	CloseHandle(hSnapshot);
	DEBUG("Process is not running");
	return nullptr;
}

//========================================================================
//								CURSOR ACTIONS
//========================================================================
// USAGE:
//			Sleep(1000);
//			POINT p;
//			GetCursorPos(&p);
//			std::cout << "x: " << p.x << "y: " << p.y << std::endl;
//			Sleep(1000);
//			SetCursorPos(-1920/2,1080/2);

void left_mouse_click()
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Left button down
    SendInput(1, &input, sizeof(INPUT));

    // Simulate a short delay (optional)
    Sleep(50);

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP; // Left button up
    SendInput(1, &input, sizeof(INPUT));
}


//========================================================================
//								KEY STROKES
//========================================================================
// USAGE:
//			send_keystrokes(std::string("eE\r"))

void print_INPUT(INPUT input)
{
	DEBUG("Printing input struct");
	
	std::bitset<8> input_char = char(input.ki.wVk);
	
	DEBUG("input char:" << input_char);
	
	std::bitset<16> input_dwFlags = char(input.ki.dwFlags);
	
	DEBUG("input dw:" << input_dwFlags);
}

void send_key_press(BYTE key,int delay = 50) 
{
    // Simulate Enter key press
    keybd_event(key, 0, 0, 0);
    
    // Simulate a short delay (optional)
    Sleep(50);
    
    // Simulate Enter key release
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

uint8_t send_keystrokes(std::string input_chars)
{
	DEBUG("Sending keystroke");
	size_t num_chars = input_chars.size();
	std::vector<INPUT> inputs;
	int count_shifts = 0;
	
	// Parse characters
	for(size_t character_index = 0; character_index < num_chars; ++character_index)
	{
		SHORT key = VkKeyScanExA(input_chars[character_index],GetKeyboardLayout(0));
		std::bitset<16> all = key;
		std::bitset<8> top = char(TOP_BYTE(key));
		std::bitset<8> bot = char(BOTTOM_BYTE(key));
		/*std::cout << "16bit byte : " <<  all << std::endl;
		std::cout << "top byte : " <<  top << std::endl;
		std::cout << "bottom_byte : " << bot  << std::endl;*/
		
		INPUT char_press;
		char_press.type = INPUT_KEYBOARD;
		char_press.ki.dwFlags = 0;
		char_press.ki.wVk = BOTTOM_BYTE(key);
		inputs.push_back(char_press);
		
		char_press.ki.dwFlags = KEYEVENTF_KEYUP;
		inputs.push_back(char_press);
		
		if(TOP_BYTE(key))
		{
			//std::cout << "shift key is used" << std::endl; 
			INPUT shift_press;
			shift_press.type = INPUT_KEYBOARD;
			shift_press.ki.dwFlags = 0;
			shift_press.ki.wVk = VK_SHIFT;
			inputs.insert(std::next(inputs.end()-3),shift_press);
			
			shift_press.ki.dwFlags = KEYEVENTF_KEYUP;
			inputs.push_back(shift_press);
			
			++count_shifts;
		}
	}
	
	/*for(INPUT& input : inputs)
	{
		print_INPUT(input);
	}*/
	
	UINT expected_count = static_cast<UINT>(2 * (num_chars + count_shifts));

	UINT units_sent = SendInput(expected_count,inputs.data(),sizeof(INPUT));
	
	if (units_sent != expected_count)
	{
		std::cout << "SendInput failed: 0x" << std::hex << HRESULT_FROM_WIN32(GetLastError()) << std::endl;
	} 
	
	return units_sent;
}

//========================================================================
//						SCRIPTS (INTERACTING FUNCTIONS)
//========================================================================
int click(HWND hwnd, std::string goal_string, POINT& centre, DWORD timeout, DWORD interval)
{	
	DEBUG("clicking on " << goal_string);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	int x_offset = rect.left;
	int y_offset = rect.top;
	
	if(find(hwnd,goal_string,centre,timeout,interval)) return -1;
	SetCursorPos(x_offset+centre.x,y_offset+centre.y);
	left_mouse_click();
	
	return 0;
}

int login(HWND hwnd)
{
	DEBUG("Logging in");
	POINT centre;
	
	if(click(hwnd,"email",centre,10000,1000)) return -1;
	Sleep(1000);
	send_keystrokes(email_string);
	Sleep(1000);
	
	if(click(hwnd,"password",centre,5000,1000)) return -1;
	Sleep(1000);
	send_keystrokes(password_string);
	Sleep(1000);
	send_key_press(VK_RETURN);
	
	if(click(hwnd,"play",centre,5000,1000)) return -1;
	
	DEBUG("Logging in finished");
	
	return 0;
}

int close_popups(HWND hwnd)
{
	DEBUG("Closing popups");
	POINT centre;
	
	click(hwnd,"claim",centre,5000,1000);
	if(click(hwnd,"closex",centre,5000,1000)) return -1;
		
	return 0;
}

int game(HWND hwnd)
{
	DEBUG("Navigating game");
	
	//send_key_press(0x58); // presses X 
					      // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	
	walk_towards(hwnd,"vault");
	
	return 0;
}

int wait_for_load(HWND hwnd)
{
	DEBUG("Waiting for load");
	POINT centre;
	if(find(hwnd,"closex",centre,120000,3000)) return -1;
	return 0;
}

/*
CONSIDERATIONS:
	if find Load1,2,3 or 4 imagesize
		wait 5s
	else if find closex
		return
		
 instead of closign when tagret to walk towards is out of screen, walk around a bit first
 
 add timeouts to all checks instead of sleeping for arbitrary amounts of time
*/


//========================================================================
//								MAIN
//========================================================================

int main(void)
{
	DEBUG("Starting...");

	PROCESS_INFORMATION* pi = start_process(launcher_path);

	// Get the process ID of the launched process (game launcher)
	DWORD launcherProcessId = pi->dwProcessId;
	DEBUG("Process ID is: " << launcherProcessId);
	
	// Sleep for some time to allow the launcher to start
	Sleep(1000);

	// Attempt to find the window handle
	HWND hwnd = find_window_by_process_id(launcherProcessId);

	// Check if a valid window handle was found
	if (hwnd == nullptr)
	{
		DEBUG("Window handle get failed");
		end_process(pi);
		return -1;
	}
	
	// Bring window to front
	bring_to_front(hwnd);
	
	// Login and launch exalt
	if(login(hwnd)) 
	{
		DEBUG("Login failed");
		end_process(pi);
		return -1;
	}

	// Check game is running
	Sleep(10000);
	hwnd = find_window_by_process_name(game_name.c_str());
	
	if(wait_for_load(hwnd))
	{
		DEBUG("wait for load failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}
	
	if(!hwnd)
	{
		DEBUG("Game launch failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}
	
	if(close_popups(hwnd)) 
	{
		DEBUG("Closing popups failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}
	
	Sleep(1000);
	
	if(game(hwnd)) 
	{
		DEBUG("Playing game failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}

	Sleep(5000);
	end_process(pi);
	
	return 0;
}

// need an email ryan function