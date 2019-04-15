/**
 * @file Liberty.c
 * Liberty.hで宣言された関数の定義を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "Liberty.h"

#define BUFFER_LENGTH 512 /**< Libertyの受信バッファの長さ */

/** Libertyから受信するデバイスレコードを格納する構造体 */
typedef struct {
    unsigned short header;    /**< ヘッダ */
    unsigned char stationNum; /**< ステーション番号 */
    unsigned char command;    /**< コマンド名 */
    unsigned char error;      /**< エラー情報 */
    unsigned char reserved;   /**< 予約領域 */
    signed short bodySize;    /**< データ本体の大きさ */
    signed int button;        /**< ボタン押下情報 */
    float data[6];            /**< 位置とオイラー角 */
    char cr;                  /**< 改行 */
    char lf;                  /**< 復帰 */
} LibertyDeviceRecord;

/** Libertyから受信したデータを格納するバッファ */
static char buffer[BUFFER_LENGTH];
/** バッファに格納されているデータの大きさ */
static size_t dataSizeInBuffer = 0;

/** Libertyが接続されたUSBポートのハンドル */
static libusb_device_handle *handle = NULL;
/** Libertyに使用するUSBコンテキスト */
static libusb_context *context = NULL;

/** Libertyのデバイスムーブイベントに対するコールバック関数 */
static void (*deviceMovedFunc)(int device, double x, double y, double z);
/** Libertyのデバイススウェイイベントに対するコールバック関数 */
static void (*deviceSwayedFunc)(int device, double x, double y, double z);
/** Libertyのデバイスプレスイベントに対するコールバック関数 */
static void (*devicePressedFunc)(int device);
/** Libertyのデバイスリリースイベントに対するコールバック関数 */
static void (*deviceReleasedFunc)(int device);

/** 直近のボタン押下状態 */
static int recentButtonStates[LIBERTY_SENSOR_NUM];
/** メインループ終了フラグ */
static volatile int loopEnd = 0;

/**
 * Libertyのデバイスムーブイベントに対するデフォルトのコールバック関数。
 * @param device デバイス番号。
 * @param x x座標値。
 * @param y y座標値。
 * @param z z座標値。
 */
static void
doNothingDeviceMoved(int device, double x, double y, double z)
{
}

/**
 * Libertyのデバイススウェイイベントに対するデフォルトのコールバック関数。
 * @param device デバイス番号。
 * @param x x軸周りの回転量。
 * @param y y軸周りの回転量。
 * @param z z軸周りの回転量。
 */
static void
doNothingDeviceSwayed(int device, double x, double y, double z)
{
}

/**
 * Libertyのデバイスプレスイベントに対する、デフォルトのコールバック関数。
 * @param device デバイス番号。
 */
static void
doNothingDevicePressed(int device)
{
}

/**
 * Libertyのデバイスリリースイベントに対する、デフォルトのコールバック関数。
 * @param device デバイス番号。
 */
static void
doNothingDeviceReleased(int device)
{
}

/**
 * Libertyのデバイスムーブイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void
setLibertyMovedFunc(void (*func)(int device, double x, double y, double z))
{
    if (func == NULL) {
        deviceMovedFunc = doNothingDeviceMoved;
    } else {
        deviceMovedFunc = func;
    }
}

/**
 * Libertyのデバイススウェイイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void
setLibertySwayedFunc(void (*func)(int device, double x, double y, double z))
{
    if (func == NULL) {
        deviceSwayedFunc = doNothingDeviceSwayed;
    } else {
        deviceSwayedFunc = func;
    }
}

/**
 * Libertyのデバイスプレスイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void
setLibertyPressedFunc(void (*func)(int device))
{
    if (func == NULL) {
        devicePressedFunc = doNothingDevicePressed;
    } else {
        devicePressedFunc = func;
    }
}

/**
 * Libertyのデバイスリリースイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void
setLibertyReleasedFunc(void (*func)(int device))
{
    if (func == NULL) {
        deviceReleasedFunc = doNothingDeviceReleased;
    } else {
        deviceReleasedFunc = func;
    }
}

/**
 * Libertyへのデータの送信。
 * @param buf 送信するデータ。
 * @param size 送信するデータのバイト数。
 * @return 送信に成功した場合は送信したデータのバイト数、失敗した場合は負数。
 */
static int
sendData(unsigned char *buf, int size)
{
    int result;
    int actualWrite;
    /* タイムアウトまでの時間(ms) */
    const int timeout = 50;
    /* Libertyの書き込みエンドポイント */
    const int writeEp = 0x04;

    result = libusb_bulk_transfer(handle, writeEp, buf, size, &actualWrite, timeout);
    return result == 0 ? actualWrite : result;
}

/**
 * Libertyからのデータの受信。
 * @param buf 受信データの格納先。
 * @param size 受信データの許容バイト数。
 * @return 受信に成功した場合は受信したデータのバイト数、失敗した場合は負数。
 */
static int
receiveData(unsigned char *buf, int size)
{
    int result;
    int actualRead;
    /* タイムアウトまでの時間(ms) */
    const int timeout = 50;
    /* Libertyの読み込みエンドポイント */
    const int readEp = 0x88;

    result = libusb_bulk_transfer(handle, readEp, buf, size, &actualRead, timeout);
    return result == 0 ? actualRead : result;
}

/**
 * Libertyへのコマンドの送信。
 * @param command 送信するコマンド。
 * @return 送信に成功した場合は送信したコマンドのバイト数、失敗した場合は負数。
 */
static int
sendCommand(char *command)
{
    /* Libertyの処理速度を考慮して5msスリープ */
    usleep(5000);

    return sendData((unsigned char*)command, strlen(command));
}

/**
 * Libertyから反応が返ってくるまで待機。
 */
static void
waitForResponse()
{
    unsigned char buf[BUFFER_LENGTH];
    int received = 0;
    int sent = 0;
    /* 送受信に成功するまでループ */
    do { 
        sent = sendCommand("\r");
        received = receiveData(buf, BUFFER_LENGTH);
        usleep(1000000);
    } while (received < 0 || sent < 0);
}

/**
 * Libertyのデバイスレコードの検証。
 * @param record 検証するデバイスレコード。
 * @return 正常なデータの場合は0、不正なデータの場合は0以外。
 */
static int
validate(LibertyDeviceRecord* record)
{
    return record->header != 0x594c || 
        record->stationNum <= 0 || record->stationNum > LIBERTY_SENSOR_NUM ||
        record->command != 'P' ||
        (record->button != 1 && record->button != 0) ||
        record->cr != 0x0d ||
        record->lf != 0x0a;
}

/**
 * Libertyからデータを受信し、バッファに追加。
 */
static void
appendBuffer(void)
{
    /* バッファの末尾 */
    char* tail = buffer + dataSizeInBuffer;
    /* バッファの空き容量 */
    size_t remain = BUFFER_LENGTH - dataSizeInBuffer;
    int received = receiveData((unsigned char*)tail, remain);
    if (received > 0) {
        /* 受信に成功したらバッファ内のデータの大きさを更新 */
        dataSizeInBuffer += received;
    }
}

/**
 * Libertyのメインループの開始。
 */
void
startLibertyMainLoop(void)
{
    /* Libertyのデバイスレコード1件分のバイト数 */
    const int recordSize = 38;
    /* メインループ終了フラグを解除 */
    loopEnd = 0;
    while (!loopEnd) {
        /* バッファ内にデバイスレコード1件分のデータが存在しているかどうかで分岐 */
        if (dataSizeInBuffer < recordSize) {
            /* 存在しない場合、データを要請 */
            sendCommand("P");
            appendBuffer();
        } else {
            /* 存在する場合、バッファからデータを取得して解析 */
            LibertyDeviceRecord record;
            memcpy(&record, buffer, recordSize);
            if (validate(&record)) {
                /* バッファの先頭1バイトを削除 */
                --dataSizeInBuffer;
                memmove(buffer, buffer + 1, dataSizeInBuffer);
            } else {
                /* デバイス番号を0番から開始するように調整 */
                int device = record.stationNum - 1;

                /* ボタン状態の更新を確認 */
                if (recentButtonStates[device] != record.button) {
                    if (record.button) {
                        /* デバイスプレスイベントを配信 */
		        (*devicePressedFunc)(device);
                    } else {
                        /* デバイスリリースイベントを配信 */
                        (*deviceReleasedFunc)(device);
                    }
                }
                /* ボタン状態の更新 */
                recentButtonStates[device] = record.button;

                /* デバイスムーブイベント、デバイススウェイイベントを配信 */
                (*deviceMovedFunc)(device, record.data[0], record.data[1], record.data[2]);
                (*deviceSwayedFunc)(device, record.data[3], record.data[4], record.data[5]);

                /* 取得したデータをバッファから削除 */
                dataSizeInBuffer -= recordSize;
                memmove(buffer, buffer + recordSize, dataSizeInBuffer);
            }
        }
    }
}

/**
 * Libertyのメインループの停止。
 */
void
stopLibertyMainLoop(void)
{
    loopEnd = 1;
}

/**
 * Libertyへの初期化コマンドの送信。
 */
static void
sendInitializeCommands()
{
    char setOutputUnitAsCm[] = "U1\r";
    char setOutputDataAsFloatBinary[] = "F1\r";
    char setOutputFormats[] = "O*,10,2,4,1\r";
    char setStylusMouseMode[] = "L1,0\r";
    char setHemispheres[] = "H*,0,0,-1\r";
    char resetReferenceFrames[] = {'\022', '*', '\r', '\0'};
    char setReferenceFrame1[] = 
        "a1,-30.00,0.00,-6.00,-30.00,1.00,-6.00,-30.00,0.00,-7.00\r";
    char setReferenceFrame2[] = 
        "a2,-30.00,0.00,-6.00,-30.00,1.00,-6.00,-30.00,0.00,-7.00\r";
    char setReferenceFrame3[] = 
        "a3,-9.27,-28.53,-6.00,-10.22,-28.22,-6.00,-9.27,-28.53,-7.00\r";
    char setReferenceFrame4[] = 
        "a4,-9.27,-28.53,-6.00,-10.22,-28.22,-6.00,-9.27,-28.53,-7.00\r";
    char setReferenceFrame5[] = 
        "a5,24.27,-17.63,-6.00,23.68,-18.44,-6.00,24.27,-17.63,-7.00\r";
    char setReferenceFrame6[] = 
        "a6,24.27,-17.63,-6.00,23.68,-18.44,-6.00,24.27,-17.63,-7.00\r";
    char setReferenceFrame7[] = 
        "a7,24.27,17.63,-6.00,24.86,16.82,-6.00,24.27,17.63,-7.00\r";
    char setReferenceFrame8[] = 
        "a8,24.27,17.63,-6.00,24.86,16.82,-6.00,24.27,17.63,-7.00\r";
    char setReferenceFrame9[] = 
        "a9,-9.27,28.53,-6.00,-8.32,28.84,-6.00,-9.27,28.53,-7.00\r";
    char setReferenceFrame10[] = 
        "a10,-9.27,28.53,-6.00,-8.32,28.84,-6.00,-9.27,28.53,-7.00\r";
    char setReceiverRotation[] = "G0,0,0\r";
    char setDisableContinuousPrinting[] = "P";
    printf("### begin initialize\n");
    sendCommand(resetReferenceFrames);
    sendCommand(setReferenceFrame1);
    sendCommand(setReferenceFrame2);
    sendCommand(setReferenceFrame3);
    sendCommand(setReferenceFrame4);
    sendCommand(setReferenceFrame5);
    sendCommand(setReferenceFrame6);
    sendCommand(setReferenceFrame7);
    sendCommand(setReferenceFrame8);
    sendCommand(setReferenceFrame9);
    sendCommand(setReferenceFrame10);
    sendCommand(setHemispheres);
    sendCommand(setReceiverRotation);
    sendCommand(setOutputUnitAsCm);
    sendCommand(setOutputFormats);
    sendCommand(setStylusMouseMode);
    sendCommand(setOutputDataAsFloatBinary);
    sendCommand(setDisableContinuousPrinting);
    printf("### finished initialize\n");
}

/**
 * Libertyの初期化。
 * @return 初期化に成功した場合は0、失敗した場合は0以外。
 */
int
initializeLiberty()
{
    int result;
    /* LibertyのベンダID */
    const int vid = 0x0f44;
    /* LibertyのプロダクトID */
    const int pid = 0xff20;

    /* コールバック関数を初期化 */
    deviceMovedFunc = doNothingDeviceMoved;
    deviceSwayedFunc = doNothingDeviceSwayed;
    devicePressedFunc = doNothingDevicePressed;
    deviceReleasedFunc = doNothingDeviceReleased;

    /* libusbライブラリを初期化 */
    result = libusb_init(&context);
    if (result) {
        fprintf(stderr, "libusb initialize error.\n");
        context = NULL;
        return -1;
    }

    /* Libertyのハンドラを生成 */
    handle = libusb_open_device_with_vid_pid(context, vid, pid);
    if (!handle) {
        fprintf(stderr, "cannot open device (vid: %04x, pid: %04x).\n", vid, pid);
        libusb_exit(context);
        return -2;
    }

    /* Libertyから応答があるまで待機 */
    puts("### wait for a responce from liberty...");
    waitForResponse();
    puts("### get a response from liberty.");

    /* Libertyへ初期化コマンドを送信 */
    sendInitializeCommands();

    return 0;
}

/**
 * Libertyのリソースの開放。
 */
void
finalizeLiberty()
{
    libusb_close(handle);
    libusb_exit(context);
}
