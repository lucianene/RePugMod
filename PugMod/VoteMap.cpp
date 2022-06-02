#include "precompiled.h"

CVoteMap gVoteMap;

void CVoteMap::Load()
{
	this->m_Data.clear();
	this->m_Vote.clear();
}

void CVoteMap::Init()
{
	this->m_Vote.clear();

	this->m_Data = gUtil.LoadMapList(VOTE_MAP_FILE, gCvars.GetVoteMapSelf()->value ? true : false);
	
	CBasePlayer* Players[MAX_CLIENTS] = { NULL };

	auto Num = gPlayer.GetList(Players, true);

	for (int i = 0; i < Num; i++)
	{
		auto Player = Players[i];

		if (Player)
		{
			auto EntityIndex = Player->entindex();

			gMenu[EntityIndex].Create(_T("Vote Map:"), false, (void*)this->MenuHandle);

			gMenu[EntityIndex].AddList(this->m_Data);

			gMenu[EntityIndex].Show(EntityIndex);
		}
	}

	gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, _T("Starting Vote Map."));

	gTask.Create(PUG_TASK_VOTE, gCvars.GetVoteDelay()->value, false, (void*)this->Stop);

	gTask.Create(PUG_TASK_LIST, 0.5f, true, (void*)this->List, this);
}

void CVoteMap::MenuHandle(int EntityIndex, P_MENU_ITEM Item)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		gVoteMap.AddVote(Item.Info, 1);

		gUtil.SayText(NULL, Player->entindex(), _T("\3%s\1 choosed \3%s\1"), STRING(Player->edict()->v.netname), Item.Text.c_str());

		if (gVoteMap.GetCount() >= gPlayer.GetNum(false))
		{
			gVoteMap.Stop();
		}
	}
}


void CVoteMap::Stop()
{
	CBasePlayer* Players[MAX_CLIENTS] = { NULL };

	int Num = gPlayer.GetList(Players, true);

	for (int i = 0; i < Num; i++)
	{
		auto Player = Players[i];

		if (Player)
		{
			if (!Player->IsBot())
			{
				int EntityIndex = Player->entindex();

				gMenu[EntityIndex].Hide(EntityIndex);
			}
		}
	}

	gTask.Remove(PUG_TASK_LIST);

	gTask.Remove(PUG_TASK_VOTE);

	int Winner = gVoteMap.GetWinner();

	if (Winner == -1)
	{
		gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, _T("The map choice has failed: \3No votes."));

		gVoteMap.RandomMap(true);
	}
	else 
	{
		gTask.Create(PUG_TASK_EXEC, 5.0f, false, (void*)SERVER_COMMAND, gUtil.VarArgs("changelevel %s\n", gVoteMap.GetItem(Winner)));

		gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, _T("Changing map to \4%s\1..."), gVoteMap.GetItem(Winner));
	}
}

void CVoteMap::Changelevel(char* MapName)
{
	if (MapName)
	{
		SERVER_COMMAND(gUtil.VarArgs("changelevel %s\n", MapName));
	}
}

void CVoteMap::List(CVoteMap* VoteMap)
{
	std::string VoteList;

	for (std::size_t MapIndex = 0; MapIndex < VoteMap->m_Data.size(); MapIndex++)
	{
		if (VoteMap->m_Vote[MapIndex])
		{
			VoteList += VoteMap->m_Data[MapIndex];
			VoteList += " [";
			VoteList += std::to_string(VoteMap->m_Vote[MapIndex]);
			VoteList += "]\n";
		}
	}

	gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, 0.23, 0.02, 0, 0.0, 0.53, 0.0, 0.0, 1), _T("Choose the map (%d):"),(int)gTask.Timeleft(PUG_TASK_VOTE));
	
	gUtil.HudMessage(NULL, gUtil.HudParam(255, 255, 225, 0.23, 0.02, 0, 0.0, 0.53, 0.0, 0.0, 2), "\n%s", VoteList.length() ? VoteList.c_str() : _T("No votes."));
}

int CVoteMap::GetCount()
{
	int Count = 0;

	for (std::size_t i = 0; i < this->m_Data.size(); i++)
	{
		Count += this->m_Vote[i];
	}

	return Count;
}

int CVoteMap::GetWinner()
{
	int Winner = 0, WinnerVotes = 0;

	for (std::size_t i = 0; i < this->m_Data.size(); i++)
	{
		if (this->m_Vote[i] > WinnerVotes)
		{
			Winner = i;
			WinnerVotes = this->m_Vote[i];
		}
		else if (this->m_Vote[i] == WinnerVotes)
		{
			if (RANDOM_LONG(0, 1))
			{
				Winner = i;
				WinnerVotes = this->m_Vote[i];
			}
		}
	}

	return this->m_Vote[Winner] == 0 ? -1 : Winner;
}

const char* CVoteMap::GetItem(int ItemIndex)
{
	return this->m_Data[ItemIndex].c_str(); 
}

int CVoteMap::RandomMap(bool Change)
{
	std::srand(std::time(0));

	int Random = std::rand() % this->m_Data.size();;

	if (Change)
	{
		gTask.Create(PUG_TASK_EXEC, 5.0f, false, (void*)SERVER_COMMAND, gUtil.VarArgs("changelevel %s\n", this->m_Data[Random].c_str()));

		gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, _T("Changing map to \4%s\1..."), this->m_Data[Random].c_str());
	}

	return Random;
}
