#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "sqlite3.h"
#include "db.h"
#include "game.h"
#include "textures.h"
#include "debugnet.h"

namespace DB {
    bool TableExists(sqlite3 *database, char* table_name)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        bool found = false;
        std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, table_name, strlen(table_name), NULL);

            int step = sqlite3_step(res);
            sqlite3_finalize(res);
            if (step == SQLITE_ROW) {
                found = true;
            }
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
        return found;
    }

    void GetVitaDbGames(GameCategory *category)
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open(VITA_APP_DB_FILE, &db);
        std::string sql = "select titleId,val from tbl_appinfo where key=572932585 and titleID not like 'NPXS%'";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            std::string title = std::string((const char*)sqlite3_column_text(res, 1));
            std::replace( title.begin(), title.end(), '\n', ' ');
            sprintf(game.title, "%s", title.c_str());
            sprintf(game.category, "%s", GAME::GetGameCategory(game.id));
            sprintf(game.rom_path, "%s", "");
            game.tex = no_icon;
            game.type = TYPE_BUBBLE;
            category->games.push_back(game);
            game_scan_inprogress = game;
            games_scanned++;
            step = sqlite3_step(res); 
        }
        sqlite3_finalize(res);
        sqlite3_close(db);
    }

    int GetVitaDbGamesCount()
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open(VITA_APP_DB_FILE, &db);
        std::string sql = "select count(distinct(titleId)) from tbl_appinfo where key=572932585 and titleID not like 'NPXS%'";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        int count = 0;
        if (step == SQLITE_ROW)
        {
            count = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);
        sqlite3_close(db);

        return count;
    }

    void SetupDatabase(sqlite3 *database)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        if (!TableExists(db, GAMES_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + GAMES_TABLE + "(" +
                COL_TITLE + " TEXT," +
                COL_TYPE + " INTEGER," +
                COL_CATEGORY + " TEXT," +
                COL_ROM_PATH + " TEXT NOT NULL PRIMARY KEY)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, FAVORITES_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + FAVORITES_TABLE + "(" +
                COL_TITLE_ID + " TEXT," +
                COL_TITLE + " TEXT," +
                COL_TYPE + " INTEGER," +
                COL_CATEGORY + " TEXT," +
                COL_ROM_PATH + " TEXT)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void InsertFavorite(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("INSERT INTO ") + FAVORITES_TABLE + "(" + COL_TITLE_ID + "," +
            COL_TITLE + "," + COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + ") VALUES (?, ?, ?, ?, ?)";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->id, strlen(game->id), NULL);
            sqlite3_bind_text(res, 2, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 3, game->type);
            sqlite3_bind_text(res, 4, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 5, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteFavorite(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + FAVORITES_TABLE + " WHERE " + COL_TITLE + "=? AND " +
            COL_TYPE + "=? AND " + COL_CATEGORY + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 2, game->type);
            sqlite3_bind_text(res, 3, game->category, strlen(game->category), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void GetFavorites(sqlite3 *database, GameCategory *category)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_TITLE_ID + "," + COL_TITLE + "," +
            COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + " FROM " + FAVORITES_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            sprintf(game.title, "%s", sqlite3_column_text(res, 1));
            game.type = sqlite3_column_int(res, 2);
            sprintf(game.category, "%s", sqlite3_column_text(res, 3));
            sprintf(game.rom_path, "%s", sqlite3_column_text(res, 4));
            game.tex = no_icon;
            games_scanned++;
            game_scan_inprogress = game;
            category->games.push_back(game);
            step = sqlite3_step(res);
        }
        sqlite3_finalize(res);
        
        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void InsertGame(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("INSERT INTO ") + GAMES_TABLE + "(" + COL_TITLE +
            "," + COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + ") VALUES (?, ?, ?, ?)";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 2, game->type);
            sqlite3_bind_text(res, 3, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 4, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   bool GameExists(sqlite3 *database, const char* rom_path)
   {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_TITLE + "," +
            COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH +
            " FROM " + GAMES_TABLE + " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        bool found = false;
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                found = true;
            }
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return found;
    }

    int GetCachedGamesCount(sqlite3 *database)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        int rc = sqlite3_open(VITA_APP_DB_FILE, &db);
        std::string sql = std::string("select count(1) from ") + GAMES_TABLE;
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        int count = 0;
        if (step == SQLITE_ROW)
        {
            count = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);
        sqlite3_close(db);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return count;
    }

   void GetCachedGames(sqlite3 *database, GameCategory *category)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_TITLE + "," +
            COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + " FROM " + GAMES_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", " ");
            sprintf(game.title, "%s", sqlite3_column_text(res, 0));
            game.type = sqlite3_column_int(res, 1);
            sprintf(game.category, "%s", sqlite3_column_text(res, 2));
            sprintf(game.rom_path, "%s", sqlite3_column_text(res, 3));
            game.tex = no_icon;
            games_scanned++;
            game_scan_inprogress = game;
            category->games.push_back(game);
            step = sqlite3_step(res);
        }
        sqlite3_finalize(res);
        
        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void DeleteGame(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + GAMES_TABLE + " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }
}