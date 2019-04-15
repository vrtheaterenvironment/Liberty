/**
 * @file IntList.h
 * IntList構造体の定義とその操作関数の宣言を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#ifndef INT_LIST_H
#define INT_LIST_H /**< インクルードガード用定数 */

#include <pthread.h>

/** int型の値を格納するリスト */
typedef struct {
    int size;               /**< リストの要素数 */
    int capacity;           /**< リストの容量 */
    int *elements;          /**< リストの要素 */
    pthread_mutex_t mutex;  /**< ミューテックス */
} IntList;

/**
 * リストにメモリ領域を割り当て、そのリストを初期化。
 * @param list 初期化するリストのポインタ。
 */
void initializeIntList(IntList *list);

/**
 * リストのリソースを解放。
 * @param list リソースを開放するリストのポインタ。
 */
void finalizeIntList(IntList *list);

/**
 * リストの末尾に要素を追加。
 * リストの容量が足りない場合には、容量を拡大してから要素を追加する。
 * @param 要素を追加するリストのポインタ。
 * @param リストに追加する要素。
 */
void addIntList(IntList *list, int element);

/**
 * リストから要素を削除。
 * ただし、リストに削除対象の要素が複数含まれる場合、
 * リストの先頭に最も近い要素のみが削除される。
 * @param 要素を削除するリストのポインタ。
 * @param リストから削除する要素。
 */
void removeIntList(IntList *list, int element);

#endif
