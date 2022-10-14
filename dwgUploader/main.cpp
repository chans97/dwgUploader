#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define _WINSOCKAPI_
#include <windows.h>
#include <iostream>
#include <atlbase.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <commdlg.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <atlbase.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <locale>


#include <io.h> // for file check
//#include <Afx.h> // cwhd ����
#include <WinSock2.h>

#include <fcntl.h>
#include <fstream>
#include <WS2tcpip.h>

#include"resource.h" //��ȭ���� ���ҽ� 


#define num_output_file 2

#pragma comment(lib,"ws2_32.lib")
using namespace std;


// �����ڵ� ���ڿ��� UTF8 ���ڿ��� ��ȯ��Ų��.
static char *unicodeToUtf8(const WCHAR *zWideFilename, int &nUTFLen) {
	int nByte;
	char *zFilename;

	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
	nUTFLen = nByte;
	zFilename = (char*)malloc(nByte);
	if (zFilename == 0) {
		return 0;
	}
	nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte,
		0, 0);
	if (nByte == 0) {
		free(zFilename);
		zFilename = 0;
	}
	return zFilename;
}

// �־��� ���ڿ��� Ư�� ���ڿ��� �������� ������ �װ��� vector�� ��´�. 
// - Parameter
//		wstring s : �־��� ���ڿ�
//		wstring divid : Ư�� ���ڿ�
// - Returns
//		vector : ������ ���ڿ��� �迭
vector<wstring> split_into_vector_with_string(wstring s, wstring divid) {
	vector<wstring> v;
	int start = 0;
	int d = s.find(divid);
	while (d != -1) {
		v.push_back(s.substr(start, d - start));
		start = d + 1;
		d = s.find(divid, start);
	}
	v.push_back(s.substr(start, d - start));

	return v;
}

wstring repeat_char(wstring str, wchar_t sign)
{
	wstring result;
	for (int i = 0; i < str.size(); i++)
	{
		result.push_back(str[i]);
		if (str[i] == sign) {
			result.push_back(sign);
		}
	}
	return result;
}



std::wstring remove_extension(const std::wstring& filename) {
	size_t lastdot = filename.find_last_of(L".");
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}


void PlaceInCenterOfScreen(HWND window, DWORD style, DWORD exStyle)
{
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT clientRect;
	GetClientRect(window, &clientRect);

	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	SetWindowPos(window, NULL,
		screenWidth / 2 - clientWidth / 2,
		screenHeight / 2 - clientHeight / 2,
		clientWidth, clientHeight, 0
	);
}

HINSTANCE g_hinst;
wstring system_input_variable;
wstring system_input_socket_number;
BOOL fail_flag = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nCmdShow)
{

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));

	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	cout << "Please Wait...\n��ȯ�� �ð��� �ҿ�˴ϴ�. ��� ��ٷ��ּ���.";

	g_hinst = hInstance;
	// url scheme variable ����
	wstring total_variable = A2BSTR(szCmdLine);
	size_t colon = total_variable.rfind(':');
	wstring extension = total_variable.substr(colon + 1);
	size_t quote = extension.rfind('"');
	system_input_variable = extension.substr(0, quote);

#if NDEBUG
	if (total_variable.empty()) {
		MessageBox(NULL, (LPCWSTR)L"No item number\nretry.", (LPCWSTR)L"ECI-PLM Uploader", MB_OK | MB_ICONERROR);
		return 0;
	}
	system_input_socket_number = split_into_vector_with_string(system_input_variable, L"/")[1];
	system_input_variable = split_into_vector_with_string(system_input_variable, L"/")[0];



#else
	system_input_variable = L"87";
	system_input_socket_number = L"0";
#endif // NDEBUG


	// DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
	if (fail_flag)
		MessageBox(NULL, (LPCWSTR)L"ECI-PLM Upload ���� ����.\n Retry �ٽ� �õ��ϼ���.", (LPCWSTR)L"ECI-PLM Uploader", MB_OK | MB_ICONERROR);

	return 0;
}
