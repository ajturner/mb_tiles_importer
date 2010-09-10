build:
	gcc -o mb_tiles_importer mbtilesimporter.c sqlite3.c encode.c -lpthread -ldl
