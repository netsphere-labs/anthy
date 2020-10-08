// -*- coding:utf-8-with-signature -*-

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <assert.h>

#define STRICT 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <tchar.h>
#include <sddl.h>
#include <versionhelpers.h>

#ifndef SDDL_ALL_APP_PACKAGES
#define SDDL_ALL_APP_PACKAGES               TEXT("AC")      // All applications running in an app package context
#endif


// @return -1 ReadFile() error. パイプを作り直せ.
//         0  EOF
static int dispatch_request( HANDLE hPipe )
{
  assert(hPipe != INVALID_HANDLE_VALUE );

  char buf[2000];
  
  // 1行読む
  DWORD bytes_read = 0;
  BOOL r = ReadFile(hPipe, buf, sizeof(buf) - 1, &bytes_read, NULL);
  if ( !r ) {
    printf("ReadFile() failed: %d\n", GetLastError());
    return -1;
  }
  if (bytes_read == 0)
    return 0;
  
  buf[bytes_read] = '\0';

  if (strstr(buf, "BYE")) {
    printf("Normaly BYE\n");
    return 0;
  }

  // ここからテスト..............
  // 接続元情報
  TCHAR user_name[100];
  GetNamedPipeHandleState( hPipe, NULL, NULL, NULL, NULL, user_name,
                                sizeof(user_name) / sizeof(TCHAR));
  wprintf(L"user_name = %s\n", user_name);

  // 返信
  char write_buf[2000];
  strcpy(write_buf, buf); // echo
  strcat(write_buf, "\nhogehoge\nEND\n");

  DWORD bytes_written = 0;
  r = WriteFile(hPipe, write_buf, strlen(write_buf), &bytes_written, NULL);
  if ( !r ) {
    printf("WriteFile() failed: %d\n", GetLastError());
    return -1;
  }

  FlushFileBuffers(hPipe);
  return 1;
}


struct PipeData
{
    OVERLAPPED overlap;
    HANDLE hPipe;
};


// @return 成功時 0, エラー時 -1
static int listen_pipe( PipeData* pipe_data )
{
  assert( pipe_data );
  assert( pipe_data->hPipe != INVALID_HANDLE_VALUE);

  // FILE_FLAG_OVERLAPPED pipe では, 直ちに戻る.
  BOOL fConnected = ConnectNamedPipe(pipe_data->hPipe, &pipe_data->overlap);
  assert( !fConnected );

  DWORD err = GetLastError();
  switch (err)
  {
  case ERROR_IO_PENDING:
    // この場合は, signaled になっていない.
    break;
  case ERROR_PIPE_CONNECTED:
    // A client connects in the interval between the call to
    // CreateNamedPipe() and the call to ConnectNamedPipe().
    break;
  default:
    {
      printf("ConnectNamedPipe() failed: %d\n", err);
      return -1;
    }
  }

  return 0;
}


/**
 * Obtains the SID of the user running this thread or process.
 * @param [out] ppSidUser the SID string of the current user,
 * @return FALSE  could not obtain the user SID
 *
 * See https://support.microsoft.com/en-us/help/813414/how-to-create-an-anonymous-pipe-that-gives-access-to-everyone
 */
static BOOL GetUserSid( LPTSTR* ppSidUser )
{
  assert( ppSidUser );

  HANDLE hToken = NULL;
  DWORD       cbName = 250;
  DWORD       cbDomainName = 250;

    //if( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) ) {
    //if( GetLastError() == ERROR_NO_TOKEN ) {
  if( !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
    return FALSE;

  TOKEN_USER* pTokenUser = nullptr;
  DWORD  dwLength = 0;
  while ( !GetTokenInformation( hToken,       // handle of the access token
                              TokenUser,    // type of information to retrieve
                              pTokenUser,   // address of retrieved information
                              dwLength,            // size of the information buffer
                              &dwLength     // address of required buffer size
                              )) {
    if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
      pTokenUser = (TOKEN_USER*) HeapAlloc( GetProcessHeap(),
                                                HEAP_ZERO_MEMORY, dwLength );
      if( pTokenUser )
	continue;
    }

    CloseHandle(hToken);
    return FALSE;
  }

  ConvertSidToStringSid(pTokenUser->User.Sid, ppSidUser);

  HeapFree( GetProcessHeap(), 0, pTokenUser );
  return TRUE;
}


BOOL create_sddl(TCHAR* sddl_str, int buf_num)
{
  assert(sddl_str);
  assert( IsWindows8OrGreater() );

  LPTSTR pszUserSid = nullptr;
  if ( !GetUserSid(&pszUserSid) )
    return FALSE;

  // DACLフォーマット
  //     D:dacl_flags(string_ace1)(string_ace2)...(string_aceN)
  // ACEフォーマット
  //     ace_type;ace_flags;rights;object_guid;inherit_object_guid;account_sid;(resource_attribute)
  // ACE type
  //    "A"  SDDL_ACCESS_ALLOWED
  // Access rights
  //     "GA" SDDL_GENERIC_ALL
  
  // 実行ユーザ (と管理者) からの接続のみを受け入れる
  _stprintf_s(sddl_str, buf_num,
	      _T("D:%s(A;;GA;;;") SDDL_RESTRICTED_CODE _T(")")
	      _T("(A;;GA;;;") SDDL_LOCAL_SYSTEM _T(")")
	      _T("(A;;GA;;;") SDDL_BUILTIN_ADMINISTRATORS _T(")")
	      _T("(A;;GA;;;%s)"), // User SID
	      (IsWindows8OrGreater() ? _T("(A;;GA;;;") SDDL_ALL_APP_PACKAGES _T(")") : _T("")),
	     pszUserSid);

  // (SDDL_MANDATORY_LABEL, SDDL_NO_WRITE_UP, SDDL_ML_LOW)
  _tcscat_s(sddl_str, buf_num - _tcslen(sddl_str), _T("S:(ML;;NW;;;LW)"));

  LocalFree(pszUserSid);
  return TRUE;
}


// パイプの名前空間 ... ユーザごとに別になる.
/*
	LPWSTR pszUserUUID = nullptr;
	if (GetUserUUID(&pszUserUUID))
	{
		_snwprintf_s(mgrpipename, _TRUNCATE, L"%s%s", IMCRVMGRPIPE, pszUserUUID);
		_snwprintf_s(mgrmutexname, _TRUNCATE, L"%s%s", IMCRVMGRMUTEX, pszUserUUID);

		LocalFree(pszUserUUID);
	}
*/


constexpr int BUFSIZE = 4000;
static TCHAR krnlobj_sddl[1000];

// @return 失敗時 -1
int create_my_pipe( const TCHAR* pipeName, PipeData* pipe_data )
{
  assert(pipe_data);

  memset(pipe_data, 0, sizeof(PipeData));
      
  // ConnectNamedPipe() に渡すのは, manual-reset event objectでなければな
  // らない.
  HANDLE hEvent = CreateEvent(NULL, // lpEventAttributes
                                TRUE, // bManualReset
                                FALSE, // bInitialState. ここ重要.
                                NULL); // lpName
  if ( !hEvent ) { // INVALID_HANDLE_VALUE ではない
    printf("CreateEvent() failed: %d\n", GetLastError());
    return -1;
  }
  pipe_data->overlap.hEvent = hEvent;

  create_sddl(krnlobj_sddl, sizeof(krnlobj_sddl) / sizeof(TCHAR));

  PSECURITY_DESCRIPTOR psd = nullptr;
  SECURITY_ATTRIBUTES sa;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(krnlobj_sddl,
                                SDDL_REVISION_1, &psd, nullptr)) {
    printf("ConvertStringSecurityDescriptorToSecurityDescriptor() failed: %d\n",
            GetLastError());
    CloseHandle(pipe_data->overlap.hEvent);
    pipe_data->overlap.hEvent = NULL;
    return -1;
  }
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = psd;
  sa.bInheritHandle = FALSE;

  // Non-blocking mode is only for LAN Man 2.0
  pipe_data->hPipe = CreateNamedPipe(pipeName,
                                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS |
                                       PIPE_WAIT, // blocking mode
                                PIPE_UNLIMITED_INSTANCES, // 最大値
                                BUFSIZE,
                                BUFSIZE,
                                0, // 0 (default) = 50 ms. 長い方がいい?
                                &sa); // lpSecurityAttributes
  LocalFree(psd);
  if (pipe_data->hPipe == INVALID_HANDLE_VALUE) {
    printf("CreateNamedPipe() failed: %d\n", GetLastError());
    CloseHandle(pipe_data->overlap.hEvent);
    pipe_data->overlap.hEvent = NULL;
    return -1;
  }

  return 0;
}


// Windows 10, version 1709:  Pipes are only supported within an
// app-container; ie, from one UWP process to another UWP process that's art
// of the same app. Also, named pipes must use the syntax "\\.\pipe\LOCAL" for
// the pipe name.
const TCHAR* pipeName = _T("\\\\.\\pipe\\my_test_pipe");

// 同時接続可能数
constexpr int LISTEN_INSTANCES = 250;

static PipeData pipe_data[LISTEN_INSTANCES];

int egg_windows()
{
  HANDLE wait_objects[LISTEN_INSTANCES];

  for (int i = 0; i < LISTEN_INSTANCES; i++) {
    if ( create_my_pipe(pipeName, pipe_data + i) < 0 ) {
      printf("create_my_pipe() failed.\n");
      return -1;
    }
    assert(pipe_data[i].hPipe != INVALID_HANDLE_VALUE);
    assert(pipe_data[i].overlap.hEvent);
    wait_objects[i] = pipe_data[i].overlap.hEvent;

    if ( listen_pipe(&pipe_data[i]) < 0 ) {
      printf("listen_pipe() failed.\n");
      return -1;
    }
  }

  // メインループ
  while (true) {
    DWORD dwWait = WaitForMultipleObjects(LISTEN_INSTANCES,
                                        wait_objects,
                                        FALSE, // does not wait for all
                                        INFINITE);
    int i = dwWait - WAIT_OBJECT_0;
    if (i < 0 || i > (LISTEN_INSTANCES - 1)) {
      printf("Index out of range.\n");
      return -1;
    }

    int r = dispatch_request(pipe_data[i].hPipe);
    if (r < 0) {
      DisconnectNamedPipe(pipe_data[i].hPipe);  // shutdown
      //CloseHandle(hPipe);
      if ( listen_pipe(&pipe_data[i]) < 0 ) {
	printf("listen_pipe() in main-loop failed.\n");
	return -1;
      }	
      continue;
    }
  }
  
  return 0;
}
