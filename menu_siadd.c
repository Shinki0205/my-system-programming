#include "RMSS.h"
#include "menu_siadd.h"

int menu_siadd(pthread_t selfId, PGconn *con, int soc, char *recvBuf, char *sendBuf)
{
  char connInfo[BUFSIZE];
  PGresult *res;
  char comm[BUFSIZE];
  char sql[BUFSIZE];
  char item_info[BUFSIZE];
  int resultRows, n, i;
  int mng_id;
  int mng_pass;
  int mng_pass_ok;
  Items item;
  char *sqlBegin = "BEGIN", *sqlCommit = "COMMIT", *sqlRollback = "ROLLBACK";
  int sendLen;

  n = sscanf(recvBuf, "%s %d %d %s %s %s %s %d %s %d %d", comm, &mng_id, &mng_pass, item.item_name, item.category, item.item_genre, item.season, &item.price, item.recipe, &item.favo_flag, &item.ensure_flag);

  if (n != 11)
  {
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_200, ENTER);
    return -1;
  }

  /* スキーマ検索パスを変更 */
  sprintf(sql, "SET search_path to online_menu");
  PQexec(con, sql);

  /* 店長IDとパスワードの入力 */
  sprintf(sql, "SELECT * FROM shop_mng_t WHERE shop_mng_id=%d", mng_id);
  res = PQexec(con, sql);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    printf("%s\n", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    return -1;
  }
  resultRows = PQntuples(res);
  if (resultRows != 1)
  {
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1201, ENTER);
    return -1;
  }
  mng_pass_ok = atoi(PQgetvalue(res, 0, 2));

  if (mng_pass_ok == mng_pass)
  {
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1202, ENTER);
    return -1;
  }

  /* 店長IDから店舗情報を入手 */
  sprintf(sql, "SELECT shop_id, region FROM shop_info_t WHERE shop_mng_id=%d", mng_id);
  res = PQexec(con, sql);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    printf("%s\n", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1201, ENTER);
    return -1;
  }
  resultRows = PQntuples(res);
  if (resultRows != 1)
  {
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_1201, ENTER);
    return -1;
  }
  item.limited_shop = atoi(PQgetvalue(res, 0, 0));

  PQexec(con, sqlBegin);

  /* 店舗オリジナルメニュー追加 */
  item.price_taxin = item.price + (item.price * TAX);
  sprintf(sql, "SELECT nextval('item_id_seq')");
  res = PQexec(con, sql);
  item.item_id = atoi(PQgetvalue(res, 0, 0));

  if (item.favo_flag == 0)
  {
    if (item.ensure_flag == 0)
    {
      sprintf(sql, "INSERT INTO items_t(item_id, item_name, category, item_genre,season, limited_shop, price, price_taxin, recipe, favo_flag, ensure_flag) VALUES(%d, '%s', '%s', '%s', '%s', %d, %d, %d, '%s', 'false', 'false')",
              item.item_id,
              item.item_name,
              item.category,
              item.item_genre,
              item.season,
              item.limited_shop,
              item.price,
              item.price_taxin,
              item.recipe);
    }
    else
    {
      sprintf(sql, "INSERT INTO items_t(item_id, item_name, category, item_genre,season, limited_shop, price, price_taxin, recipe, favo_flag, ensure_flag) VALUES(%d, '%s', '%s', '%s', '%s', %d, %d, %d, '%s', 'false', 'true')",
              item.item_id,
              item.item_name,
              item.category,
              item.item_genre,
              item.season,
              item.limited_shop,
              item.price,
              item.price_taxin,
              item.recipe);
    }
  }
  else
  {
    if (item.ensure_flag == 0)
    {
      sprintf(sql, "INSERT INTO items_t(item_id, item_name, category, item_genre,season, limited_shop, price, price_taxin,recipe, favo_flag, ensure_flag) VALUES(%d, '%s', '%s', '%s', '%s', %d, %d, %d, '%s', 'true', 'false')",
              item.item_id,
              item.item_name,
              item.category,
              item.item_genre,
              item.season,
              item.limited_shop,
              item.price,
              item.price_taxin,
              item.recipe);
    }
    else
    {
      sprintf(sql, "INSERT INTO items_t(item_id, item_name, category, item_genre, season, limited_shop, price, price_taxin, recipe, favo_flag, ensure_flag) VALUES(%d, '%s', '%s', '%s', '%s', %d, %d, %d, '%s', 'true', 'true')",
              item.item_id,
              item.item_name,
              item.category,
              item.item_genre,
              item.season,
              item.limited_shop,
              item.price,
              item.price_taxin,
              item.recipe);
    }
  }

  res = PQexec(con, sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    printf("%s", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_300, ENTER);
    PQexec(con, sqlRollback);
    return -1;
  }

  ///*** 店舗在庫に商品を登録 ***///
  if (item.limited_shop != 0)
  {
    sprintf(sql, "SELECT shop_id FROM shop_info_t WHERE shop_id = %d", item.limited_shop);
    printf("[%s]\n", sql);
    res = PQexec(con, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      printf("%s\n", PQresultErrorMessage(res));
      sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_100, ENTER);
      PQexec(con, sqlRollback);
      return -1;
    }
    resultRows = PQntuples(res);
    if (resultRows != 1)
    {
      sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_100, ENTER);
      PQexec(con, sqlRollback);
      return -1;
    }
  }

  resultRows = PQntuples(res);
  stockadd stock[resultRows];
  for (i = 0; i < resultRows; i++)
  {
    stock[i].shop_id = atoi(PQgetvalue(res, i, 0));
  }

  for (i = 0; i < resultRows; i++)
  {
    sprintf(sql, "INSERT INTO stock_t (shop_id, item_id) VALUES(%d, %d)",
            stock[i].shop_id,
            item.item_id);
    printf("[%s]\n", sql);
    res = PQexec(con, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      printf("%s", PQresultErrorMessage(res));
      sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_100, ENTER);
      PQexec(con, sqlRollback);
      return -1;
    }
  }

  // 本部在庫に商品を登録
  sprintf(sql, "INSERT INTO head_stock_t (item_id, head_item_stock) VALUES(%d, 0)",
          item.item_id);
  printf("[%s]\n", sql);
  res = PQexec(con, sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    printf("%s", PQresultErrorMessage(res));
    sprintf(sendBuf, "%s %d%s", ER_STAT, E_CODE_100, ENTER);
    PQexec(con, sqlRollback);
    return -1;
  }
  /* プロトコル・レスポンス */
  sendLen = sprintf(sendBuf, "%s %d %s %s %s %s %d %d %d %s %d %d%s", OK_STAT,
                    item.item_id,
                    item.item_name,
                    item.category,
                    item.item_genre,
                    item.season,
                    item.limited_shop,
                    item.price,
                    item.price_taxin,
                    item.recipe,
                    item.favo_flag,
                    item.ensure_flag, ENTER);

  /*プロトコル・レスポンスを送信*/
  send(soc, sendBuf, sendLen, 0);
  printf("[C_THREAD %ld] SEND=> %s", selfId, sendBuf);

  PQexec(con, sqlCommit); // トランザクション終了（コミット）

  return 0;
}
