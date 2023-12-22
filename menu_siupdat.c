#include "RMSS.h"
#include "menu_siupdat.h"

int menu_siupdat(pthread_t selfId, PGconn *con, int soc, char *recvBuf, char *sendBuf){
  PGresult *res;
  char connInfo[BUFSIZE];
  char sql[BUFSIZE];
  char comm[BUFSIZE];
  char item_info[BUFSIZE];
  int resultRows, n;
  int mng_id;
  int mng_pass;
  int mng_pass_ok;
  char lim_area[10];
  int lim_shop;
  Items item;
  char *sqlBegin="BEGIN", *sqlCommit="COMMIT", *sqlRollback="ROLLBACK";
  int sendLen;

  n = sscanf(recvBuf, "%s %d %d %d %s %s %s %s %s %d %d %s %d %d", comm, &mng_id, &mng_pass, &item.item_id, item.item_name, item.category, item.item_genre, item.season, lim_area, &lim_shop, &item.price, item.recipe, &item.favo_flag, &item.ensure_flag);

  if(n != 14){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_200, ENTER);
    return -1;
  }

  /* スキーマ検索パスを変更 */
  sprintf(sql, "SET search_path to online_menu");
  PQexec(con, sql);

  /* 店長IDとパスワードの入力 */
  sprintf(sql, "SELECT * FROM shop_mng_t WHERE shop_mng_id=%d", mng_id);
  res = PQexec(con, sql);
  if(PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("%s\n", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    return -1;
  }
  resultRows = PQntuples(res);
  if(resultRows != 1){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1301, ENTER);
    return -1;
  }
  mng_pass_ok = atoi(PQgetvalue(res, 0, 2));

  if(mng_pass_ok == mng_pass){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1302, ENTER);
    return -1;
  }

  /* 店長IDから店舗情報を入手 */
  sprintf(sql, "SELECT shop_id, region FROM shop_info_t WHERE shop_mng_id=%d", mng_id);
  printf("%s\n", sql);
  res = PQexec(con, sql);
  if(PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("%s\n", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    return -1;
  }
  resultRows = PQntuples(res);
  if(resultRows != 1){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1301, ENTER);
    return -1;
  }
  strcpy(item.limited_area, PQgetvalue(res, 0, 1));
  item.limited_shop = atoi(PQgetvalue(res, 0, 0));

  if(strcmp(lim_area, item.limited_area) != 0){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1303, ENTER);
    return -1;
  }else if(lim_shop != item.limited_shop){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1303, ENTER);
    return -1;
  }
    
  sprintf(sql, "SELECT * FROM items_t WHERE item_id=%d", item.item_id);
  res = PQexec(con, sql);
  if(PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("%s\n", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    return -1;
  }
  resultRows = PQntuples(res);
  if(resultRows != 1){
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1301, ENTER);
    return -1;
  }   
    
  PQexec(con, sqlBegin);//トランザクション開始

  /* 店舗オリジナルメニュー編集 */
  item.price_taxin = item.price+(item.price*TAX);

  if(item.favo_flag == 0){
    if(item.ensure_flag == 0){
      sprintf(sql, "UPDATE items_t SET item_name='%s', category='%s', item_genre='%s', season='%s', price=%d, price_taxin=%d, recipe='%s', favo_flag='false', ensure_flag='false' WHERE item_id='%d'",
	      item.item_name,
	      item.category,
	      item.item_genre,
	      item.season,
	      item.price,
	      item.price_taxin,
	      item.recipe,
	      item.item_id);
    }else{
      sprintf(sql, "UPDATE items_t SET item_name='%s', category='%s', item_genre='%s', season='%s', price=%d, price_taxin=%d, recipe='%s', favo_flag='false', ensure_flag='true' WHERE item_id='%d'",
	      item.item_name,
	      item.category,
	      item.item_genre,
	      item.season,
	      item.price,
	      item.price_taxin,
	      item.recipe,
	      item.item_id);
    }
  }else{
    if(item.ensure_flag == 0){
      sprintf(sql, "UPDATE items_t SET item_name='%s', category='%s', item_genre='%s', season='%s', price=%d, price_taxin=%d, recipe='%s', favo_flag='true', ensure_flag='false' WHERE item_id='%d'",
	      item.item_name,
	      item.category,
	      item.item_genre,
	      item.season,
	      item.price,
	      item.price_taxin,
	      item.recipe,
	      item.item_id);
    }else{
      sprintf(sql, "UPDATE items_t SET item_name='%s', category='%s', item_genre='%s', season='%s', price=%d, price_taxin=%d, recipe='%s', favo_flag='true', ensure_flag='true' WHERE item_id='%d'",
	      item.item_name,
	      item.category,
	      item.item_genre,
	      item.season,
	      item.price,
	      item.price_taxin,
	      item.recipe,
	      item.item_id);
    }
  }
  
  res = PQexec(con, sql);
  if(PQresultStatus(res) != PGRES_COMMAND_OK){
    printf("%s", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    PQexec(con, sqlRollback);
    return -1;
  }

  /* プロトコル・レスポンス */
  sendLen = sprintf(sendBuf, "%s %d %s %s %s %s %s %d %d %d %s %d %d%s", OK_STAT,
	  item.item_id,
	  item.item_name,
	  item.category,
	  item.item_genre,
	  item.season,
	  item.limited_area,
	  item.limited_shop,
	  item.price,
	  item.price_taxin,
          item.recipe,
          item.favo_flag,
          item.ensure_flag, ENTER);

  /*プロトコル・レスポンスを送信*/
  send(soc, sendBuf, sendLen, 0);
  printf("[C_THREAD %ld] SEND=> %s", selfId, sendBuf);

  PQexec(con, sqlCommit);  //トランザクション終了（コミット）

  return 0;
}
