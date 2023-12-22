#ifndef _MENUSIADD_
#define _MENUSIADD_

//*** エラーコード***//
#define E_CODE_1201  1201  //指定された店長IDが存在しない
#define E_CODE_1202  1202  //指定された店長パスワードが適切でない

#define TAX 0.1

typedef struct items_t{
  int item_id;
  char item_name[128];
  char category[6];
  char item_genre[5];
  char season[5];
  int limited_shop;
  int price;
  int price_taxin;
  char recipe[255];
  int favo_flag;
  int ensure_flag;
}Items;

typedef struct _Stockadd
{
  int shop_id;
} stockadd;

#endif
