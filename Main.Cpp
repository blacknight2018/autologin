#include <iostream>
#include <string>
#include <vector>
#include <WinSock2.h>
#include <streambuf>
#include <strstream>
#include <sstream>
#include <string>
#include <iostream>
#include <Windows.h>
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <time.h>
#include <sys/timeb.h>
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"ws2_32.lib")
using namespace std;
/* HTTP GET */
string sendRET( LPCWSTR url )
{
	string ret;

	DWORD dwSize = 0;

	DWORD dwDownloaded = 0;

	LPSTR pszOutBuffer = NULL;

	HINTERNET hSession	= NULL,
		  hConnect	= NULL,
		  hRequest	= NULL;
	BOOL bResults		= FALSE;
	hSession = WinHttpOpen( L"User Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if ( hSession )
	{
		hConnect = WinHttpConnect( hSession, L"172.16.30.33", INTERNET_DEFAULT_HTTP_PORT, 0 );
	}
	if ( hConnect )
	{
		hRequest = WinHttpOpenRequest( hConnect, L"GET", url, L"HTTP/1.1", WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0 );
	}
	if ( hRequest )
	{
		bResults = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );
	}
	if ( bResults )
	{
		bResults = WinHttpReceiveResponse( hRequest, NULL );
	}
	if ( bResults )
	{
		do
		{
			/* Check for available data. */
			dwSize = 0;
			if ( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
			{
				printf( "Error %u in WinHttpQueryDataAvailable.\n", GetLastError() );
				break;
			}
			if ( !dwSize )
				break;
			pszOutBuffer = new char[dwSize + 1];
			if ( !pszOutBuffer )

			{
				printf( "Out of memory\n" );
				break;
			}
			ZeroMemory( pszOutBuffer, dwSize + 1 );
			if ( !WinHttpReadData( hRequest, (LPVOID) pszOutBuffer, dwSize, &dwDownloaded ) )

			{
				printf( "Error %u in WinHttpReadData.\n", GetLastError() );
			}else  {
				/* printf("%s", pszOutBuffer); */
				ret += pszOutBuffer;
			}
			delete[] pszOutBuffer;
			if ( !dwDownloaded )
				break;
		}
		while ( dwSize > 0 );
	}
	if ( hRequest )
		WinHttpCloseHandle( hRequest );

	if ( hConnect )
		WinHttpCloseHandle( hConnect );

	if ( hSession )
		WinHttpCloseHandle( hSession );

	return(ret);
}


/* 13位时间戳 */
long long getTimeStamp()
{
	timeb t;
	ftime( &t );
	return(t.time * 1000 + t.millitm);
}


/* 登陆返回结果 */
bool Login( wstring studentNumber, wstring studentPassword )
{
	wstring httpRet = L"/drcom/login?callback=dr";
	httpRet += std::to_wstring( getTimeStamp() );
	httpRet += L"&DDDDD=";
	httpRet += studentNumber;
	httpRet += L"&upass=";
	httpRet += studentPassword;
	httpRet += L"&0MKKey=123456&R1=0&R3=1&R6=0&para=00&v6ip=&_=" + std::to_wstring( getTimeStamp() );
	string ret = sendRET( (httpRet.data() ) );
	cout << ret;
	return(!(ret.find( "stime" ) == string::npos) );
}


/* 真 在线 假离线 */
bool GetStates()
{
	string ret;
	ret = sendRET( L"" );
	return(!(ret.find( "NID=" ) == string::npos) );
}


/* 闪烁切换 */
void SwitchWindow( HWND hWmd )
{
	FlashWindow( hWmd, true );
	SwitchToThisWindow( hWmd, true );
}


/* 写入注册表开机启动 */
void setAutoStart()
{
	char Buf[100]; /* 当前程序绝对路径 */
	GetModuleFileNameA( GetModuleHandle( NULL ), Buf, 100 );
	const char	*szSubKey	= "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	const char	*szModule	= Buf;
	HKEY		hKey;

	/* 打开注册表启动项 */
	if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
	{
		/* 添加一个子Key,并设置值，"Mytest"并不一定是应用程序名字（不加后缀.exe） ，可以自己设置； */
		RegSetValueEx( hKey, "Mytest", 0, REG_SZ, (BYTE *) szModule, strlen( szModule ) );
		/* 关闭注册表 */
		RegCloseKey( hKey );
	}
	return;
}


int main()
{
	HWND		mHwnd;
	const short	time = 2000;    /* 间隔1000毫秒自动检测掉线 */
	mHwnd = GetConsoleWindow();     /* 取控制台窗口句柄 */
	setAutoStart();
	cout << "正在设置开机自启动(安全软件可能会拦截)" << endl;
	wifstream	in( "log.txt", ios::in );
	wstring		student, passwd;
	if ( !in.is_open() )
	{
		wcout << "log.txt is nos exits" << endl;
		wcout << "学生号:";
		wcin >> student;
		wcout << "密码:";
		wcin >> passwd;
		wofstream out( "log.txt", ios::out );
		out << student << " " << passwd;
		out.close();
	}else  {
		in >> student >> passwd;
		in.close();
	}
	int counts = 1;
	while ( 1 )
	{
		Sleep( time );
		while ( false == GetStates() ) /* 如果掉线了 */
		{
			SwitchWindow( mHwnd );
			cout << counts << ":" << "掉线了,重连中" << endl;

			if ( false == Login( student, passwd ) )
				cout << "重连失败" << endl;
			else{
				cout << "连接成功" << endl;
				break;
			}
			Sleep( 3200 );
		}
		cout << counts << ":" << "在线" << endl;
		counts++;
	}
	return(0);
}


