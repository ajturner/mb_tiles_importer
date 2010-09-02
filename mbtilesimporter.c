#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/types.h>
#include <sqlite3.h>
#include <string.h>

/**
 * MapBox tiles importer
 */

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

void fail_with_help(char **argv) {
  fprintf(stderr, "Usage: %s -f (png, jpg) [-m<key1> <value1> [-m<key2> <value2>] ... ] [-M <metadata file>] -s <tile source directory> -d <destination mbtiles file>\n", argv[0]);
  exit(1);
}

int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  int c;

  static const char *accepted_formats[] = {"png", "jpg"};
  char* format;
  char* destination;
  char* metadata;
  char* metafile;
  char* tile_source;

  while(1) {
    c = getopt (argc, argv, "f:m:M:s:d:");
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
      }
    }
  }

  if( argc != 3 ){
    fail_with_help(argv);
  }

  // TODO: check that file doesn't already exist
  rc = sqlite3_open_v2(destination, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  rc = sqlite3_exec(db, "create table tiles (zoom_level integer, tile_column integer, tile_row integer, tile_data blob)", callback, 0, &zErrMsg);
  rc = sqlite3_exec(db, "create unique index tile_index on tiles (zoom_level, tile_column, tile_row)", callback, 0, &zErrMsg);

  struct dirent *entz;
  struct dirent *entx;
  struct dirent *enty;
  DIR *dirz = opendir(tile_source);
  DIR *dirx;
  DIR *diry;
  char dirname[300];

  if (dirz) {
    while((entz = readdir(dirz)) != NULL) {
      if (strcmp(entz->d_name, ".") && strcmp(entz->d_name, "..")) { 
        strcpy(dirname, tile_source);
        strcat(dirname, "/");
        strcat(dirname, entz->d_name);
        dirx = opendir(dirname);
        while((entx = readdir(dirx)) != NULL) {
          if (strcmp(entx->d_name, ".") && strcmp(entx->d_name, "..")) { 
            dirname[] = {""};
            strcpy(dirname, tile_source);
            strcat(dirname, "/");
            strcat(dirname, entz->d_name);
            strcat(dirname, "/");
            strcat(dirname, entx->d_name);
            diry = opendir(dirname);
            /*
            while ((enty = readdir(diry)) != NULL) {
              if (strcmp(enty->d_name, ".") && strcmp(enty->d_name, "..")) { 
                printf("z[%s] completed\n", enty->d_name);
              }
            }
            */
            printf("x[%s] completed\n", entx->d_name);
          }
        }
        printf("z[%s] completed\n", entz->d_name);
      }
    }
  }
  else { fail_with_help(argv); }

  sqlite3_close(db);
  return 0;
}
