#ifndef _MENUSIUPDAT_
#define _MENUSIUPDAT_

//*** エラーコード***//
#define E_CODE_1301    1301  //指定された店長IDが存在しない
#define E_CODE_1302    1302  //指定された店長パスワードが適切でない
#define E_CODE_1303    1303  //店舗オリジナルメニューの制限と違います
#define E_CODE_1304    1304  //指定されたメニューIDが存在しません

#define TAX 0.1

typedef struct items_t{
  int item_id;
  char item_name[128];
  char category[6];
  char item_genre[5];
  char season[5];
  char limited_area[10];
  int limited_shop;
  int price;
  int price_taxin;
  char recipe[255];
  int favo_flag;
  int ensure_flag;
}Items;

#endif
