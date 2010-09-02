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

/*
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}
*/

void fail_with_help(char **argv) {
  fprintf(stderr, "Usage: %s -f (png, jpg) [-m<key1> <value1> [-m<key2> <value2>] ... ] [-M <metadata file>] -s <tile source directory> -d <destination mbtiles file>\n", argv[0]);
  exit(1);
}

/* Convert binary data to binhex encoded data.
** The out[] buffer must be twice the number of bytes to encode.
** "len" is the size of the data in the in[] buffer to encode.
** Return the number of bytes encoded, or -1 on error.
*/
int bin2hex(char *out, const char *in, int len)
{
  int ct = len;
  if (!in || !out || len < 0) return -1;

  /* hexadecimal lookup table */
  static char hex[] = "0123456789ABCDEF";

  while (ct-- > 0)
  {
    *out   = hex[*in >> 4];
    *out++ = hex[*in++ & 0x0F];
  }

  return len;
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

  static const char *accepted_formats[] = {"png", "jpg"};
  char* format;
  char* destination;
  char* metadata;
  char* metafile;
  char* tile_source;
  char * formatpt;

  char query[400];
  char dirname[400];
  char fname[400];
  unsigned char buffer[40000];
  char encoded_buffer[40000];
  char entyname[400];

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

  // if( argc != 3 ){
  //   fail_with_help(argv);
  // }

  // TODO: check that file doesn't already exist
  rc = sqlite3_open_v2(destination, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  rc = sqlite3_exec(db, "create table tiles (zoom_level integer, tile_column integer, tile_row integer, tile_data blob)", NULL, 0, &zErrMsg);
  rc = sqlite3_exec(db, "create unique index tile_index on tiles (zoom_level, tile_column, tile_row)", NULL, 0, &zErrMsg);
  dirz = opendir(tile_source);

  if (dirz) {
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
                  printf(".");
                  strcpy(entyname, enty->d_name);
                  yvalue = strcspn(entyname, ".");
                  entyname[yvalue] = '\0';
                  sprintf(fname, "%s/%s/%s/%s", tile_source, entz->d_name, entx->d_name, enty->d_name);

                  FILE * f;
                  long int filesize = 0;
                  f = fopen(fname, "r");
                  while (!feof(f))
                  {
                      fgets(buffer,256,f);
                      filesize += 256;
                  }

                  bin2hex(encoded_buffer, buffer, filesize);
                  
                  sprintf(query, 
                      "insert into tiles (zoom_level, tile_column, tile_row, tile_data) values (%s, %s, %s, %s);", 
                      entz->d_name, 
                      entx->d_name, 
                      entyname, 
                      encoded_buffer);

                  rc = sqlite3_exec(db, query, NULL, 0, &zErrMsg);

                  if (rc) {
                    fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
                    sqlite3_close(db);
                    exit(1);
                  }

                  encoded_buffer[0] = '\0';
                }
              }
            }
          }
        }
      }
    }
  }
  else { fail_with_help(argv); }

  sqlite3_close(db);
  return 0;
}
