#include "DirWatcher.h"

DirWatcher::DirWatcher(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//----------------------------�������------------------------------
	this->setWindowTitle("Directory Watcher");
	this->setMinimumSize(600, 400);

	ui.labelDir->setText("Target Directory:");
	ui.lineEditDir->setReadOnly(true);
	ui.buttonBrowser->setText("Browse");
	ui.plainTextEdit->setReadOnly(true);
	ui.buttonStart->setText("Start");
	ui.buttonClear->setText("Clear");
	ui.buttonChange->setText("Change Directory");
	//----------------------------����������------------------------------

	// ����߼�
	connect(ui.buttonClear, &QPushButton::clicked, [=]() {
		ui.plainTextEdit->setPlainText("");
		});

	// ��ʼ������� �ı�Ŀ¼����
	connect(ui.lineEditDir, &QLineEdit::textChanged, [=](const QString& text) {
		if (!text.isEmpty()) {
			ui.buttonStart->setEnabled(true);
			ui.buttonChange->setEnabled(true);
		}
		});


	// ѡ���ļ����߼�
	connect(ui.buttonBrowser, &QPushButton::clicked, [=]() {
		QString qDir = QFileDialog::getExistingDirectory(this, 
			"Choose a Directory", "C:/", QFileDialog::ShowDirsOnly);
		if (!qDir.isEmpty()) {
			ui.lineEditDir->setText(qDir.replace('/', '\\') + '\\');
			ui.buttonBrowser->setEnabled(false);
		}
		});

	// �ı�Ŀ¼���������߼�
	connect(ui.buttonChange, &QPushButton::clicked, [=]() {
		if (QMessageBox::Ok == QMessageBox::question(this, "Change Directory to Watch",
			"Changing Directory need to Restart process\nAre you sure?",
			QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel)) {

			// ��ʧ����
			this->hide();

			// ��ȡ���ǽ��̵�ȫ·��
			TCHAR szApplication[MAX_PATH];
			DWORD cchLength = _countof(szApplication);
			QueryFullProcessImageName(GetCurrentProcess(), 0, szApplication, &cchLength);
			// ��ʼ���ṹ
			SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
			sei.lpVerb = TEXT("open"); // ������ģʽ
			sei.lpFile = szApplication;
			sei.lpParameters = NULL;
			sei.nShow = SW_SHOWNORMAL; // ��һ�������� ��Ȼû�д�����ʾ
			// �����ǵĳ���
			ShellExecuteEx(&sei);

			if (GetLastError() == S_OK) {
				// �����ɹ�
				this->close();
			}
			else
			{
				// ʧ��
				this->show();
			}

		}
		});

	// ����߼�
	connect(ui.buttonStart, &QPushButton::clicked, [=]() {
		const wchar_t* pwsTmp = reinterpret_cast<const wchar_t*>(ui.lineEditDir->text().utf16());
		std::thread t1(std::mem_fn(&DirWatcher::MonitorFile), this, (PTCHAR)pwsTmp);
		t1.detach();
		ui.buttonStart->setEnabled(false);
		});

	// ��ʼ���ܼ�� �� �ı�Ŀ¼
	ui.buttonStart->setEnabled(false);
	ui.buttonChange->setEnabled(false);

	// ���뻺����
	pbBuf = new BYTE[BUFFER_SIZE];

}

// ��غ���
BOOL DirWatcher::MonitorFile(PTCHAR pszDirectory) {

	// ��Ŀ¼, ��ȡ�ļ����
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
		// �õ� ������
		RtlZeroMemory(pbBuf, BUFFER_SIZE);
		PFILE_NOTIFY_INFORMATION pFileNotifyInfo = (PFILE_NOTIFY_INFORMATION)pbBuf;

		// ���ü��Ŀ¼
		bRet = ::ReadDirectoryChangesW(hDirectory,
			pFileNotifyInfo,
			BUFFER_SIZE,
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |			// �޸��ļ���
			FILE_NOTIFY_CHANGE_DIR_NAME |           // �޸��ļ�����
			FILE_NOTIFY_CHANGE_ATTRIBUTES |			// �޸��ļ�����
			FILE_NOTIFY_CHANGE_LAST_WRITE,			// ���һ��д��
			&dwRet,
			NULL,
			NULL);
		if (FALSE == bRet)
		{
			ui.plainTextEdit->appendPlainText("ReadDirectoryChangesW failed");
			break;
		}
		// �жϲ������Ͳ���ʾ
		switch (pFileNotifyInfo->Action)
		{
		case FILE_ACTION_ADDED:
		{
			// �����ļ�
			// ����pFileNotifyInfo���������β���ַ��� ������Ҫ��ô����
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, 
				pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(
				QString("[File Added Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_REMOVED:
		{
			// �ƶ��ļ�
			// ����pFileNotifyInfo���������β���ַ��� ������Ҫ��ô����
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Removed Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_MODIFIED:
		{
			// �޸��ļ�
			// ����pFileNotifyInfo���������β���ַ��� ������Ҫ��ô����
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Modified Action] %1").arg(QString::fromWCharArray(wsFileName)));
			break;
		}
		case FILE_ACTION_RENAMED_OLD_NAME:
		{
			// �������ļ�
			// ����pFileNotifyInfo���������β���ַ��� ������Ҫ��ô����
			RtlZeroMemory(wsFileName, sizeof(WCHAR) * MAX_PATH);
			memcpy_s(wsFileName, sizeof(WCHAR) * MAX_PATH, pFileNotifyInfo->FileName, pFileNotifyInfo->FileNameLength);
			ui.plainTextEdit->appendPlainText(QString("[File Renamed Action] OldName: %1").arg(QString::fromWCharArray(wsFileName)));

			// ��һ��FILE_NOTIFY_INFORMATION������������ļ�ʱ��������
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


	// �رվ��, �ͷ��ڴ�
	::CloseHandle(hDirectory);

	return TRUE;
}
