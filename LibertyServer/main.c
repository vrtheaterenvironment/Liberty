/**
 * @file main.c
 * メイン処理を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include "Server.h"
#include "Liberty.h"

/** サーバ */
static Server server;

/**
 * Libertyのデバイスムーブイベントに対するコールバック関数。
 * @param device デバイス番号。
 * @param x x座標値。
 * @param y y座標値。
 * @param z z座標値。
 */
static void
moveDevice(int device, double x, double y, double z)
{
    double position[] = {x, y, z};
    sendDeviceMoved(&server, device, position);
}

/**
 * Libertyのデバイススウェイイベントに対するコールバック関数。
 * @param device デバイス番号。
 * @param x x軸周りの回転量。
 * @param y y軸周りの回転量。
 * @param z z軸周りの回転量。
 */
static void
swayDevice(int device, double x, double y, double z)
{
    double posture[] = {x, y, z};
    sendDeviceSwayed(&server, device, posture);
}

/**
 * Libertyのデバイスプレスイベントのコールバック関数。
 * @param device デバイス番号。
 */
static void
pressDevice(int device)
{
    sendDevicePressed(&server, device, (unsigned char)0);
}

/**
 * Libertyのデバイスリリースイベントのコールバック関数。
 * @param device デバイス番号。
 */
static void
releaseDevice(int device)
{
    sendDeviceReleased(&server, device, (unsigned char)0);
}

/**
 * Libertyのメインループを実行。
 * @param arg 使用しない。
 * @return arg。
 */
void*
doLibertyMainLoop(void* arg)
{
    /* Libertyのメインループを開始 */
    startLibertyMainLoop();
    /* メインループが終了したらリソースを解放 */
    finalizeLiberty();

    return arg;
}

/**
 * 入力された2つの数値のうち、大きい方の値の取得。
 * @param a 数A。
 * @param b 数B。
 * @return 数Aと数Bのうち、大きい方の値。
 */
static int
getMax(int a, int b)
{
    return a > b ? a : b;
}

/**
 * ファイルディスクリプタ集合をselect()の呼び出しが可能な状態に設定。
 * @param fdSet 設定するファイルディスクリプタ集合。
 * @param server 監視するサーバ。
 * @param waitSet 監視する待ちリスト。
 * @return 監視対象のファイルディスクリプタの値のうち最大の値+1。
 */
static int
initializeFdSet(fd_set *fdSet, Server *server, IntList *waitSet)
{
    int i;
    int maxFdNum = 0;

    /* ファイルディスクリプタ集合を初期化 */
    FD_ZERO(fdSet);

    /* コンソールを監視するように設定 */
    FD_SET(STDIN_FILENO, fdSet);
    maxFdNum = getMax(maxFdNum, STDIN_FILENO);

    /* サーバソケットを監視するように設定 */
    FD_SET(server->socket, fdSet);
    maxFdNum = getMax(maxFdNum, server->socket);

    /* 待ちリストを監視するように設定 */
    for (i = 0; i < waitSet->size; ++i) {
        FD_SET(waitSet->elements[i], fdSet);
        maxFdNum = getMax(maxFdNum, waitSet->elements[i]);
    }

    return maxFdNum + 1;
}

/**
 * メイン関数。
 * @argc 引数の数。
 * @argv コマンドライン引数。
 */
int 
main(int argc, char *argv[])
{
    /* デバイスへの関連付けが完了していないクライアントのリスト */
    IntList waitSet;
    /* サーバのポート番号 */
    const int serverPort = 11113;
    /* サーバのデバイス数 */
    const int devicesNum = LIBERTY_SENSOR_NUM;
    /* Libertyのメインループを実行するスレッド */
    pthread_t libertyThread;

    /* SIGPIPE検出時に何もしないように設定 */
    signal(SIGPIPE, SIG_IGN);

    /* サーバを初期化 */
    if (initializeServer(&server, devicesNum, serverPort)) {
        printf("server initialize error\n");
        return EXIT_FAILURE;
    }
    /* 待ちリストを初期化 */
    initializeIntList(&waitSet);

    /* Libertyを初期化 */
    if (initializeLiberty()) {
        printf("liberty initialize error\n");
        finalizeLiberty();
        return EXIT_FAILURE;
    }

    /* Libertyのコールバック関数を登録 */
    setLibertyMovedFunc(moveDevice);
    setLibertySwayedFunc(swayDevice);
    setLibertyPressedFunc(pressDevice);
    setLibertyReleasedFunc(releaseDevice);

    /* Libertyのメインループスレッドを開始 */
    if (pthread_create(&libertyThread, NULL, doLibertyMainLoop, NULL)) {
        printf("thread creation error\n");
        finalizeLiberty();
        finalizeServer(&server);
        finalizeIntList(&waitSet);
        return EXIT_FAILURE;
    }

    /* サーバのループ */
    puts("pressed [Enter], exit server.");
    while (1) {
        fd_set fdSet;
        int i;
        int result;

        /* ファイルディスクリプタ集合の初期化 */
        result = initializeFdSet(&fdSet, &server, &waitSet);

        /* タイムアウトを設定 */
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        /* 入出力の選択 */
        if (select(result, &fdSet, NULL, NULL, &timeout) < 0) {
            perror("select()");
        }

        /* 標準入力から読み込み可能ならループ終了 */
        if (FD_ISSET(STDIN_FILENO, &fdSet)) {
            char buffer[256];
            if ((result = read(STDIN_FILENO, buffer, 256)) > 0) {
                stopLibertyMainLoop();
                break;
            }
        } 

        /* 待ちリストから受信 */
        for (i = 0; i < waitSet.size; ++i) {
            int socket = waitSet.elements[i];
            if (FD_ISSET(socket, &fdSet)) {
                /* 対象デバイス番号を受信 */
                unsigned char deviceId;
                int result = read(socket, &deviceId, 1);
                switch (result) {
                case 1:
                    if (deviceId < server.devicesNum) {
                        /* サーバのリストにクライアントを追加 */
                        IntList *clients = &server.clients[deviceId];
                        pthread_mutex_lock(&clients->mutex);
                        addIntList(clients, socket);
                        pthread_mutex_unlock(&clients->mutex);
                        /* 待ちリストからクライアントを削除 */
                        removeIntList(&waitSet, socket);
                        --i;
                    }
                    break;
                case -1: 
                    /* 待ちリストからクライアントを削除 */
                    removeIntList(&waitSet, socket);
                    --i;
                    break;
                }
            }
        }

        /* 接続を受理 */
        if (FD_ISSET(server.socket, &fdSet)) {
            result = acceptServer(&server);
            if (result != -1) {
                addIntList(&waitSet, result);
            }
        }
    }

    /* リソースの解放 */
    pthread_join(libertyThread, NULL);
    finalizeServer(&server);
    finalizeIntList(&waitSet);

    return EXIT_SUCCESS;
}
