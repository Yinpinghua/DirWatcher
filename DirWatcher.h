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
	// 监控函数
	BOOL MonitorFile(PTCHAR pszDirectory);
	// 存放FILE_NOTIFY_INFORMATION信息的缓冲区
	PBYTE pbBuf;
	// 用来存放变化的文件名信息
	WCHAR wsFileName[MAX_PATH];

private:
	Ui::DirWatcherClass ui;
};
