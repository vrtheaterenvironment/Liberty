/**
 * @file Server.c
 * Server.hで宣言された関数の定義を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "Server.h"

/**
 * サーバソケットをバインド。
 * @param serverSocket バインドするソケット。
 * @param port 待ち受けポート番号。
 */
static int
setServerPort(int serverSocket, int port)
{
    /** ソケットオプション変更用変数 */
    const int ONE = 1;
    /** サーバアドレスの長さ */
    socklen_t serverAddrLength;
    /** サーバアドレス */
    struct sockaddr_in serverAddr; 

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);
    serverAddrLength = sizeof(serverAddr);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &ONE, sizeof(int));

    /* サーバソケットをバインド */
    return bind(serverSocket, (struct sockaddr*)&serverAddr, serverAddrLength);
}

/**
 * サーバの設定を初期化。
 * @param server 初期化するサーバ。
 * @param deviceNum デバイスの数。
 * @param port サーバが使用するポート番号。
 * @return 正常に初期化できた場合は0、できなかった場合は0以外。
 */
int
initializeServer(Server *server, int devicesNum, int port)
{
    int i;
    int serverSocket;

    /* サーバソケットを作成 */
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        return -1;
    }

    /* ソケットをバインド */
    if (setServerPort(serverSocket, port)) {
        close(serverSocket);
        return -2;
    } 

    /* 接続キューを生成 */
    if (listen(serverSocket, SOMAXCONN)) {
        close(serverSocket);
        return -3;
    }

    /* クライアントソケット群を初期化 */
    server->socket = serverSocket;
    server->devicesNum = devicesNum;
    server->clients = (IntList*)malloc(sizeof(IntList) * devicesNum);
    for (i = 0; i < server->devicesNum; ++i) {
        initializeIntList(&server->clients[i]);
    }

    return 0;
}

/**
 * サーバの終了処理。
 * @param server 終了するサーバ。
 */
void
finalizeServer(Server *server)
{
    int i;
    int j;

    for (i = 0; i < server->devicesNum; ++i) {
        /* クライアントとの接続を閉鎖 */
        IntList *clients = &server->clients[i];
        pthread_mutex_lock(&clients->mutex);
        for (j = 0; j < clients->size; ++j) {
            close(clients->elements[j]);
        }
        pthread_mutex_unlock(&clients->mutex);

        /* クライアントソケット群のリソースを解放 */
        finalizeIntList(&server->clients[i]);
    }
    /* サーバソケットを閉鎖 */
    close(server->socket);
}

/**
 * クライアントからの接続を受理。
 * @param 接続を受理するサーバ。
 * @return 接続を受理した場合はそのソケット、失敗した場合は-1。
 */
int
acceptServer(Server *server)
{
    /* クライアントアドレス */
    struct sockaddr_in clientAddr; 
    /* クライアントアドレスの長さ */
    socklen_t clientAddrLength;

    clientAddrLength = sizeof(clientAddr);
    /* 接続を受理 */
    return accept(server->socket, (struct sockaddr*)&clientAddr, &clientAddrLength);
}

/**
 * 紀元（1970年1月1日00:00:00 UTC）からの経過時間の取得（ミリ秒）。
 * @return ミリ秒単位の現在時刻。
 */
static long long
getCurrentTimeMillis()
{
    struct timeval tv;
    long long mills;
    /* 秒とマイクロ秒単位で現在時刻を取得 */
    gettimeofday(&tv, NULL);
    /* ミリ秒単位に変換 */
    mills = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
    return mills;
}

/**
 * unsigned char配列の配置順を逆順に並べ替え。
 * @param dist 並べ替える配列。
 * @param size 配列の要素数。
 */
static void
reverse(unsigned char *dist, size_t size)
{
    unsigned char copiedArray[size];
    int i;
    size_t tail;

    /* 配列の中身を逆順になるようにコピー */
    tail = size - 1;
    memcpy(copiedArray, dist, sizeof(unsigned char) * size);
    for (i = 0; i < size; ++i) {
        dist[i] = copiedArray[tail - i];
    }
}

/**
 * クライアント群に指定したデータを送信。
 * @param list クライアントソケットのリスト。
 * @param data 送信データ。
 * @param size 送信データの大きさ（バイト）。
 */
static void
sendToClients(IntList *list, unsigned char *data, size_t size)
{
    int i;

    pthread_mutex_lock(&list->mutex);
    for (i = 0; i < list->size; ++i) {
        int client = list->elements[i];
        int remain = size;
        int wrote = 0;
        /* 送信データが残っている間ループ */
        while (remain > 0) {
            int result = write(client, data + wrote, remain);
            /* クライアントから切断されたらソケットを閉じてリストから削除 */
            if (result == -1) {
                removeIntList(list, client);
                close(client);
                break;
            }
            wrote += result;
            remain -= result;
        }
    }
    pthread_mutex_unlock(&list->mutex);
}

/**
 * クライアント群へデバイスプレスイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param button ボタン番号。
 */
void
sendDevicePressed(Server *server, int device, unsigned char button)
{
    /* デバイスプレスイベントを表すヘッダ */
    const unsigned char HEADER = 0;
    unsigned char data[10];
    long long time = getCurrentTimeMillis();

    /* バイトオーダを考慮しつつ配列に送信データを格納 */
    data[0] = HEADER;
    data[1] = button;
    memcpy(data + 2, &time, 8);
    reverse(data + 2, 8);

    /* クライアントへデータを送信 */
    sendToClients(&server->clients[device], data, 10);
}

/**
 * クライアント群へデバイスリリースイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param button ボタン番号。
 */
void
sendDeviceReleased(Server *server, int device, unsigned char button)
{
    /* デバイスリリースイベントを表すヘッダ */
    const unsigned char HEADER = 1;
    unsigned char data[10];
    long long time = getCurrentTimeMillis();

    /* バイトオーダを考慮しつつ配列に送信データを格納 */
    data[0] = HEADER;
    data[1] = button;
    memcpy(data + 2, &time, 8);
    reverse(data + 2, 8);

    /* クライアントへデータを送信 */
    sendToClients(&server->clients[device], data, 10);
}


/**
 * クライアント群へデバイスムーブイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param positioon デバイスの位置を格納した長さ3の配列。
 */
void
sendDeviceMoved(Server *server, int device, double position[])
{
    /* デバイスムーブイベントを表すヘッダ */
    const unsigned char HEADER = 2;
    unsigned char data[33];
    long long time = getCurrentTimeMillis();

    /* バイトオーダを考慮しつつ配列に送信データを格納 */
    data[0] = HEADER;
    memcpy(data + 1, position, 8 * 3);
    memcpy(data + 25, &time, 8);
    reverse(data + 1, 8);
    reverse(data + 9, 8);
    reverse(data + 17, 8);
    reverse(data + 25, 8);

    /* クライアントへデータを送信 */
    sendToClients(&server->clients[device], data, 33);
}

/**
 * クライアント群へデバイススウェイイベントを配信する。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param posture デバイスの姿勢を格納した長さ3の配列。
 */
void
sendDeviceSwayed(Server *server, int device, double posture[])
{
    /* デバイスウェイイベントを表すヘッダ */
    const unsigned char HEADER = 3;
    unsigned char data[33];
    long long time = getCurrentTimeMillis();

    /* バイトオーダを考慮しつつ配列に送信データを格納 */
    data[0] = HEADER;
    memcpy(data + 1, posture, 8 * 3);
    memcpy(data + 25, &time, 8);
    reverse(data + 1, 8);
    reverse(data + 9, 8);
    reverse(data + 17, 8);
    reverse(data + 25, 8);

    /* クライアントへデータを送信 */
    sendToClients(&server->clients[device], data, 33);
}
