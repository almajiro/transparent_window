/**
 * @file transparent_testing.cpp
 * @brief application window transparent tool.
 * @author almajiro
 * @date 2018/01/18
 */

 /**
  * @note
  * TCHAR = char
  * LPCTSTR = const char*
  * LPTSTR = char*
  * when UNICODE defined
  * TCHAR = WCHAR
  * LPCTSTR = const WCHAR*
  * LPTSTR = WCHAR*
  *
  * change the console color
  * SetConsoleTextAttribute
  *    - https://msdn.microsoft.com/ja-jp/library/cc429756.aspx
  * Console color list
  *    - http://www.geocities.jp/gameprogrammingunit/win/console/color.htm
  * PDCurses
  * rewind - clear the keyboard buffer
  * fflush - clear the file buffer
  */

  /**
   * features
   * 1.		transparent the application window.
   * 2.		trigger the keyboard, and when hit the shortcut the application window set to 0%.
   * ex.	keep the code clean
   */

 // include headers
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winuser.h>
#include <psapi.h>
#include <string.h>
#include <conio.h>

#define VERSION "1.0b"

// is window owner?
#define IsWindowOwner(h) (GetWindow(h,GW_OWNER) == NULL)

//! running applications data
struct { TCHAR title[1024]; DWORD pid; } windows[100];

//! running applications counter
int windowCounter = 0;

char ktoc[256][256];

//! console standart output handler
HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

//! console color
WORD dangerColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
WORD defaultColor = FOREGROUND_GREEN;
WORD highGreenColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
WORD promptColor = FOREGROUND_GREEN | FOREGROUND_RED;

//! mutex handle
HANDLE hMSP;

// function list
BOOL __transparentWindow(HWND target, int alpha);					// ok
BOOL transparentWindow(int id, int alpha);							// ok
BOOL IsEnumCheck(HWND hWnd, LPCTSTR lpTitle, LPCTSTR lpClass);		// ok
BOOL changeTransparentUsingArrowKey(int id);						// ok
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);			// ok
HWND gethWndfromWindows(int TargetID);								// ok
BOOL setConsole(WORD wAttributes);									// ok
void setDefault();													// ok
void selected(int id);
void dispSelectedMenu();											// ok
void dispSelectedHeader(int id);									// ok
void triggerMenu(int id);
void dispTriggerMenu();												// ok
void dispTriggerHeader();											// ok
void displayMessage(LPCTSTR message, bool status);					// ok
char getChoice();													// ok
char listApplications();											// ok
int inputNumber();													// ok

BOOL __transparentWindow(HWND target, int alpha) {
	COLORREF r = RGB(255, 255, 255);

	//! get window data with extended window style mode.
	long i = GetWindowLong(target, GWL_EXSTYLE);

	// changes an attribute of the specified window.
	SetWindowLong(target, GWL_EXSTYLE, i | WS_EX_LAYERED);

	// sets the opacity and transparency color key of a layered window.
	SetLayeredWindowAttributes(target, r, alpha, LWA_ALPHA);

	return true;
}

/**
 * @brief transparent the application window.
 * @param id integer application id.
 * @param alpha integer window transparency
 * @return window handler HWND
 */
BOOL transparentWindow(int id, int alpha)
{
	//! window handler
	HWND hWnd = gethWndfromWindows(id);

	// if failed to get window handler.
	if (hWnd == NULL) return false;

	__transparentWindow(hWnd, alpha);

	return true;
}

/**
 * @brief enum check
 * @param hWnd HWND window handler
 * @param LPCTSTR lpTitle title
 * @param LPCTSTR lpClass class
 * @return boolean window status
 */
BOOL IsEnumCheck(HWND hWnd, LPCTSTR lpTitle, LPCTSTR lpClass)
{
	if (IsWindowVisible(hWnd)) {
		if (IsWindowOwner(hWnd)) {
			if (lpTitle[0] != TEXT('\0')) {
				if (lstrcmp(lpClass, TEXT("Progman")) != 0) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/**
 * @brief callback function for EnumWindows
 * @param hWnd HWND window handler
 * @param lParal LPARAM application defined value
 * @return boolean true
 */
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	//! title buffer
	TCHAR szTitle[1024];
	TCHAR szClass[1024];

	GetWindowText(hWnd, szTitle, sizeof(szTitle));
	GetClassName(hWnd, szClass, sizeof(szClass));

	if (IsEnumCheck(hWnd, szTitle, szClass)) {
		DWORD ProcessID;
		GetWindowThreadProcessId(hWnd, &ProcessID);

		windows[windowCounter].pid = ProcessID;
		strcpy_s(windows[windowCounter].title, szTitle);
		windowCounter++;

		TCHAR *pt = szTitle;
		int i = 0;

		printf("|");
		setConsole(dangerColor);
		_tprintf(TEXT(" No.%03d "), windowCounter);
		setConsole(defaultColor);
		printf("| ");

		while (*pt != '\0') {
			setConsole(highGreenColor);
			putchar(*pt);

			if (i == 65) {
				setConsole(defaultColor);
				printf(" |\n");
				printf("|        | ");
				i = 0;
			}
			else {
				i++;
			}

			pt++;
		}

		setConsole(defaultColor);
		if (i <= 65) {
			for (int j = 0; j < 67 - i; j++) {
				printf(" ");
			}
			puts("|");
		}
		puts("+--------+--------------------------------------------------------------------+");
	}
	return TRUE;
}

/**
 * @brief get window handler
 * @param id integer application id.
 * @return window handler HWND
 */
HWND gethWndfromWindows(int id)
{
	//! get top window handler
	HWND hWnd = GetTopWindow(NULL);

	do {
		DWORD ProcessID;

		//! title buffer
		TCHAR szTitle[1024];

		GetWindowThreadProcessId(hWnd, &ProcessID);
		GetWindowText(hWnd, szTitle, sizeof(szTitle));

		if (windows[id].pid == ProcessID && !lstrcmp(windows[id].title, szTitle)) {
			return hWnd;
		}
	} while ((hWnd = GetNextWindow(hWnd, GW_HWNDNEXT)) != NULL);

	return NULL;
}

/**
 * @brief set all windows to 100%.
 */
void setDefault()
{
	for (int i = 0; i < windowCounter; i++) {
		transparentWindow(i, 255);
	}
	puts("");
	displayMessage("すべてのアプリケーションの透明度を元に戻しました。", false);

	// wait for 2 seconds.
	Sleep(2000);
}

/**
* @brief display the properties page menu
*/
void dispSelectedMenu() {
	puts("+----------+------------------------------------------------------------------+");
	puts("   T > 透明度を矢印キーで設定 (←↑↓→)");
	puts("   C > 透明度を数値で入力 (0-100)");
	puts(" ESC > 戻る");
	puts("-------------------------------------------------------------------------------");
}

/**
* @brief display the trigger page menu
*/
void dispTriggerMenu()
{
	puts("+----------+-----------------------------------------+------------------------+");
	puts("   R > トリガーの起動");
	puts("   S > トリガーキーの設定");
	puts("   A > アクションキーの設定");
	puts("   P > 透明度の設定");
	puts(" ESC > 戻る");
	puts("-------------------------------------------------------------------------------");
}

/**
 * @brief display the properties page header
 * @param id integer application id.
 */
void dispSelectedHeader(int id)
{
	system("cls");

	setConsole(defaultColor);
	puts("+----------------------------+                       +------------------------+");
	puts("| 選択されたアプリケーション |                       | ウィンドウ透明化ツール |");
	puts("+----------+-----------------+-----------------------+------------------------+");
	_tprintf("| タスク名 | ");

	TCHAR *pt = windows[id].title;
	int i = 0;

	while (*pt != 0) {
		setConsole(highGreenColor);
		putchar(*pt);

		setConsole(defaultColor);
		if (i == 63) {
			printf(" |\n");
			printf("|          | ");
			i = 0;
		}
		else {
			i++;
		}

		pt++;
	}

	if (i <= 63) {
		setConsole(defaultColor);
		for (int j = 0; j < 65 - i; j++) {
			printf(" ");
		}
		puts("|");
	}

	setConsole(defaultColor);
	_tprintf("|   PID    | %4d                                                             |\n", windows[id].pid);
}

/**
 * @brief display the trigger menu header
 */
void dispTriggerHeader()
{
	setConsole(defaultColor);
	puts("+----------+                                         +------------------------+");
	puts("| トリガー |                                         | ウィンドウ透明化ツール |");
	//puts("+----------+-----------------------------------------+------------------------+");
}

/**
 * @brief get one character.
 * @return character code.
 */
char getChoice()
{
	char ch;
	ch = toupper(_getche());
	_putch('\n');
	return ch;
}

/**
 * @brief change transparent using arrow key.
 * @param id integer application id.
 * @return hwnd error status in boolean.
 */
BOOL changeTransparentUsingArrowKey(int id)
{
	//! character buffer
	char ch;

	//! counter
	int i;

	//! window alpha
	int alpha = 255;

	transparentWindow(id, alpha);
	_putch('\n');

	setConsole(dangerColor);
	puts(" >>> ESCで戻る");
	setConsole(promptColor);

	while (1) {
		ch = 0;

		printf("\r\t透明度:\t");
		printf("|");
		for (i = 0; i < alpha / 5; i++) {
			if (i * 5 <= 150)
				setConsole(dangerColor);
			else if (i * 5 >= 200)
				setConsole(defaultColor);
			else
				setConsole(promptColor);

			if ((alpha / 5 - 1) == i) {
				printf(">");
			}
			else {
				printf("-");
			}
		}
		for (; i < 51; i++) {
			printf(" ");
		}

		setConsole(promptColor);
		printf("|  [");

		if (alpha <= 150)
			setConsole(dangerColor);
		else if (alpha >= 200)
			setConsole(defaultColor);
		else
			setConsole(promptColor);

		printf("%003.f%%", ((float)alpha / (float)255 * 100));

		setConsole(promptColor);
		printf("] ");

		ch = _getch();
		if (ch == 0x48 || ch == 0x4d) {
			if (alpha < 255) {
				alpha += 5;
				if (!transparentWindow(id, alpha)) return false;
			}
		}
		if (ch == 0x50 || ch == 0x4b) {
			if (alpha > 5) {
				alpha -= 5;
				if (!transparentWindow(id, alpha)) return false;
			}
		}

		if (ch == 0x1B) break;
		Sleep(1);
	}

	// successful to exit property
	return true;
}

/**
 * @brief show properties.
 * @param id integer application id.
 */
void selected(int id)
{
	//! alpha temp
	int input, alpha;

	//! choice temp
	char choice;

	//! hwnd error flag
	bool hwnd_flag = false;

	//! menu sleep flag
	bool menu_flag = true;

	while (1)
	{
		menu_flag = true;

		dispSelectedHeader(id);
		dispSelectedMenu();

		setConsole(promptColor);
		printf(" 選択> ");
		choice = getChoice();

		switch (choice) {
		case 'C':	// change transparent by value
			while (1) {
				setConsole(promptColor);
				printf(" 透明度[%%] (ESCで戻る)> ");
				input = inputNumber();

				if (input == -1 || (input >= 0 && input <= 100)) break;
				displayMessage("正しい数値を入力してください。", true);
			}

			if (input == -1) {
				menu_flag = false;
				break;
			}

			alpha = ((float)input / 100) * (float)255;
			if (transparentWindow(id, alpha)) {
				setConsole(defaultColor);
				printf(" >>> ウィンドウの透明度を%d%%へ変更しました。\n", input);
				displayMessage("一つ前のメニューへ戻ります。", false);
			}
			else
				hwnd_flag = true;

			break;
		case 'T':	// change transparent using arrow key
			hwnd_flag = !changeTransparentUsingArrowKey(id);
			menu_flag = hwnd_flag;
			puts("\n");
			break;

		default:
			menu_flag = false;
		}

		if (choice == 0x1B) break;

		if (hwnd_flag) {
			displayMessage("HWNDが取得できませんでした。ホームへ戻ります。", true);
			Sleep(2000);
			break;
		}

		if (menu_flag)
			Sleep(2000);

	}
}

/**
 * @brief list applications and return the option.
 * @return option (character code).
 */
char listApplications()
{
	//! charactor code temp
	char ch;

	do {
		setConsole(defaultColor);

		// clear the windows list.
		windowCounter = 0;
		for (int i = 0; i < 100; i++) {
			windows[i].pid = NULL;
			strcpy_s(windows[i].title, TEXT(""));
		}

		// clear the command line.
		system("cls");

		puts("+----------------------------------+                 +------------------------+");
		puts("| 現在起動しているアプリケーション |                 | ウィンドウ透明化ツール |");
		puts("+--------+-------------------------+-----------------+------------------------+");
		puts("| No一覧 |                       アプリケーション名                           |");
		puts("+--------+--------------------------------------------------------------------+");

		// display the applications.
		EnumWindows(EnumWindowsProc, (LPARAM)0);

		setConsole(promptColor);
		printf(" R:リスト更新, S:設定, X:すべて元に戻す, T:トリガー ");

		setConsole(dangerColor);
		printf("ESC:終了");
		setConsole(promptColor);

		printf("> ");

	} while ((ch = toupper(_getche())) == 'R');

	return ch;
}

/**
 * @brief get integer value.
 * @return integer value or -1 (ESC CODE).
 */
int inputNumber() {

	//! getch temp.
	char ch;

	//! string buffer max 255.
	char buf[4];

	//! buffer counter.
	int i = 0;

	//! escape flag.
	bool esc = false;

	while ((ch = _getch()) != 13) {
		if (i < 3) {
			if (ch >= '0' && ch <= '9') {
				putchar(ch);
				buf[i++] = ch;
			}

			//! 0x1B == ESC
			if (ch == 0x1B) {
				esc = true;
				break;
			}
		}

		if (i <= 3) {
			// backspace
			if (ch == '\b' && i > 0) {
				printf("\b \b");
				i--;
			}
		}
	}

	if (esc) return -1;

	buf[i] = '\0';
	puts("");

	return atoi(buf);
}

/**
 * @brief set console color
 * @return boolean
 */
BOOL setConsole(WORD wAttributes)
{
	return SetConsoleTextAttribute(hConsoleOutput, wAttributes);
}

/**
 * @brief display message
 * @param status boolean 1=error 0=message
 * @return boolean
 */
void displayMessage(LPCTSTR message, bool status)
{
	setConsole(status ? dangerColor : defaultColor);
	printf(" >>> ");
	puts(message);
}

/**
 * @brief display trigger menu
 * @param id integer application id.
 */
void triggerMenu(int id)
{
	TCHAR newTitle[1024];
	TCHAR oldTitle[1024];

	bool visible = true;
	bool hwnd_active = true;
	bool kill = false;
	char choice;

	static int val_max = 255, val_min = 0;
	static int _val_max = 100, _val_min = 0;
	static char key[2] = { VK_CONTROL, VK_SPACE };
	static char key_child[5] = { '1', '2', '3', '4', '5' };

	HWND target = gethWndfromWindows(id);

	do {
		system("title ウィンドウ透明化ツール");

		setConsole(defaultColor);
		system("cls");
		dispTriggerHeader();
		dispTriggerMenu();

		setConsole(promptColor);
		printf(" 選択> ");
		choice = getChoice();

		// menu
		if (choice == 'R') {
			system("title ウィンドウ透明化ツール - トリガー起動中");
			while (1) {

				if (!GetWindowText(target, newTitle, sizeof(newTitle))) {
					hwnd_active = false;
					break;
				}

				if (strcmp(oldTitle, newTitle) || choice == 'R') {
					choice = 0;
					system("cls");

					setConsole(defaultColor);
					puts("<<<トリガー起動中>>>\n");
					setConsole(promptColor);

					printf("アプリケーション名\n >");
					puts(newTitle);
					strcpy_s(oldTitle, newTitle);

					setConsole(defaultColor);
					printf("\n >ウィンドウ透明度の最高値: %d%%\n", _val_max);
					printf(" >ウィンドウ透明度の最小値: %d%%\n\n", _val_min);

					setConsole(promptColor);
					puts("現在設定されているショートカットキー");
					printf(" >%s + %s\n", ktoc[key[0]], ktoc[key[1]]);
					printf(" >> + %s ウィンドウの透明度を設定\n", ktoc[key_child[0]]);
					printf(" >> + %s ウィンドウを最大化\n", ktoc[key_child[1]]);
					printf(" >> + %s ウィンドウを最小化\n", ktoc[key_child[2]]);
					printf(" >> + %s ウィンドウを元に戻す\n", ktoc[key_child[3]]);
					printf(" >> + %s ターゲットのアプリケーションを終了\n\n", ktoc[key_child[4]]);

					displayMessage("ESCでトリガーを停止します。\n", 1);

				}

				if ((GetKeyState(key[0]) & 0x8000) && (GetKeyState(key[1]) & 0x8000)) {
					bool actionFlag = false;

					printf("\r                                                                              \r");

					if (GetKeyState(key_child[1]) & 0x8000) {
						ShowWindow(target, SW_MAXIMIZE);
						printf("\r >>> ウィンドウを最大化しました。");
						actionFlag = true;
					}

					if (GetKeyState(key_child[2]) & 0x8000) {
						ShowWindow(target, SW_MINIMIZE);
						printf("\r >>> ウィンドウを最小化しました。");
						actionFlag = true;
					}

					if (GetKeyState(key_child[3]) & 0x8000) {
						ShowWindow(target, SW_RESTORE);
						printf("\r >>> ウィンドウを元に戻しました。");
						actionFlag = true;
					}

					if (GetKeyState(key_child[4]) & 0x8000) {
						setConsole(dangerColor);
						puts("トリガーターゲットのアプリケーションを終了します。");
						::PostMessage(target, WM_CLOSE, 0, 0);
						Sleep(2000);
						kill = true;
						break;
					}

					if (GetKeyState(key_child[0]) & 0x8000) {
						actionFlag = true;
						visible = !visible;

						if (visible) {
							setConsole(defaultColor);
							printf("\r >>> 現在トリガーされているアプリケーションの透明度は最高値へ設定されています。");
							__transparentWindow(target, val_max);
						}
						else {
							setConsole(dangerColor);
							printf("\r >>> 現在トリガーされているアプリケーションの透明度は最小値へ設定されています。");
							__transparentWindow(target, val_min);
						}
					}

					if (actionFlag) Sleep(500);
				}

				if (_kbhit()) {
					if (_getch() == 0x1B) break;
				}

			}
		}
		if (choice == 'S') {
			setConsole(defaultColor);
			puts("\n >>> トリガーキーの設定");
			puts(" >>> 準備ができた場合はキーを押してください(ESCで戻る)");

			if (_getch() != 0x1B) {
				int i, j, k;
				bool flag;
				bool error;

				for (j = 0; j < 2; j++) {
					setConsole(defaultColor);
					printf(" >>> %dつ目のトリガーキー\n", j + 1);

					flag = false;
					error = false;

					for (int i = 3; i >= 0; i--) {
						printf("\r >>> カウントダウン: %d", i);
						Sleep(1000);
					}
					puts("");

					for (i = 0; i < 256; i++) {
						if (GetKeyState(i) & 0x8000) {
							if (i >= 0x15 && 0x19 >= i) break;
							if (i >= 0x1C && 0x1F >= i) break;
							if (i >= 0x29 && 0x2B >= i) break;
							if (i >= 0xE5 && 0xFE <= i) break;
							if (i >= 0xE8 || i == 0x2F || i == 0xE5 || i == 0x6C) break;

							setConsole(promptColor);
							printf(" >>> %s\n", ktoc[i]);
							setConsole(defaultColor);

							for (k = 0; k < 5; k++) {
								if (key_child[k] == i) {
									error = true;
									break;
								}
							}

							if (j == 1 && key[0] == i) {
								error = true;
							}

							if (error) break;

							flag = true;
							break;
						}
					}

					if (flag) {
						key[j] = i;
					}
					else if (error) {
						displayMessage("既に設定されているトリガーキー又はアクションキーは設定できません。", true);
						j--;
						Sleep(1000);
					}
					else {
						displayMessage("キーが押されませんでした。", true);
						break;
					}
				}

				displayMessage("一つ前のメニューへ戻ります。", false);
				Sleep(1500);
			}

		}
		if (choice == 'P') {
			char ch;

			setConsole(defaultColor);
			puts("\n >>> 透明度の設定");
			puts("	   E > ウィンドウ透明度の最高値(0-100)");
			puts("	   D > ウィンドウ透明度の最小値(0-100)");
			puts("	 ESC > 戻る\n");

			do {
				setConsole(promptColor);
				printf("透明度の設定 > 選択> ");
				ch = getChoice();

				if (ch == 'E') {
					int num;
					while (1) {
						printf("ウィンドウ透明度の最高値(ESCで戻る) > ");
						num = inputNumber();
						if (num == 0) puts("0");
						if ((num <= 100 && num >= 0) || num == -1) break;
						displayMessage("正しい数値を入力してください。", true);
					};

					if (num != -1) {
						_val_max = num;
						val_max = ((float)num / 100) * (float)255;
					}
					else puts("");
				}

				if (ch == 'D') {
					int num;
					while (1) {
						printf("ウィンドウ透明度の最小値(ESCで戻る) > ");
						num = inputNumber();
						if ((num <= 100 && num >= 0) || num == -1) break;
						displayMessage("正しい数値を入力してください。", true);
					};

					if (num != -1) {
						_val_min = num;
						val_min = ((float)num / 100) * (float)255;
					}
					else puts("");
				}
				fflush(stdin);

			} while (ch != 0x1B);
		}
		if (choice == 'A') {
			char ch;
			bool runFlag;
			int target;

			setConsole(defaultColor);
			puts("\n >>> アクションキーの設定");
			puts("	   T > ウィンドウの透明度を設定");
			puts("	   M > ウィンドウを最大化");
			puts("	   N > ウィンドウを最小化");
			puts("	   R > ウィンドウを元に戻す");
			puts("	   K > ターゲットのアプリケーションを終了");
			puts("	 ESC > 戻る\n");

			do {
				setConsole(promptColor);
				printf("アクションキーの設定 > 選択> ");

				ch = getChoice();

				switch (ch) {
					case 'T':
						displayMessage("ウィンドウの透明度を設定", false);
						target = 0;
						runFlag = true;
						break;

					case 'M':
						displayMessage("ウィンドウを最大化", false);
						target = 1;
						runFlag = true;
						break;

					case 'N':
						displayMessage("ウィンドウを最小化", false);
						target = 2;
						runFlag = true;
						break;

					case 'R':
						displayMessage("ウィンドウを元に戻す", false);
						target = 3;
						runFlag = true;
						break;

					case 'K':
						displayMessage("ターゲットのアプリケーションを終了", false);
						target = 4;
						runFlag = true;
						break;

					default:
						runFlag = false;
				}

				if (runFlag) {
					puts(" >>> 準備ができた場合はキーを押してください(ESCで戻る)");
					if (_getch() != 0x1B) {
						int i, j, k;
						bool flag = false;
						bool error = false;

						for (int i = 3; i >= 0; i--) {
							printf("\r >>> カウントダウン: %d", i);
							Sleep(1000);
						}
						puts("");

						for (i = 0; i < 256; i++) {
							if (GetKeyState(i) & 0x8000) {
								if (i >= 0x15 && 0x19 >= i) break;
								if (i >= 0x1C && 0x1F >= i) break;
								if (i >= 0x29 && 0x2B >= i) break;
								if (i >= 0xE5 && 0xFE <= i) break;
								if (i >= 0xE8 || i == 0x2F || i == 0xE5 || i == 0x6C) break;

								setConsole(promptColor);
								printf(" >>> %s\n", ktoc[i]);
								setConsole(defaultColor);

								for (k = 0; k < 5; k++) {
									if (i == target) break;
									if (key_child[k] == i) {
										error = true;
										break;
									}
								}

								for (k = 0; k < 2; k++) {
									if (key[k] == i) {
										error = true;
										break;
									}
								}

								if (error) break;

								flag = true;
								break;
							}
						}

						if (flag) {
							key_child[target] = i;
							displayMessage("設定されました。", false);
							Sleep(2000);
						}
						else if (error) {
							displayMessage("既に設定されているトリガーキー又はアクションキーは設定できません。", true);
							Sleep(1000);
						}
						else {
							displayMessage("キーが押されませんでした。", true);
							Sleep(1000);
						}
						
						//fflush(stdin); // not working.
						if (_kbhit())
							_getch();
					}
				}
			} while (ch != 0x1B);
		}

		// exit
		if (!hwnd_active) {
			system("cls");
			displayMessage("無効なウィンドウハンドラです。", true);
			displayMessage("トップメニューへ戻ります。", true);
			Sleep(2000);
			break;
		}
		if (kill) break;
	} while (choice != 0x1B);
}

/**
 * @brief the main function.
 * @return return the status code to system.
 */
int main()
{
	//! for windows id
	int id;

	//! for options
	char ch;

	system("title ウィンドウ透明化ツール - Initializing");

	setConsole(highGreenColor);
	printf("ウィンドウ透明化ツール V");
	puts(VERSION);

	setConsole(dangerColor);
	puts("Initializing...");

	// initializing ktoc array
	strcpy_s(ktoc[VK_LBUTTON], "マウス左ボタン");
	strcpy_s(ktoc[VK_RBUTTON], "マウス右ボタン");
	strcpy_s(ktoc[VK_CANCEL], "マウス右ボタン");
	strcpy_s(ktoc[VK_MBUTTON], "マウス中央ボタン");
	strcpy_s(ktoc[VK_BACK], "Backspace");
	strcpy_s(ktoc[VK_TAB], "Tab");
	strcpy_s(ktoc[VK_CLEAR], "NumLock を外した状態のテンキー5");
	strcpy_s(ktoc[VK_RETURN], "Enter");
	strcpy_s(ktoc[VK_SHIFT], "Shift");
	strcpy_s(ktoc[VK_CONTROL], "Ctrl");
	strcpy_s(ktoc[VK_MENU], "Alt");
	strcpy_s(ktoc[VK_PAUSE], "Pause");
	strcpy_s(ktoc[VK_CAPITAL], "Caps Lock");
	strcpy_s(ktoc[VK_ESCAPE], "Esc");
	strcpy_s(ktoc[VK_SPACE], "Spacebar");
	strcpy_s(ktoc[VK_PRIOR], "Page Up");
	strcpy_s(ktoc[VK_NEXT], "Page Down");
	strcpy_s(ktoc[VK_END], "End");
	strcpy_s(ktoc[VK_HOME], "Home");
	strcpy_s(ktoc[VK_ESCAPE], "Esc");
	strcpy_s(ktoc[VK_LEFT], "左方向キー");
	strcpy_s(ktoc[VK_UP], "上方向キー");
	strcpy_s(ktoc[VK_RIGHT], "右方向キー");
	strcpy_s(ktoc[VK_DOWN], "下方向キー");
	strcpy_s(ktoc[VK_SNAPSHOT], "Insert");
	strcpy_s(ktoc[VK_SNAPSHOT], "PrintScreen");
	strcpy_s(ktoc[VK_DELETE], "Delete");
	strcpy_s(ktoc[VK_LWIN], "左 Windows キー");
	strcpy_s(ktoc[VK_RWIN], "右 Windows キー");
	strcpy_s(ktoc[VK_APPS], "アプリケーションキー");
	strcpy_s(ktoc[VK_MULTIPLY], "テンキー *");
	strcpy_s(ktoc[VK_ADD], "テンキー +");
	strcpy_s(ktoc[VK_SUBTRACT], "テンキー +");
	strcpy_s(ktoc[VK_DECIMAL], "テンキー .");
	strcpy_s(ktoc[VK_DIVIDE], "テンキー /");
	strcpy_s(ktoc[VK_NUMLOCK], "Num Lock");
	strcpy_s(ktoc[VK_SCROLL], "	Scroll");
	strcpy_s(ktoc[VK_LSHIFT], "左Shiftキー");
	strcpy_s(ktoc[VK_RSHIFT], "右Shiftキー");
	strcpy_s(ktoc[VK_LCONTROL], "左Ctrlキー");
	strcpy_s(ktoc[VK_RCONTROL], "右Ctrlキー");
	strcpy_s(ktoc[VK_LMENU], "左Altキー");
	strcpy_s(ktoc[VK_RMENU], "右Altキー");

	// numpad and 0-9
	for (int i = 0; i < 10; i++) {
		char temp[255], num_temp[2];
		strcpy_s(temp, "NumPad");
		_itoa_s(i, num_temp, sizeof(char) * 2, 10);
		strcat_s(temp, num_temp);
		strcpy_s(ktoc[0x60 + i], temp);
		strcpy_s(ktoc[0x30 + i], num_temp);
	}

	// main keyboard a-z
	for (int i = 0x41; i <= 0x5A; i++) {
		int j;
		j = toupper(i);

		char temp[2] = { j, '\0' };
		strcpy_s(ktoc[i], temp);
	}
	puts("ktoc array initialized.");

	hMSP = CreateMutex(NULL, TRUE, "Application Window Transparent Tool");

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		MessageBox(NULL, "既にアプリケーションが起動しています。", "エラー", MB_OK);
		ReleaseMutex(hMSP);
		CloseHandle(hMSP);
		return 1;
	}

	system("title ウィンドウ透明化ツール");

	// main loop
	while (1)
	{
		//! list application list and get options.
		ch = listApplications();

		switch (ch) {
		case 'X':
			// set all windows to 100%
			setDefault();
			break;

		case 'T':
		case 'S':
			puts("");

			while (1) {
				setConsole(dangerColor);
				printf(" No");
				setConsole(promptColor);
				printf("を入力(ESCで戻る) > ");
				id = inputNumber();
				if ((id <= windowCounter && id > 0) || id == -1) break;
				displayMessage("正しいアプリケーション番号を入力してください。", true);
			};

			// back to the application list.
			if (id == -1) break;

			if (ch == 'T')
				triggerMenu(--id);
			else
				selected(--id);

			break;
		}

		//! 0x1B == ESC
		if (ch == 0x1B) break;
	}

	ReleaseMutex(hMSP);
	CloseHandle(hMSP);

	// return code
	return 0;
}