#include "DirWatcher.h"

DirWatcher::DirWatcher(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//----------------------------界面设计------------------------------
	this->setWindowTitle("Directory Watcher");
	this->setMinimumSize(600, 400);

	ui.labelDir->setText("Target Directory:");
	ui.lineEditDir->setReadOnly(true);
	ui.buttonBrowser->setText("Browse");
	ui.plainTextEdit->setReadOnly(true);
	ui.buttonStart->setText("Start");
	ui.buttonClear->setText("Clear");
	ui.buttonChange->setText("Change Directory");
	//----------------------------界面设计完毕------------------------------

	// 清除逻辑
	connect(ui.buttonClear, &QPushButton::clicked, [=]() {
		ui.plainTextEdit->setPlainText("");
		});

	// 开始监控条件 改变目录条件
	connect(ui.lineEditDir, &QLineEdit::textChanged, [=](const QString& text) {
		if (!text.isEmpty()) {
			ui.buttonStart->setEnabled(true);
			ui.buttonChange->setEnabled(true);
		}
		});


	// 选择文件夹逻辑
	connect(ui.buttonBrowser, &QPushButton::clicked, [=]() {
		QString qDir = QFileDialog::getExistingDirectory(this, 
			"Choose a Directory", "C:/", QFileDialog::ShowDirsOnly);
		if (!qDir.isEmpty()) {
			ui.lineEditDir->setText(qDir.replace('/', '\\') + '\\');
			ui.buttonBrowser->setEnabled(false);
		}
		});

	// 改变目录（重启）逻辑
	connect(ui.buttonChange, &QPushButton::clicked, [=]() {
		if (QMessageBox::Ok == QMessageBox::question(this, "Change Directory to Watch",
			"Changing Directory need to Restart process\nAre you sure?",
			QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel)) {

			// 消失窗口
			this->hide();

			// 获取我们进程的全路径
			TCHAR szApplication[MAX_PATH];
			DWORD cchLength = _countof(szApplication);
			QueryFullProcessImageName(GetCurrentProcess(), 0, szApplication, &cchLength);
			// 初始化结构
			SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
			sei.lpVerb = TEXT("open"); // 正常打开模式
			sei.lpFile = szApplication;
			sei.lpParameters = NULL;
			sei.nShow = SW_SHOWNORMAL; // 这一步别忘了 不然没有窗口显示
			// 打开我们的程序
			ShellExecuteEx(&sei);

			if (GetLastError() == S_OK) {
				// 启动成功
				this->close();
			}
			else
			{
				// 失败
				this->show();
			}

		}
		});

	// 监控逻辑
	connect(ui.buttonStart, &QPushButton::clicked, [=]() {
		const wchar_t* pwsTmp = reinterpret_cast<const wchar_t*>(ui.lineEditDir->text().utf16());
		std::thread t1(std::mem_fn(&DirWatcher::MonitorFile), this, (PTCHAR)pwsTmp);
		t1.detach();
		ui.buttonStart->setEnabled(false);
		});

	// 初始不能监控 和 改变目录
	ui.buttonStart->setEnabled(false);
	ui.buttonChange->setEnabled(false);

	// 申请缓冲区
	pbBuf = new BYTE[BUFFER_SIZE];

}

// 监控函数
BOOL DirWatcher::MonitorFile(PTCHAR pszDirectory) {

	// 打开目录, 获取文件句柄
	HANDLE hDirectory = ::CreateFile(pszDirectory, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDirectory)
	{
		ui.plainTextEdit->appendPlainText("CreateFile failed");
		return FALSE;
	}

	BOOL bRet = FALSE;
	DWORD dwRet = 0;

	do
	{
		// 得到 缓冲区
		RtlZeroMemory(pbBuf, BUFFER_SIZE);
		PFILE_NOTIFY_INFORMATION pFileNotifyInfo = (PFILE_NOTIFY_INFORMATION)pbBuf;

		// 设置监控目录
		bRet = ::ReadDirectoryChangesW(hDirectory,
			pFileNotifyInfo,
			BUFFER_SIZE,
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |			// 修改文件名
			FILE_NOTIFY_CHANGE_DIR_NAME |           // 修改文件夹名
			FILE_NOTIFY_CHANGE_ATTRIBUTES |			// 修改文件属性
			FILE_NOTIFY_CHANGE_LAST_WRITE,			// 最后一次写入
			&dwRet,
			NULL,
			NULL);
		if (FALSE == bRet)
		{
			ui.plainTextEdit->appendPlainText("ReadDirectoryChangesW failed");
			break;
		}
		// 判断操作类型并显示
		switch (pFileNotifyInfo->Action)
		{
		case FILE_ACTION_ADDED:
		{
			// 新增文件
			// 由于pFileNotifyInfo不是以零结尾的字符串 所以需要这么处理
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, 
				pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(
				QString("[File Added Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_REMOVED:
		{
			// 移动文件
			// 由于pFileNotifyInfo不是以零结尾的字符串 所以需要这么处理
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Removed Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_MODIFIED:
		{
			// 修改文件
			// 由于pFileNotifyInfo不是以零结尾的字符串 所以需要这么处理
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Modified Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_RENAMED_OLD_NAME:
		{
			// 重命名文件
			// 由于pFileNotifyInfo不是以零结尾的字符串 所以需要这么处理
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Renamed Action] OldName: %1").arg(QString::fromWCharArray(wsFileName)));

			// 下一个FILE_NOTIFY_INFORMATION块就是重命名文件时的新名字
			PFILE_NOTIFY_INFORMATION pNewFileNotifyInfo = 
				(PFILE_NOTIFY_INFORMATION)((PBYTE)pFileNotifyInfo + pFileNotifyInfo->NextEntryOffset);
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pNewFileNotifyInfo->FileName, pNewFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Renamed Action] NewName: %1").arg(QString::fromWCharArray(wsFileName)));

			break;
		}
		default:
		{
			break;
		}
		}

	} while (bRet);


	// 关闭句柄, 释放内存
	::CloseHandle(hDirectory);

	return TRUE;
}
