#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

#include "basetik_bin.h"

struct ticket_dumb {
	u8 unused1[0x1DC];
	u64 titleID_be;
	u8 unused2[0x16C];
} __attribute__((packed));

void fix_unused_tickets(void) {
	Handle ticketHandle;
	Result res;
	struct ticket_dumb *buf;
	u32 i, j;

	u32 titlesNANDCount, titlesSDMCCount, ticketsCount;
	u32 titlesNANDRead, titlesSDMCRead, ticketsRead;
	u32 titleTotalCount, titleTotalRead;
	u32 missingCount = 0;

	u64 *titles;
	u64 *tickets;
	u64 *missingTickets;

	bool etd_available;
	res = AM_QueryAvailableExternalTitleDatabase(&etd_available);
	if (R_FAILED(res)) {
		printf("Echec de la requete vers la base de donnees des titres externes.\n  AM_QueryAvailableExternalTitleDatabase:\n  0x%08lx", res);
		return;
	}

	if (!etd_available) {
		puts(
			"La base de donnees des titres externes n'est pas disponible.\n"
			"\n"
			"Cela signifie que la 3DS pense qu'il n'y a aucun\n"
			"logiciel installe.\n"
		);
		return;
	}

	// get titles
	res = AM_GetTitleCount(MEDIATYPE_NAND, &titlesNANDCount);
	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTitleCount(MEDIATYPE_NAND): 0x%08lx\n", res);
		return;
	}
	res = AM_GetTitleCount(MEDIATYPE_SD, &titlesSDMCCount);
	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTitleCount(MEDIATYPE_SD): 0x%08lx\n", res);
		return;
	}

	titleTotalCount = titlesNANDCount + titlesSDMCCount;

	titles = calloc(titleTotalCount, sizeof(u64));
	if (!titles) {
		puts("Echec de l'allocation de memoire pour les titres.\nCela ne devrait pas arriver.");
		return;
	}

	res = AM_GetTitleList(&titlesNANDRead, MEDIATYPE_NAND, titlesNANDCount, titles);
	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTitleList(MEDIATYPE_NAND): 0x%08lx\n", res);
		free(titles);
		return;
	}
	res = AM_GetTitleList(&titlesSDMCRead, MEDIATYPE_SD, titlesSDMCCount, titles + titlesNANDRead);
	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTitleList(MEDIATYPE_SD): 0x%08lx\n", res);
		free(titles);
		return;
	}

	titleTotalRead = titlesNANDRead + titlesSDMCRead;

	// get tickets
	res = AM_GetTicketCount(&ticketsCount);
	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTicketCount: 0x%08lx\n", res);
		free(titles);
		return;
	}

	tickets = calloc(ticketsCount, sizeof(u64));
	if (!tickets) {
		puts("Echec de l'allocation de memoire pour les tickets.\nCela ne devrait pas arriver.");
		free(titles);
		return;
	}

	missingTickets = calloc(titleTotalRead, sizeof(u64));
	if (!missingTickets) {
		puts("Echec de l'allocation de memoire pour les tickets manquants.\nCela ne devrait pas arriver");
		free(titles);
		free(tickets);
		return;
	}

	res = AM_GetTicketList(&ticketsRead, ticketsCount, 0, tickets);

	if (R_FAILED(res)) {
		printf("Echec de l'appel AM_GetTicketList: 0x%08lx\n", res);
		free(titles);
		free(tickets);
		free(missingTickets);
		return;
	}

	// get missing tickets
	u64 currentTitle;
	bool found;
	for (i = 0; i < titleTotalRead; i++) {
		currentTitle = titles[i];
		found = false;
		for (j = 0; j < ticketsRead; j++) {
			if (currentTitle == tickets[j]) {
				found = true;
				break;
			}
		}

		if (!found) {
			fputs("", stdout); // this fixes an issue and I have no fucking clue why.
			missingTickets[missingCount] = currentTitle;
			missingCount++;
		}
	}

	free(titles);
	free(tickets);

	if (!missingCount) {
		puts("Pas de tickets manquants.");
		free(missingTickets);
		return;
	}

	// now we install the tickets...
	buf = malloc(basetik_bin_size);
	if (!buf) {
		puts("Echec de l'allocation de memoire pour le tampon de ticket.\nCela ne devrait pas arriver");
		free(missingTickets);
		return;
	}

	memcpy(buf, basetik_bin, basetik_bin_size);

	for (i = 0; i < missingCount; i++) {
		buf->titleID_be = __builtin_bswap64(missingTickets[i]);
		res = AM_InstallTicketBegin(&ticketHandle);
		if (R_FAILED(res)) {
			printf("L'installation du ticket pour %016llx a echoue:\n  AM_InstallTicketBegin: 0x%08lx\n", missingTickets[i], res);
			res = AM_InstallTicketAbort(ticketHandle);
			if (R_FAILED(res)) printf("Echec de l'appel AM_InstallTicketAbort: 0x%08lx\n", res);
			continue;
		}

		res = FSFILE_Write(ticketHandle, NULL, 0, buf, sizeof(struct ticket_dumb), 0);
		if (R_FAILED(res)) {
			printf("L'installation du ticket pour %016llx a echoue:\n  FSFILE_Write: 0x%08lx\n", missingTickets[i], res);
			res = AM_InstallTicketAbort(ticketHandle);
			if (R_FAILED(res)) printf("Echec de l'appel AM_InstallTicketAbort: 0x%08lx\n", res);
			continue;
		}

		res = AM_InstallTicketFinish(ticketHandle);
		if (R_FAILED(res)) {
			printf("L'installation du ticket pour %016llx a echoue:\n  AM_InstallTicketFinish: 0x%08lx\n", missingTickets[i], res);
			res = AM_InstallTicketAbort(ticketHandle);
			if (R_FAILED(res)) printf("Echec de l'appel AM_InstallTicketAbort: 0x%08lx\n", res);
			continue;
		}

		printf("L'installation du ticket pour %016llx.\n", missingTickets[i]);

	}

	free(buf);
	free(missingTickets);
	puts("Titres repares avec succes.");
}

int main(int argc, char* argv[])
{
	amInit();
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	puts("faketik v1.1.2 FR");

	puts("Debut de la reparation des tickets...");
	fix_unused_tickets();

	puts("\nPOURQUOI MON JEU N'EST-IL PAS APPARU ?");
	puts("  Visitez la page ci-dessous pour obtenir de\nl'aide !\n");
	puts("    https://github.com/TheRinzler65/faketik_FR");
	puts("\nAppuyez sur START ou B pour quitter.");

	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START || kDown & KEY_B)
			break;
	}

	gfxExit();
	amExit();
	return 0;
}	