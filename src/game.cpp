#include <cstdint>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <vitasdk.h>

#include "game.h"
#include "sfo.h"
#include "fs.h"
#include "windows.h"
#include "textures.h"

#define GAME_LIST_FILE "ux0:data/SMLA00001/games_list.txt"

std::vector<Game> games;
int page_num = 1;
int max_page;

namespace GAME {

    void Init() {
    }

    void Scan() {
        if (!FS::FileExists(GAME_LIST_FILE))
        {
            FS::MkDirs("ux0:data/SMLA00001");
            void* fd = FS::Create(GAME_LIST_FILE);

            std::vector<std::string> dirs = FS::ListDir("ux0:app/");
            for(std::size_t i = 0; i < dirs.size(); ++i) {
                char sfo_file[256];
                sprintf(sfo_file, "ux0:app/%s/sce_sys/param.sfo", dirs[i].c_str());
                if (FS::FileExists(sfo_file)) {
                    const auto sfo = FS::Load(sfo_file);

                    Game game;
                    std::string title = std::string(SFO::GetString(sfo.data(), sfo.size(), "TITLE"));
                    std::replace( title.begin(), title.end(), '\n', ' ');

                    sprintf(game.id, "%s", dirs[i].c_str());
                    sprintf(game.title, "%s", title.c_str());
                    sprintf(game.category, "%s", SFO::GetString(sfo.data(), sfo.size(), "CATEGORY"));
                    sprintf(game.icon_path, "ur0:appmeta/%s/icon0.png", dirs[i].c_str());

                    char line[512];
                    sprintf(line, "%s||%s||%s||%s\n", game.id, game.title, game.category, game.icon_path);
                    FS::Write(fd, line, strlen(line));

                    game.tex = no_icon;
                    games.push_back(game);

                }
            }
            FS::Close(fd);

        }
        else {
            LoadCache();
        }
        qsort(&games[0], games.size(), sizeof(Game), GameComparator);
        max_page = (games.size() + 18 - 1) / 18;
    }

    void Launch(const char *title_id) {
       	char uri[32];
        sprintf(uri, "psgm:play?titleid=%s", title_id);
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelExitProcess(0);
    };

    std::string nextToken(std::vector<char> &buffer, int &nextTokenPos)
    {
        std::string token = "";
        if (nextTokenPos >= buffer.size())
        {
            return NULL;
        }

        for (int i = nextTokenPos; i < buffer.size(); i++)
        {
            if (buffer[i] == '|')
            {
                if ((i+1 < buffer.size()) && (buffer[i+1] == '|'))
                {
                    nextTokenPos = i+2;
                    return token;
                }
                else {
                    token += buffer[i];
                }
            }
            else if (buffer[i] == '\n') {
                nextTokenPos = i+1;
                return token;
            }
            else {
                token += buffer[i];
            }
        }

        return token;
    }

    void LoadCache() {
        std::vector<char> game_buffer = FS::Load(GAME_LIST_FILE);
        int position = 0;
        while (position < game_buffer.size())
        {
            Game game;
            sprintf(game.id, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.title, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.category, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.icon_path, "%s",  nextToken(game_buffer, position).c_str());
            game.tex = no_icon;
            games.push_back(game);
        }
    };

    void LoadGameImages(int prev_page, int page) {
        int del_page = 0;

        if ((page > prev_page) or (prev_page == max_page && page == 1))
        {
            del_page = DecrementPage(page, 10);
        } else if ((page < prev_page) or (prev_page == 1 && page == max_page))
        {
            del_page = IncrementPage(page, 10);
        }

        int high = del_page * 18;
        int low = high - 18;
        if (del_page > 0)
        {
            for (int i=low; (i<high && i < games.size()); i++)
            {
                Game *game = &games[i];
                if (game->tex.id != no_icon.id)
                {
                    Textures::Free(&game->tex);
                    game->tex = no_icon;
                }
            }
        }

        high = page * 18;
        low = high - 18;
        for(std::size_t i = low; (i < high && i < games.size()); i++) {
            Game *game = &games[i];
            if (game->tex.id == no_icon.id && page == page_num)
            {
                LoadGameImage(game);
            }
        }
    }

    int IncrementPage(int page, int num_of_pages)
    {
        int new_page = page + num_of_pages;
        if (new_page > max_page)
        {
            new_page = new_page % max_page;
        }
        return new_page;
    }

    int DecrementPage(int page, int num_of_pages)
    {
        int new_page = page - num_of_pages;
        if (new_page > 0)
        {
            return new_page;
        }
        return new_page + max_page;
    }

    void LoadGameImage(Game *game) {
        Tex tex;
        if (FS::FileExists(game->icon_path)) {
            if (Textures::LoadImageFile(game->icon_path, &tex))
            {
                game->tex = tex;
            }
            else
            {
                game->tex = no_icon;
            }
            
            
        } else {
            game->tex = no_icon;
        }
    }

    void Exit() {
    }

	void StartLoadImagesThread(int prev_page_num, int page)
	{
		LoadImagesParams page_param;
		page_param.prev_page_num = prev_page_num;
		page_param.page_num = page;
		load_images_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::LoadImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (load_images_thid >= 0)
			sceKernelStartThread(load_images_thid, sizeof(LoadImagesParams), &page_param);
	}

    int LoadImagesThread(SceSize args, LoadImagesParams *params) {
        sceKernelDelayThread(300000);
        GAME::LoadGameImages(params->prev_page_num, params->page_num);
        return sceKernelExitDeleteThread(0);
    }

    int GameComparator(const void *v1, const void *v2)
    {
        const Game *p1 = (Game *)v1;
        const Game *p2 = (Game *)v2;
        int p1_len = strlen(p1->title);
        int p2_len = strlen(p2->title);
        int len = p1_len;
        if (p2_len < p1_len)
            len = p2_len;
        return strncmp(p1->title, p2->title, len);
    }
}
