/**
 * @file Server.h
 * デバイス情報の配信を行うサーバ構造体の定義と、
 * その操作関数の宣言を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#ifndef SERVER_H
#define SERVER_H /**< インクルードガード用定数  */

#include "IntList.h"

/** サーバ構造体 */
typedef struct {
  int socket;       /**< サーバソケット */
  int devicesNum;   /**< サーバで扱うデバイスの数 */
  IntList *clients; /**< クライアントソケット群 */
} Server;

/**
 * サーバの設定を初期化。
 * @param server 初期化するサーバ。
 * @param deviceNum デバイスの数。
 * @param port サーバが使用するポート番号。
 * @return 正常に初期化できた場合は0、できなかった場合は0以外。
 */
int initializeServer(Server *server, int deviceNum, int port);

/**
 * サーバのリソースの解放。
 * @param server リソースを解放するサーバ。
 */
void finalizeServer(Server *server);

/**
 * クライアントからの接続を受理。
 * @param 接続を受理するサーバ。
 * @return 接続を受理した場合はそのソケット、失敗した場合は-1。
 */
int acceptServer(Server *server);

/**
 * クライアント群へデバイスプレスイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param button ボタン番号。
 */
void sendDevicePressed(Server *server, int device, unsigned char button);

/**
 * クライアント群へデバイスリリースイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param button ボタン番号。
 */
void sendDeviceReleased(Server *server, int device, unsigned char button);

/**
 * クライアント群へデバイスムーブイベントを配信。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param positioon デバイスの位置を格納した長さ3の配列。
 */
void sendDeviceMoved(Server *server, int device, double position[]);

/**
 * クライアント群へデバイススウェイイベントを配信する。
 * @param server イベントを送信するサーバ。
 * @param device デバイス番号。
 * @param posture デバイスの姿勢を格納した長さ3の配列。
 */
void sendDeviceSwayed(Server *server, int device, double posture[]);

#endif
