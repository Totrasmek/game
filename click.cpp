#include <iostream>

#define NOMINXMAX
#include <Windows.h>
#include <tlhelp32.h>

#include <stdint.h>
#include <vector>
#include <bitset>
#include <string>

#include <fstream>
#include <string>
#include <unordered_map>

#include "CkMailMan.h"
#include "CkEmail.h"

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

std::wstring launcher_path = L""; // = L"D:\\games\\launcher\\launcher.exe"
std::wstring game_name = L"RotMG Exalt.exe";
std::string screenshot_base_path = "screenshots\\";
std::string screenshot_path = "";
std::string gmail_password = "";
std::string gmail_sender = "";
std::string gmail_receiver = "";

//========================================================================
//							EMAILS
//========================================================================

void send_email(const char* subject, const char* body)
{
    CkMailMan mailman;

    // Gmail SMTP server settings
    mailman.put_SmtpHost("smtp.gmail.com");
    mailman.put_SmtpUsername(gmail_sender.c_str());
    mailman.put_SmtpPassword(gmail_password.c_str()); // Use App Password here
    mailman.put_StartTLS(true); // Use STARTTLS
	mailman.put_SmtpPort(587);

    CkEmail email;

    email.put_Subject(subject);
    email.put_Body(body);
    email.put_From(gmail_sender.c_str());
    email.AddTo("",gmail_receiver.c_str());

    // Send the email
    bool success = mailman.SendEmail(email);
    if (success != true) {
        std::cout << "Error sending email: " << mailman.lastErrorText() << std::endl;
    }
}
//========================================================================
//							CONFIG FILE PARSER
//========================================================================

std::string trimRight(const std::string& str) {
    size_t end = str.find_last_not_of(" \t");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string trimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::pair<std::unordered_map<std::string, std::string>, std::unordered_map<std::string, std::string>> read_config(const std::string& filename)
{
    std::unordered_map<std::string, std::string> config;
    std::unordered_map<std::string, std::string> credentials;
    std::ifstream file(filename);
    std::string line;
    bool readingCredentials = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments

        if (line == "[Credentials]") {
            readingCredentials = true;
            continue;
        }

        auto delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) continue;

        auto key = trimRight(line.substr(0, delimiterPos));
        auto value = trimLeft(line.substr(delimiterPos + 1));

        if (readingCredentials) {
            credentials[key] = value;
        } else {
            config[key] = value;
        }
    }

    return {config, credentials};
}

//========================================================================
//						OPENCV FOR IMAGE MATCHING
//========================================================================

// Function to capture a screenshot of the window
cv::Mat capture_screenshot(HWND hwnd) 
{
	//DEBUG("Capturing screenshot");
	
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

int load_img(std::string goal_string,cv::Mat& image)
{	
	goal_string = screenshot_path + goal_string + std::string(".png");
	
	image = cv::imread(goal_string);
	
	if(check_mat(image)) return -1;
	return 0;
}

int needle_point(HWND hwnd,std::string goal_string,POINT& centre, double confidence = 0.95, const cv::Mat& mask = cv::Mat())
{	
	cv::Mat haystack = capture_screenshot(hwnd);
	
	cv::Mat needle;
	if(load_img(goal_string,needle)) return -1;
			
	cv::Mat result;
			
	if (!mask.empty()) cv::matchTemplate(haystack, needle, result, cv::TM_CCOEFF_NORMED, mask);
	else cv::matchTemplate(haystack, needle, result, cv::TM_CCOEFF_NORMED);
	
	// Find the location of the best match
	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
	centre.x = maxLoc.x + needle.cols/2;
	centre.y = maxLoc.y + needle.rows/2;
	
	double confidence_result = result.at<float>(maxLoc.y,maxLoc.x);
	//DEBUG("Confidence is: " << confidence_result);
	
	// Draw a rectangle around the detected region
	//cv::rectangle(haystack, maxLoc, cv::Point(maxLoc.x + needle.cols, maxLoc.y + needle.rows), cv::Scalar(0, 255, 0), 2);

	// Display the result
	//display_image(haystack);
	
	if(confidence_result >= confidence) return 0;
	return -1;
}

int find(HWND hwnd, std::string goal_string, POINT& centre, DWORD timeout, DWORD interval, double confidence = 0.95, const cv::Mat& mask = cv::Mat())
{	
	DWORD startTime = GetTickCount();
	while (GetTickCount() - startTime < timeout)
	{
		//DEBUG(GetTickCount());
		static DWORD lastTime = 0;
        if (GetTickCount() - lastTime >= interval) 
		{
            lastTime = GetTickCount();

			if(!needle_point(hwnd,goal_string,centre,confidence,mask)) return 0;
        }
    }
	return -1;
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

void send_key_press(BYTE key,int delay=50) 
{
    // Simulate Enter key press
    keybd_event(key, 0, 0, 0);
    
    // Simulate a short delay (optional)
    Sleep(delay);
    
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
int click(HWND hwnd, std::string goal_string, POINT& centre, DWORD timeout, DWORD interval,double confidence = 0.95)
{	
	DEBUG("clicking on " << goal_string);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	int x_offset = rect.left;
	int y_offset = rect.top;
	
	if(find(hwnd,goal_string,centre,timeout,interval,confidence)) return -1;
	SetCursorPos(x_offset+centre.x,y_offset+centre.y);
	left_mouse_click();
	
	return 0;
}

int login(HWND hwnd,std::string u, std::string p)
{
	DEBUG("Logging in");
	POINT centre;
	
	if(click(hwnd,"email",centre,10000,1000)) return -1;
	Sleep(1000);
	send_keystrokes(u);
	Sleep(1000);
	
	if(click(hwnd,"password",centre,5000,1000)) return -1;
	Sleep(1000);
	send_keystrokes(p);
	Sleep(1000);
	send_key_press(VK_RETURN);
	
	if(click(hwnd,"update",centre,5000,1000))
	{
		Sleep(60000);
		if(click(hwnd,"play",centre,60000*5,5000)) return -1;
	}
	else if(click(hwnd,"play",centre,5000,1000)) return -1;
	
	DEBUG("Logging in finished");
	
	return 0;
}

int close_popups(HWND hwnd)
{
	DEBUG("Closing popups");
	POINT centre;
	
	if(!click(hwnd,"claim",centre,5000,1000)) 
	{
		if(click(hwnd,"closex",centre,10000,1000)) return -1;
		DEBUG("CLAIMCLAIM");
		return 1;
	}
	if(click(hwnd,"closex",centre,5000,1000)) return -1;
	click(hwnd,"closex2",centre,5000,1000);
		
	return 0;
}

int wait_for_load(HWND hwnd)
{
	DEBUG("Waiting for load");
	POINT centre;
	if(find(hwnd,"closex",centre,120000,3000)) return -1;
	return 0;
}

int enter(HWND hwnd,std::string goal_string, POINT centre)
{
	DEBUG("entering " << goal_string);
	
	POINT goal_loc;
	
	int threshold = 30;
	
	while(1)
	{
		if(!needle_point(hwnd,goal_string+"_entry",goal_loc))
		{
			send_key_press(VK_CAPITAL);
			break;
		}
		
		while(needle_point(hwnd,goal_string,goal_loc)) send_key_press(0x45);
		
		Sleep(1000);
		
		int d_x = goal_loc.x - centre.x;
		//DEBUG("\nd_x = goal_loc.x - centre.x\n" << d_x << "=" << goal_loc.x << "-" << centre.x);
		int d_y = goal_loc.y - centre.y;
		//DEBUG("\nd_y = goal_loc.y - centre.y\n" << d_y << "=" << goal_loc.y << "-" << centre.y);
		//DEBUG("\nd_x " << d_x << "\nd_y " << d_y);
		
		// might need a threshold
		
		if(d_x > threshold) keybd_event(0x44, 0, 0, 0);//send_key_press(0x44,1000); // D go right
		else if(d_x < -1*threshold) keybd_event(0x41, 0, 0, 0);//send_key_press(0x41,1000); // A go left
		
		if(d_y > threshold) keybd_event(0x53, 0, 0, 0);//send_key_press(0x53,1000); // S go up
		else if(d_y < -1*threshold) keybd_event(0x57, 0, 0, 0);//;send_key_press(0x57,1000); // W go down
		
		// detect if change in old d_x d_y is none after movement in which case do a move
		
		Sleep(125);
		
		keybd_event(0x44, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(0x41, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(0x57, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(0x53, 0, KEYEVENTF_KEYUP, 0);
		
		Sleep(125);
	}
	
	return 0;
}

int game(HWND hwnd, int popup_err)
{
	DEBUG("Navigating game");
	
	//send_key_press(0x58); // presses X 

	POINT stats_loc;
	
	if(needle_point(hwnd,"stats",stats_loc)) return -1;
		
	POINT centre;
	centre.x = stats_loc.x/2;
	
	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	int height = (rect.bottom-rect.top);
	height = height >=0 ? height : -1*height; 
	centre.y = height/2;
	
	if(!popup_err)
	{
		if(enter(hwnd,"vault",centre)) return -1;
		Sleep(5000);
		if(enter(hwnd,"dailyquestroom",centre)) return -1;
		Sleep(1000);
	}
	
	if(enter(hwnd,"loginseer",centre)) return -1;
	Sleep(1000);
	int rewards = 0;
	while(!click(hwnd,"reward",centre,2000,1000,0.8)){++rewards;}
	
	return rewards;
}

int whole_process(std::string u, std::string p)
{
	DEBUG("Starting...");

	if(GetKeyState(VK_CAPITAL) & 0x0001) send_key_press(VK_CAPITAL);

	PROCESS_INFORMATION* pi = start_process(launcher_path);

	DWORD launcherProcessId = pi->dwProcessId;
	DEBUG("Process ID is: " << launcherProcessId);
	
	Sleep(10000);

	HWND hwnd = find_window_by_process_id(launcherProcessId);

	if (hwnd == nullptr)
	{
		DEBUG("Window handle get failed");
		end_process(pi);
		return -1;
	}
	
	bring_to_front(hwnd);
	
	screenshot_path = screenshot_base_path;
	
	if(login(hwnd,u,p))
	{
		DEBUG("Login failed");
		end_process(pi);
		return -1;
	}

	Sleep(10000);
	hwnd = find_window_by_process_name(game_name.c_str());
	
	RECT rect;
	GetWindowRect(hwnd, &rect);
	
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	
	DEBUG("Resolution " << height << "x" << width);
	screenshot_path += std::to_string(height) + "p\\";
	DEBUG("path is " << screenshot_path);
	
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
	
	int popup_err = close_popups(hwnd);
	if(popup_err == -1) 
	{
		DEBUG("Closing popups failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}
	
	Sleep(1000);
	
	int rewards = game(hwnd,popup_err);
	
	if(rewards == -1) 
	{
		DEBUG("Playing game failed");
		end_process(pi);
		PostMessage(hwnd,WM_CLOSE,0,0);
		return -1;
	}

	Sleep(5000);
	end_process(pi);
	PostMessage(hwnd,WM_CLOSE,0,0);
	
	return rewards;
}

//========================================================================
//								MAIN
//========================================================================

int main(void)
{	
	auto [config, credentials] = read_config("config.ini");

	std::string launcher_path_short = config["launcher_path"];
	launcher_path = std::wstring(launcher_path_short.begin(),launcher_path_short.end());
	
	std::string log = "Account statuses:";
	
    for(const auto& pair : credentials)
	{
        DEBUG(pair.first);
		int rewards = whole_process(pair.first,pair.second);
		if(rewards==-1) 
		{
			log += ("\n" + pair.first + ": failed");
		}
		else
		{
			log += ("\n" + pair.first + ": rewards " + std::to_string(rewards));
		}
    }

	send_email("logins",log.c_str());

    return 0;
	
	
}