/**
 * @file IntList.c
 * IntList.hで宣言された操作関数の定義を記述したファイル。
 *
 * Oct. 2010 by Muroran Institute of Technology
 */
#include <stdlib.h>
#include <string.h>
#include "IntList.h"

/**
 * リストにメモリ領域を割り当て、そのリストを初期化。
 * @param list 初期化するリストのポインタ。
 */
void
initializeIntList(IntList *list)
{
    const int INIT_CAPACITY = 16;
    int *elements;

    /* ミューテックスを初期化してロック */
    pthread_mutex_init(&list->mutex, NULL);
    pthread_mutex_lock(&list->mutex);

    /* 要素のメモリを割り当て */
    elements = (int*)malloc(sizeof(int) * INIT_CAPACITY);
    list->size = 0;
    list->capacity = INIT_CAPACITY;
    list->elements = elements;

    /* ロックを解除 */
    pthread_mutex_unlock(&list->mutex);
}

/**
 * リストのメモリ領域を解放。
 * @param list メモリ領域を開放するリストのポインタ。
 */
void
finalizeIntList(IntList *list)
{
    /* ミューテックスを破壊 */
    pthread_mutex_destroy(&list->mutex);
    /* 要素のメモリを解放 */
    free(list->elements);
}

/**
 * リストの末尾に要素を追加。
 * リストの容量が足りない場合には、容量を拡大してから要素を追加する。
 * @param 要素を追加するリストのポインタ。
 * @param リストに追加する要素。
 */
void
addIntList(IntList *list, int element)
{
    /* 容量が足りない場合は再割り当て */
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->elements = (int*)realloc(list->elements, sizeof(int) * list->capacity);
    }

    /* 要素を追加 */
    list->elements[list->size] = element;
    ++list->size;
}

/**
 * リストから要素を削除。
 * ただし、リストに削除対象の要素が複数含まれる場合、
 * リストの先頭に最も近い要素のみが削除される。
 * @param 要素を削除するリストのポインタ。
 * @param リストから削除する要素。
 */
void
removeIntList(IntList *list, int element)
{
    int i;
    /* 要素を探して削除 */
    for (i = 0; i < list->size; ++i) {
        if (list->elements[i] == element) {
            int* remove = list->elements + i;
            int remain = list->size - i - 1;
            memmove(remove, remove + 1, sizeof(int) * remain);
            --list->size;
            break;
        }
    }
}
