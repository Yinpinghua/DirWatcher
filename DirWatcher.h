#pragma once

#include <QtWidgets/QWidget>
#include "ui_DirWatcher.h"
#include <QFileDialog>
#include <QMessageBox>
#include <Windows.h>
#include <strsafe.h>
#include <thread>

#define BUFFER_SIZE 2048

class DirWatcher : public QWidget
{
	Q_OBJECT

public:
	DirWatcher(QWidget *parent = Q_NULLPTR);
	// ��غ���
	BOOL MonitorFile(PTCHAR pszDirectory);
	// ���FILE_NOTIFY_INFORMATION��Ϣ�Ļ�����
	PBYTE pbBuf;
	// ������ű仯���ļ�����Ϣ
	WCHAR wsFileName[MAX_PATH];

private:
	Ui::DirWatcherClass ui;
};
