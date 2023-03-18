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
//#include <Afx.h> // cwhd 위해
#include <WinSock2.h>

#include <fcntl.h>
#include <fstream>
#include <WS2tcpip.h>


// file download
#include <wininet.h>

#include"resource.h" //대화상자 리소스 


#define num_output_file 2

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "wldap32.lib")

#pragma comment(lib, "Wininet.lib")
using namespace std;
string SERVER_URL;
class file_donwloader {
public:
	file_donwloader();
	~file_donwloader();


public:
	bool download(
		__in std::wstring url,
		__in std::wstring save_path,
		__in std::wstring save_file_name
	);

private:
	void initialize();

private:

	HINTERNET _internet_handle;
};
file_donwloader::file_donwloader() {
	_internet_handle = nullptr;

	initialize();
}

file_donwloader::~file_donwloader() {

	/*
	생성한 Handle은 반드시 소멸해주어야하기때문에 잊지않도록 소멸자에 등록합니다.
	*/
	if (nullptr != _internet_handle) {
		InternetCloseHandle(_internet_handle);
		_internet_handle = nullptr;
	}
}
void file_donwloader::initialize() {

	/*
	InternetOpen API를 이용하여 HINTERNET Handle을 초기화 합니다.
	Agent값은 아무런 값이나 입력해도 되며, HTTP 프로토콜에서는 사용자 Agent로 사용되기도 합니다.
	*/

	_internet_handle = InternetOpen(
		L"file_donwload",
		INTERNET_OPEN_TYPE_DIRECT,
		nullptr,
		nullptr,
		0
	);
	if (nullptr == _internet_handle) {
		cout << "InternetOpen failed.";
	}
}
bool file_donwloader::download(
	__in std::wstring url,
	__in std::wstring save_path,
	__in std::wstring save_file_name
) {

	bool result = false;

	do {

		/*
		넘어온 parameter는 verify check를 해주어야겠죠? 필수 입력값이기때문에
		셋중에 하나라도 미입력시 정상적으로 파일을 다운로드하지 못하거나 저장하지 못할 수 있습니다.
		*/

		if (nullptr == _internet_handle) {
			cout << "_internet_handle nullptr.";
			break;
		}

		if (url.empty()) {
			cout<< "empty url.";
			break;
		}

		if (save_path.empty()) {
			cout<< "empty save_path.";
			break;
		}

		if (save_file_name.empty()) {
			cout<< "save_file_name save_path.";
			break;
		}


		/*
		InternetOpenUrl API를 이용하여 실제 접속이 시작됩니다.
		캐시가 아닌 원본파일을 항상 다운로드받기위해 Flag는 INTERNET_FLAG_RELOAD를 주었습니다.
		다른 Flag를 설정하고싶은경우 MSDN을 참조하시면 됩니다.
		*/

		const HINTERNET open_url_handle = InternetOpenUrl(
			_internet_handle,
			url.c_str(),
			nullptr,
			0,
			INTERNET_FLAG_RELOAD,
			0
		);

		if (nullptr == open_url_handle) {
			cout<< "_open_url_handle nullptr.";
			break;
		}


		/*
		해당 URL에 404, 500 등 http error가 넘어오더라도 정상적인 URL정보이기때문에 InternetOpenUrl의 HINTERNET Handle이 넘어오게 됩니다.
		그렇기때문에 NULL 또는 INVALID_HANDLE 정보로는 에러 여부를 확인할 수 없으므로
		HttpQueryInfo API를 이용하여 HTTP_QUERY_STATUS_CODE를 확인합니다.
		HTTP_STATUS_OK가 넘어왔다면 접속이 성공한것이니 이제 파일로 저장하기만 하면 됩니다.
		*/

		DWORD status_code = 0;
		DWORD status_code_size = sizeof(status_code);
		if (FALSE == HttpQueryInfo(
			open_url_handle,
			HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
			&status_code,
			&status_code_size,
			nullptr
		)) {
			cout<< "HttpQueryInfo failed (%d)", ::GetLastError();
			break;
		}

		if (sizeof(status_code) != status_code_size) {
			cout<< "status_code != status_code_size failed.";
			break;
		}

		if (HTTP_STATUS_OK != status_code) {
			cout<< "error status_code (%d).", status_code;
			break;
		}


		//
		// make save file name.
		//

		std::wstring file_name;
		file_name.assign(save_path);
		file_name.append(L"\\");
		file_name.append(save_file_name);

		const int max_buffer_size = 2014;
		char buffer[max_buffer_size] = {};

		FILE *save_file;
		save_file=_wfopen(
			file_name.c_str(),
			L"wb");
		if (1
		) {

			//
			// set result.
			//

			result = true;

			DWORD number_of_bytes_read = 0;
			do {

				/*
				여기가 가장 중요한 부분인데요 네트워크 통신이라는게 패킷단위로 왔다갔다 하다보니 한번의 통신으로 파일을 받을 수 없습니다.
				용량이 아주 작은 통신 패킷이라면 한번에 받기도 하겠지만 지정된 버퍼보다 큰 사이즈 또는 네트워크 상태에 따라 정해지지 않은 크기를 주고받기때문에
				InternetQueryDataAvailable API를 통해 현재 얼마만큼의 데이터를 읽을 수 있는지 확인하도록 합니다.
				*/

				DWORD read_available = 0;
				if (FALSE == InternetQueryDataAvailable(
					open_url_handle,
					&read_available,
					0,
					0
				)) {
					result = false;
					cout<< "InternetQueryDataAvailable failed.";
					break;
				}

				// resize buffer.
				if (read_available > max_buffer_size) {
					read_available = max_buffer_size;
				}


				//
				// internet read file.
				//

				if (FALSE == InternetReadFile(
					open_url_handle,
					buffer,
					read_available,
					&number_of_bytes_read
				)) {
					result = false;
					cout<< "InternetReadFile failed.";
					break;
				}

				if (0 < number_of_bytes_read) {

					//
					// write file.
					//
					fwrite(buffer, 1, number_of_bytes_read, save_file);
					memset(buffer, 0x00, number_of_bytes_read);
				}

			} while (0 != number_of_bytes_read);

			fclose(save_file);
		}

		InternetCloseHandle(open_url_handle);

	} while (false);

	return result;
}



// 유니코드 문자열을 UTF8 문자열로 변환시킨다.
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

// 주어진 문자열을 특정 문자열을 기준으로 나누고 그것을 vector에 담는다. 
// - Parameter
//		wstring s : 주어진 문자열
//		wstring divid : 특정 문자열
// - Returns
//		vector : 나눠진 문자열의 배열
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

BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
string resultUrl, inputUrl;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nCmdShow)
{

	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));

	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	cout << "Please Wait...\n변환에 시간이 소요됩니다. 잠시 기다려주세요.";

	g_hinst = hInstance;
	// url scheme variable 추출
	wstring total_variable = A2BSTR(szCmdLine);
	size_t colon = total_variable.rfind(':');
	wstring extension = total_variable.substr(colon + 1);
	size_t quote = extension.rfind('"');
	system_input_variable = extension.substr(0, quote);

#if NDEBUG
	if (total_variable.empty()) {
		MessageBox(NULL, (LPCWSTR)L"No item number\nretry.", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);
		return 0;
	}
	system_input_socket_number = split_into_vector_with_string(system_input_variable, L"/")[1];
	system_input_variable = split_into_vector_with_string(system_input_variable, L"/")[0];



#else
	system_input_variable = L"87";
	system_input_socket_number = L"0";
#endif // NDEBUG


	CreateDirectory(L"C:\\Temp", NULL);

	 DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
	if (fail_flag)
		MessageBox(NULL, (LPCWSTR)L"DWG Upload 전송 실패.\n Retry 다시 시도하세요.", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);

	return 0;
}

static HWND hChk1, hChk2;
OPENFILENAME OFNcp, OFNcd;
TCHAR filePathNamecp[5000] = L"";
TCHAR lpstrFilecp[5000] = L"";
static TCHAR filtercp[] = L"dwg file(*.dwg)\0*.dwg\0";
wstring cpFilename, cpFilepath;
BOOL pickcp = 0;

string readBuffer;

using std::cout; using std::cerr;
using std::endl; using std::string;
using std::ifstream; using std::ostringstream;

string readFileIntoString(const wstring& path) {
	ifstream input_file(path);
	if (!input_file.is_open()) {
		wcerr << "Could not open the file - '"
			<< path << "'" << endl;
		exit(EXIT_FAILURE);
	}
	return string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}
#include <curl/curl.h>
#include<chrono>
#include<thread>
#include <json/json.h>
#include <json/reader.h>
wstring resultUrl_w, inputUrl_w;
wstring result_path, input_path;

file_donwloader downloader_result, downloader_input;
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

BOOL CALLBACK DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		PlaceInCenterOfScreen(hDlg, WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
		SetWindowText(hDlg, _T("DWG Uploader"));
		hChk1 = CreateWindow(TEXT("button"), TEXT("DWG READY"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 120, 350, 15, hDlg, (HMENU)CHECK_BOX_CP, (HINSTANCE)hDlg, NULL);

		break;

	case WM_COMMAND:

		switch (wParam)
		{
		case IDOK:


			memset(&OFNcp, 0, sizeof(OFNcp));
			OFNcp.lStructSize = sizeof(OFNcp);
			OFNcp.hwndOwner = hDlg;
			OFNcp.lpstrFilter = filtercp;
			OFNcp.lpstrFile = lpstrFilecp;
			OFNcp.nMaxFile = sizeof(lpstrFilecp);
			OFNcp.lpstrInitialDir = L".";

			if (GetOpenFileName(&OFNcp) != 0) {
				wsprintf(filePathNamecp, L"%s 파일명을 확인하시고 잘못 선택한 경우 다시 고르시오.", OFNcp.lpstrFile);
				pickcp = MessageBox(hDlg, filePathNamecp, L"DWG 선택", MB_OK);

				// c++ 전체 파일 path에서 file명만 빼오기
				std::filesystem::path myFile = OFNcp.lpstrFile;
				std::filesystem::path fullname = myFile.filename();

				std::wstring path_string{ fullname.wstring() };

				cpFilepath = OFNcp.lpstrFile;
				cpFilename = path_string;

				int a = 0;
			}
			if (pickcp) {
				SendMessage(hChk1, BM_SETCHECK, BST_CHECKED, 0);
				USES_CONVERSION;
				SendMessage(hChk1, WM_SETTEXT, 0, (LPARAM)(cpFilename.c_str()));

			}
			return TRUE;

		case 1011:
			if (SendMessage(hChk1, BM_GETCHECK, 0, 0) != BST_CHECKED) {
				MessageBox(NULL, (LPCWSTR)L"NO DWG", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);
				return TRUE;
			}
			wcout << cpFilepath;
			
			CURL *curl;
			CURLcode res;


			curl = curl_easy_init();
			// not for Release
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			
			struct stat file_info;
			double speed_upload, total_time;
			FILE *fd;

			fd = _wfopen(cpFilepath.c_str(), L"rb");
			if (!fd)
				return 1; /* can't continue */

					  /* to get the file size */
			if (fstat(_fileno(fd), &file_info) != 0)
				return 1; /* can't continue */
			
			if (curl) {
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
				curl_easy_setopt(curl, CURLOPT_URL, "https://vector.express/api/v2/public/convert/dwg/cad2pdf/pdf?cad2pdf-auto-fit=true&cad2pdf-auto-orientation=true");
				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
				struct curl_slist *headers = NULL;
				headers = curl_slist_append(headers, "Content-Type: image/vnd.dwg");
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); 
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

				/* set where to read from (on Windows you need to use READFUNCTION too) */
				curl_easy_setopt(curl, CURLOPT_READFUNCTION, &fread);
				curl_easy_setopt(curl, CURLOPT_READDATA, fd);

				/* and give the size of the upload (optional) */
				curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
					(curl_off_t)file_info.st_size);

				/* enable verbose for easier tracing */
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

				// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s);
				//curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);
				

				res = curl_easy_perform(curl);
				if (res != CURLE_OK)
				{
					fail_flag = true;
					fprintf(stderr, "curl_easy_perform() failed: %s\n",
						curl_easy_strerror(res));
					wcout << "curl_easy_perform() failed: \n" << curl_easy_strerror(res);
				}
				else {
					/* now extract transfer info */
					curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
					curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

					fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",
						speed_upload, total_time);

				}
				std::size_t response_code = 0;

				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				std::cout << "\n\nhttp response code : " << response_code << std::endl;
				if (response_code == 200 || response_code == 201) {

					Json::Value root; 
					Json::StreamWriterBuilder CharReader;

					JSONCPP_STRING err;
					Json::CharReaderBuilder builder;
					const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
					
					if (!reader->parse(readBuffer.c_str(), readBuffer.c_str() + readBuffer.length(), &root, &err))
					{
						std::cout << "Failed to parse" << '\n';
						MessageBox(NULL, (LPCWSTR)L"변환 실패.\n Retry 다시 시도하세요.", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);
						EndDialog(hDlg, TRUE);
					}
					else
					{
						resultUrl = root["resultUrl"].asString();
						inputUrl = root["inputUrl"].asString();
					}

				}
				else {
					fail_flag = true;
					std::cout << "\n\nFail...Please retry." << std::endl;
				}

			}
			curl_easy_cleanup(curl);
			fclose(fd);

			//file 저장
			
			resultUrl_w.assign(resultUrl.begin(), resultUrl.end());
			inputUrl_w.assign(inputUrl.begin(), inputUrl.end());

			if (false == downloader_result.download(
				resultUrl_w,
				L"C:\\Temp",
				cpFilename + L".pdf"
			)) {
				MessageBox(NULL, (LPCWSTR)L"변환 실패.\n Retry 다시 시도하세요.", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);
				EndDialog(hDlg, TRUE);
				break;

			}
			if (false == downloader_input.download(
				inputUrl_w,
				L"C:\\Temp",
				cpFilename
			)) {
				MessageBox(NULL, (LPCWSTR)L"변환 실패.\n Retry 다시 시도하세요.", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONERROR);
				EndDialog(hDlg, TRUE);
				break;

			}

			input_path = L"C:\\Temp\\" + cpFilename;
			result_path = L"C:\\Temp\\" + cpFilename + L".pdf";


			// 전송 시작 
			
			cout << "\n\n uploading files...\n\n";
			CURL *curl_cadian;
			CURLcode res_cadian;
			curl_cadian = curl_easy_init();
			SERVER_URL = "https://eci-test.kro.kr:446/cadian";
			if (curl_cadian) {
				curl_easy_setopt(curl_cadian, CURLOPT_CUSTOMREQUEST, "POST");
				curl_easy_setopt(curl_cadian, CURLOPT_URL, SERVER_URL);
				curl_easy_setopt(curl_cadian, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl_cadian, CURLOPT_DEFAULT_PROTOCOL, "https");
				curl_easy_setopt(curl_cadian, CURLOPT_SSL_VERIFYPEER, false);
				struct curl_slist *headers = NULL;
				curl_easy_setopt(curl_cadian, CURLOPT_HTTPHEADER, headers);
				curl_mime *mime_cadian;
				curl_mimepart *part;
				mime_cadian = curl_mime_init(curl_cadian);
				int nUTFLen;

				part = curl_mime_addpart(mime_cadian);
				curl_mime_name(part, "itemId");
				curl_mime_data(part, unicodeToUtf8(system_input_variable.c_str(), nUTFLen), CURL_ZERO_TERMINATED);
				cout << "\n uploading with item " << unicodeToUtf8(system_input_variable.c_str(), nUTFLen) << "\n";
				cout << "SERVER_URL"<< SERVER_URL <<"\n";

				part = curl_mime_addpart(mime_cadian);
				curl_mime_name(part, "roomId");
				curl_mime_data(part, unicodeToUtf8(system_input_socket_number.c_str(), nUTFLen), CURL_ZERO_TERMINATED);
				cout << "\n uploading with socket " << unicodeToUtf8(system_input_socket_number.c_str(), nUTFLen) << "\n";

				part = curl_mime_addpart(mime_cadian);
				curl_mime_name(part, "pdf");
				curl_mime_filedata(part, unicodeToUtf8(input_path.c_str(), nUTFLen));


				part = curl_mime_addpart(mime_cadian);
				curl_mime_name(part, "dwg");
				curl_mime_filedata(part, unicodeToUtf8(result_path.c_str(), nUTFLen));

				curl_easy_setopt(curl_cadian, CURLOPT_MIMEPOST, mime_cadian);

				res_cadian = curl_easy_perform(curl_cadian);
				if (res_cadian != CURLE_OK)
				{
					fail_flag = true;
					fprintf(stderr, "curl_easy_perform() failed: %s\n",
						curl_easy_strerror(res));
					wcout << "for korea" << "curl_easy_perform() failed: %s\n" << curl_easy_strerror(res);
				}
				curl_mime_free(mime_cadian);

				std::size_t response_code = 0;
				curl_easy_getinfo(curl_cadian, CURLINFO_RESPONSE_CODE, &response_code);
				std::cout << "\n\nhttp response code : " << response_code << std::endl;
				if (response_code == 200 || response_code == 201) {
					fail_flag = false;
					MessageBox(NULL, (LPCWSTR)L"DWG Upload 전송 완료", (LPCWSTR)L"DWG Uploader", MB_OK | MB_ICONINFORMATION);
				}
				else {
					fail_flag = true;
					std::cout << "\n\nFail...Please retry." << std::endl;
				}
			}
			curl_easy_cleanup(curl_cadian);

			//삭제


			EndDialog(hDlg, TRUE);

			if (_waccess(input_path.c_str(), 0) != -1) {
				_wremove(input_path.c_str());
			}
			if (_waccess(result_path.c_str(), 0) != -1) {
				_wremove(result_path.c_str());
			}


			return TRUE;
		case IDCANCEL:

			EndDialog(hDlg, TRUE);
			return 0;
		}
		return 0;
	}

	return FALSE;
}