/**
 * @file Liberty.h
 * Libertyを扱う関数の宣言を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#ifndef LIBERTY_H
#define LIBERTY_H /**< インクルードガード用定数 */

#ifndef LIBERTY_SENSOR_NUM
#define LIBERTY_SENSOR_NUM 10 /**< Libertyに接続されているセンサの数 */
#endif

/**
 * Libertyの初期化。
 * @return 初期化に成功した場合は0、失敗した場合は0以外。
 */
int initializeLiberty();

/**
 * Libertyのリソースの開放。
 */
void finalizeLiberty();

/**
 * Libertyのメインループの開始。
 */
void startLibertyMainLoop();

/**
 * Libertyのメインループの停止。
 */
void stopLibertyMainLoop(void);

/**
 * Libertyのデバイスムーブイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void setLibertyMovedFunc(void (*func)(int device, double x, double y, double z));

/**
 * Libertyのデバイススウェイイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void setLibertySwayedFunc(void (*func)(int device, double x, double y, double z));

/**
 * Libertyのデバイスプレスイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void setLibertyPressedFunc(void (*func)(int device));

/**
 * Libertyのデバイスリリースイベントに対するコールバック関数の設定。
 * @param func コールバック関数。
 */
void setLibertyReleasedFunc(void (*func)(int device));

#endif
