#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/types.h>
#include <sqlite3.h>
#include <string.h>

/**
 * MapBox tiles importer
 * (c) Development Seed 2010
 * BSD licensed: http://creativecommons.org/licenses/BSD/
 */

int fail_with_help(char **argv) {
  fprintf(
      stderr, 
      "Usage: %s -f (png, jpg) [-m<key1> <value1> [-m<key2> <value2>] ... ] -v (verbose) [-M <metadata file>] -s <tile source directory> -d <destination mbtiles file>\n", 
      argv[0]);
  return 1;
}

int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  int c;
  int yvalue;
  
  struct dirent *entz;
  struct dirent *entx;
  struct dirent *enty;
  DIR *dirz;
  DIR *dirx;
  DIR *diry;
  int z;
  int x;
  int y;
  int i = 0;

  static const char *accepted_formats[] = {"png", "jpg"};
  char* format;
  char* destination;
  char* metadata;
  char* metafile;
  char* formatpt;

  FILE * fp;
  char* tile_source;
  char query[8000];
  char dirname[400];
  char fname[400];
  unsigned char buffer[10000];
  char entyname[400];
  int verbose = 0;
  char *mquery = "insert into tiles (zoom_level, tile_column, tile_row, tile_data) values (?1, ?2, ?3, ?4);";
  static sqlite3_stmt *pStmtInsertBlob = NULL;

  while(1) {
    c = getopt (argc, argv, "f:m:M:s:d:v");
    if (c == -1) {
      break;
    }
    else {
      switch (c) {
        case 'f':
          format = optarg; // TODO: validate
          break;
        case 'd':
          destination = optarg; // TODO: validate
          break;
        case 'm':
          metadata = optarg; // TODO: validate
          break;
        case 'M':
          metafile = optarg; // TODO: validate
          break;
        case 's':
          tile_source = optarg; // TODO: validate
          break;
        case 'v':
          verbose = 1;
          break;
      }
    }
  }

  if (format == NULL) {
    printf("Format required.");
    fail_with_help(argv);
    return 1;
  }

  // TODO: check that file doesn't already exist
  rc = sqlite3_open_v2(destination, 
      &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  rc = sqlite3_exec(db, 
      "create table tiles (zoom_level integer, tile_column integer, tile_row integer, tile_data blob)", 
      NULL, 0, &zErrMsg);

  rc = sqlite3_exec(db, 
      "create table metadata (name text, value text)", 
      NULL, 0, &zErrMsg);

  rc = sqlite3_exec(db, 
      "create unique index name on metadata (name)", 
      NULL, 0, &zErrMsg);

  rc = sqlite3_exec(db, 
      "create unique index tile_index on tiles (zoom_level, tile_column, tile_row)", 
      NULL, 0, &zErrMsg);
  dirz = opendir(tile_source);

  if (dirz != NULL) {
    while((entz = readdir(dirz)) != NULL) {
      if (strcmp(entz->d_name, ".") && strcmp(entz->d_name, "..")) { 
        sprintf(dirname, "%s/%s", tile_source, entz->d_name);
        dirx = opendir(dirname);
        while((entx = readdir(dirx)) != NULL) {
          if (strcmp(entx->d_name, ".") && strcmp(entx->d_name, "..")) { 
            sprintf(dirname, "%s/%s/%s", tile_source, entz->d_name, entx->d_name);
            diry = opendir(dirname);
            while ((enty = readdir(diry)) != NULL) {
              if (strcmp(enty->d_name, ".") && strcmp(enty->d_name, "..")) { 
                if (strstr(enty->d_name, format) != NULL) {
                  strcpy(entyname, enty->d_name);
                  yvalue = strcspn(entyname, ".");
                  entyname[yvalue] = '\0';

                  sprintf(fname, 
                      "%s/%s/%s/%s", 
                      tile_source, 
                      entz->d_name, entx->d_name, enty->d_name);

                  if (verbose == 1) {
                    printf("%s\n", fname);
                  }
                  else {
                    printf(".");
                  }

                  fp = fopen(fname, "r");
                  for (i = 0; i < 10000 && (rc = getc(fp)) != EOF; buffer[i++] = rc);
                  fclose(fp);

                  if(sqlite3_prepare_v2(db, mquery, -1, &pStmtInsertBlob, 0) != SQLITE_OK)
	                {
	                  printf("Could not prepare INSERT blob statement.\n");
	                  return 1;
	                }
                  
                  z = atoi(entz->d_name);
                  x = atoi(entx->d_name);
                  y = atoi(entyname);

                  rc = sqlite3_exec(db, mquery, NULL, 0, &zErrMsg);

                  if ((rc = sqlite3_bind_blob(pStmtInsertBlob, 
                          4, buffer, i, SQLITE_STATIC)) != SQLITE_OK) {
                    printf("failed");
                  }
                  if ((rc = sqlite3_bind_int(pStmtInsertBlob, 1, z)) != SQLITE_OK) {
                    printf("failed");
                  }
                  if ((rc = sqlite3_bind_int(pStmtInsertBlob, 2, x)) != SQLITE_OK) {
                    printf("failed");
                  }
                  if ((rc = sqlite3_bind_int(pStmtInsertBlob, 3, y)) != SQLITE_OK) {
                    printf("failed");
                  }

                  if((rc = sqlite3_step(pStmtInsertBlob)) != SQLITE_DONE)
                    printf("error\n");
                  else
                    sqlite3_reset(pStmtInsertBlob);
                }
              }
            }
          }
        }
      }
    }
  }
  else { return fail_with_help(argv); }
  printf("finished importing tiles\n");

  sqlite3_close(db);
  return 0;
}
