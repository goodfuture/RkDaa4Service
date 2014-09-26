#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv)
{
	sqlite3 *db;
	ret = sqlite3_open_v2(test.db, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
	if (ret) {
		fprintf(stderr, "Can't open or create database \"%s\".\n", DATABASE_PATH);
		return -1; 
	} 

	/* Create RTD Table */
	sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, TimeStamp INTEGER, ChlNum TEXT, ChlCode TEXT, Value REAL);", RT_DATA_TABLE);
	ret = sqlite3_exec(dbhandle, sql, NULL, NULL, &errmsg);
	sqlite3_free(sql);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Can't create table \'%s\' : %s.\n", RT_DATA_TABLE, errmsg);
		return -1;
	}
	return 0;
}
