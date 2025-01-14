
#include <string.h>
#include "gcard.h"
#include "https.h"
#include "loader/utils.h"
#include "gecko/gecko.hpp"
#include "memory/mem2.hpp"

#define MAX_URL_SIZE 263 // 128 + 129 + 6

struct provider
{
	char url[128];
	char key[129];
};

struct provider *providers = NULL;
int amount_of_providers = 0;

u8 register_card_provider(const char *url, const char *key)
{
	if(strlen(url) > 0 && strlen(key) > 0 && strstr(url, "{KEY}") != NULL && strstr(url, "{ID6}") != NULL)
	{
		providers = (struct provider*)MEM2_realloc(providers, (amount_of_providers + 1) * sizeof(struct provider));
		memset(&providers[amount_of_providers], 0, sizeof(struct provider));
		strncpy(providers[amount_of_providers].url, url, 127);
		strncpy(providers[amount_of_providers].key, key, 128);
		amount_of_providers++;
		gprintf("Gamercard provider is valid!\n");
		return 0;
	}
	gprintf("Gamertag provider is NOT valid!\n");
	return -1;
}

u8 has_enabled_providers()
{
	if (amount_of_providers != 0 && providers != NULL)
		return 1;
	return 0;
}

void add_game_to_card(const char *gameid)
{
	int i;

	char *url = (char *)MEM2_alloc(MAX_URL_SIZE); // Too much memory, but only like 10 bytes
	memset(url, 0, MAX_URL_SIZE);

	for(i = 0; i < amount_of_providers && providers != NULL; i++)
	{
		strcpy(url, providers[i].url);
		str_replace(url, "{KEY}", providers[i].key, MAX_URL_SIZE);
		str_replace(url, "{ID6}", gameid, MAX_URL_SIZE);
		gprintf("Gamertag URL:\n%s\n", url);
		struct download file = {};
		file.skip_response = 1;
		downloadfile(url, &file);
	}
	MEM2_free(url);
}
