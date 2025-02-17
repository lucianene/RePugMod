#include "precompiled.h"

CKnifeRound gKnifeRound;

void CKnifeRound::Init()
{
	this->m_Running = true;

	memset(this->m_Votes, 0, sizeof(this->m_Votes));

	gUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Knife Round Starting: \4Get Ready!!"));
}

void CKnifeRound::Stop(bool ChangeTeams)
{
	this->m_Running = false;

	if (ChangeTeams)
	{
		if (g_pGameRules)
		{
			auto WinnerTeam = (CSGameRules()->m_iRoundWinStatus == WINSTATUS_CTS) ? CT : TERRORIST;
			auto WinnerVote = (CSGameRules()->m_iRoundWinStatus == WINSTATUS_CTS) ? TERRORIST : CT;

			if (this->GetVote(WinnerVote) > this->GetVote(WinnerTeam))
			{
				CSGameRules()->SwapAllPlayers();

				gUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Changing teams automatically."));
			}
			else
			{
				gUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Teams will remain unchanged."));
			}
		}
	}

	gPugMod.RestarPeriod(NULL);
}

bool CKnifeRound::ClientHasRestrictItem(CBasePlayer* Player, ItemID item, ItemRestType type)
{
	if (this->m_Running)
	{
		if (item != ITEM_KEVLAR && item != ITEM_ASSAULT && item != ITEM_KNIFE)
		{
			return true;
		}
	}

	return false;
}

bool CKnifeRound::IsRunning()
{
	return this->m_Running;
}

void CKnifeRound::StartVote(TeamName Winner)
{
	if (this->m_Running)
	{
		if (Winner != UNASSIGNED)
		{
			memset(this->m_Votes, 0, sizeof(m_Votes));
			
			CBasePlayer* Players[MAX_CLIENTS] = { NULL };

			int Num = gPlayer.GetList(Players, Winner);

			for (int i = 0; i < Num; i++)
			{
				auto Player = Players[i];

				if (Player)
				{
					if (!Player->IsBot())
					{
						auto EntityIndex = Player->entindex();

						gMenu[EntityIndex].Create(_T("Select Starting Side:"), false, (void*)this->MenuHandle);

						gMenu[EntityIndex].AddItem(0, _T("Terrorists"), false, 1);

						gMenu[EntityIndex].AddItem(1, _T("Counter-Terrorists"), false, 2);

						gMenu[EntityIndex].Show(EntityIndex);
					}
				}
			}

			if (g_pGameRules)
			{
				if (CSGameRules()->m_bRoundTerminating)
				{
					CSGameRules()->m_flRestartRoundTime = (gpGlobals->time + gCvars.GetVoteDelay()->value);
				}
			}

			gTask.Create(PUG_TASK_VOTE, gCvars.GetVoteDelay()->value, false, (void*)this->VoteEnd);

			gTask.Create(PUG_TASK_LIST, 0.5f, true, (void*)this->List);

			gUtil.SayText(nullptr, (Winner == TERRORIST) ? PRINT_TEAM_RED : PRINT_TEAM_BLUE, _T("\3%s\1 Won: The \3%s\1 team will decide the starting side."), PUG_MOD_TEAM_STR[Winner], PUG_MOD_TEAM_STR[Winner]);
		}
	}
}

void CKnifeRound::RoundEnd(int winStatus, ScenarioEventEndRound event, float tmDelay)
{
	if (this->m_Running)
	{
		if (winStatus != WINSTATUS_NONE)
		{
			if (winStatus == WINSTATUS_TERRORISTS && event == ROUND_TERRORISTS_WIN)
			{
				this->StartVote(TERRORIST);
			}
			else if (winStatus == WINSTATUS_CTS && event == ROUND_CTS_WIN)
			{
				this->StartVote(CT);
			}
			else
			{
				gUtil.SayText(nullptr, PRINT_TEAM_DEFAULT, _T("Knife Round Failed: \3No clear winner by extermination."));

				if (!gCvars.GetKnifeRoundEndType()->value)
				{
					this->Stop(false);
				}
				else
				{
					this->Init();

					gPugMod.RestarPeriod(nullptr);
				}
			}
		}
	}
}

void CKnifeRound::List()
{
	std::string VoteList;

	if (gKnifeRound.GetVote(TERRORIST))
	{
		VoteList += PUG_MOD_TEAM_STR[TERRORIST];
		VoteList += " [";
		VoteList += std::to_string(gKnifeRound.GetVote(TERRORIST));
		VoteList += "]\n";
	}

	if (gKnifeRound.GetVote(CT))
	{
		VoteList += PUG_MOD_TEAM_STR[CT];
		VoteList += " [";
		VoteList += std::to_string(gKnifeRound.GetVote(CT));
		VoteList += "]\n";
	}

	gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, 0.23, 0.02, 0, 0.0, 0.53, 0.0, 0.0, 1), _T("Starting Side (%d):"), (int)gTask.Timeleft(PUG_TASK_VOTE));

	gUtil.HudMessage(NULL, gUtil.HudParam(255, 255, 225, 0.23, 0.02, 0, 0.0, 0.53, 0.0, 0.0, 2), "\n%s", VoteList.length() ? VoteList.c_str() : _T("No votes..."));
}

int CKnifeRound::AddVote(TeamName Team)
{
	this->m_Votes[Team]++;

	return this->m_Votes[Team];
}

int CKnifeRound::GetVote(TeamName Team)
{
	return this->m_Votes[Team];
}

void CKnifeRound::MenuHandle(int EntityIndex, P_MENU_ITEM Item)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		gKnifeRound.AddVote((TeamName)Item.Extra);

		gUtil.SayText(nullptr, Player->entindex(), _T("\3%s\1 choosed \3%s\1"), STRING(Player->edict()->v.netname), Item.Text.c_str());

		if ((gKnifeRound.GetVote(TERRORIST) + gKnifeRound.GetVote(CT)) >= (gPlayer.GetNum(false) / 2))
		{
			gKnifeRound.VoteEnd();
		}
	}
}

void CKnifeRound::VoteEnd()
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

	gKnifeRound.Stop(true);
}
