#include "precompiled.h"

CPugMod gPugMod;

void CPugMod::Load()
{
	// We start at dead state
	this->SetState(PUG_STATE_DEAD); 

	// After a time, start PUG
	this->NextState(6.0);
}

void CPugMod::Unload()
{
	if (this->m_State >= PUG_STATE_FIRST_HALF && this->m_State <= PUG_STATE_OVERTIME)
	{
		this->SetState(PUG_STATE_END);
	}
}

int CPugMod::GetState()
{
	return this->m_State;
}

void CPugMod::SetState(int State)
{
	this->m_State = State;

	switch (this->m_State)
	{
		case PUG_STATE_WARMUP:
			{
				gReady.Load();

				gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "%s started, get ready!", PUG_MOD_STATES_STR[this->m_State]);
				break;
			}
		case PUG_STATE_START:
			{
				gReady.Unload();

				if (gCvars.GetVoteMap()->value && gCvars.GetVoteMapType()->value)
				{
					gCvars.GetVoteMap()->value = 0;

					if (gCvars.GetVoteMapType()->value == 1)
					{
						gVoteMap.Init();
					}
					else if (gCvars.GetVoteMapType()->value == 2)
					{
						gVoteMap.RandomMap(true);
					}
				}
				else
				{
					if (gCvars.GetVoteTeamType()->value == -1.0f)
					{
						gVoteTeam.Init();
					}
					else
					{
						gVoteTeam.SetMode((int)gCvars.GetVoteTeamType()->value);
					}
				}

				break;
			}
		case PUG_STATE_FIRST_HALF:
			{
				gReady.Unload();

				memset(this->m_Round, 0, sizeof(this->m_Round));

				memset(this->m_Score, 0, sizeof(this->m_Score));

				if (this->CheckBalanceTeams())
				{
					this->LO3(3);

					gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "%s started: \3Good Luck & Have Fun!", PUG_MOD_STATES_STR[this->m_State]); 
				}
				else
				{
					gUtil.SayText(0, PRINT_TEAM_RED, "Failed to start match: \3Not enough players.\1");

					this->SetState(PUG_STATE_WARMUP);
				}

				break;
			}
		case PUG_STATE_HALFTIME:
			{
				if ((int)gCvars.GetShowScoreType()->value == 2)
				{
					CBasePlayer* Players[32] = { NULL };

					int Num = gPlayer.GetList(Players);

					for (int i = 0; i < Num; i++)
					{
						this->m_Frags[i] = Players[i]->edict()->v.frags;
						this->m_Death[i] = Players[i]->m_iDeaths;
					}
				}

				gTask.Create(PUG_TASK_EXEC, 5.0f, false, this->SwapTeams, this);

				if (gPlayer.GetNum() < (int)gCvars.GetPlayersMin()->value)
				{
					gReady.Load();
				}

				gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "%s started, get ready!", PUG_MOD_STATES_STR[this->m_State]);

				break;
			}
		case PUG_STATE_SECOND_HALF:
		case PUG_STATE_OVERTIME:
			{
				gReady.Unload();

				if (this->CheckBalanceTeams())
				{
					this->LO3(3);

					gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "%s started: \3Good Luck & Have Fun!", PUG_MOD_STATES_STR[this->m_State]);
				}
				else
				{
					gUtil.SayText(0, PRINT_TEAM_RED, "Failed to continue match: \3Not enough players.\1");

					this->SetState(PUG_STATE_HALFTIME);
				}

				break;
			}
		case PUG_STATE_END:
			{
				this->ViewScores(NULL);

				gCvars.GetVoteMap()->value = 1;

				gTask.Create(PUG_TASK_EXEC, 5.0f, false, this->RunState, (void*)&gPugMod);

				break;
			}
	}

	if (gCvars.GetConfig(this->m_State))
	{
		if (gCvars.GetConfig(this->m_State)->string)
		{
			gUtil.ServerCommand("exec cstrike/addons/pugmod/cfg/%s\n", gCvars.GetConfig(this->m_State)->string);
		}
	}
}

void CPugMod::NextState(float Delay)
{
	if (Delay)
	{
		gTask.Create(PUG_TASK_NEXT, Delay, false, this->RunState, this);
	}
	else
	{
		this->RunState(this);
	}
}

void CPugMod::RunState(CPugMod* PugMod)
{
	int Next = PUG_STATE_DEAD;

	switch (PugMod->m_State)
	{
		case PUG_STATE_DEAD:
			{
				Next = PUG_STATE_WARMUP;
				break;
			}
		case PUG_STATE_WARMUP:
			{
				Next = PUG_STATE_START;
				break;
			}
		case PUG_STATE_START:
			{
				Next = PUG_STATE_FIRST_HALF;
				break;
			}
		case PUG_STATE_FIRST_HALF:
			{
				Next = PUG_STATE_HALFTIME;
				break;
			}
		case PUG_STATE_HALFTIME:
			{
				if (PugMod->GetRound() < (int)gCvars.GetPlayRounds()->value)
				{
					Next = PUG_STATE_SECOND_HALF;
				}
				else
				{
					Next = PUG_STATE_OVERTIME;
				}

				break;
			}
		case PUG_STATE_SECOND_HALF:
			{
				Next = PUG_STATE_END;

				if (PugMod->GetScores(TERRORIST) == (int)gCvars.GetPlayRounds()->value / 2)
				{
					if (PugMod->GetScores(CT) == (int)gCvars.GetPlayRounds()->value / 2)
					{
						if ((int)gCvars.GetPlayRoundsOvertimeType()->value == 1)
						{
							Next = PUG_STATE_HALFTIME;
						}
					}
				}

				break;
			}
		case PUG_STATE_OVERTIME:
			{
				Next = PUG_STATE_END;

				if ((PugMod->m_Round[PUG_STATE_OVERTIME] % (int)gCvars.GetPlayRoundsOvertime()->value) == 0)
				{
					Next = PUG_STATE_HALFTIME;
				}

				break;
			}
		case PUG_STATE_END:
			{
				Next = PUG_STATE_WARMUP;
				break;
			}
	}

	PugMod->SetState(Next);
}

bool CPugMod::CheckBalanceTeams()
{
	if (gCvars.GetPlayCheckPlayers()->value)
	{
		int MinPlayers = ((int)gCvars.GetPlayersMin()->value / 2);

		int InGame, NumTerrorist, NumCT, NumSpectator;

		gPlayer.GetNum(true, InGame, NumTerrorist, NumCT, NumSpectator);

		if (NumTerrorist < MinPlayers || NumCT < MinPlayers)
		{
			if (g_pGameRules)
			{
				CSGameRules()->BalanceTeams();

				gPlayer.GetNum(true, InGame, NumTerrorist, NumCT, NumSpectator);

				if (NumTerrorist >= MinPlayers && NumCT >= MinPlayers)
				{
					return true;
				}
			}

			return false;
		}
	}

	return true;
}

void CPugMod::StartVoteMap(CBasePlayer* Player)
{
	if (this->m_State != PUG_STATE_DEAD && this->m_State != PUG_STATE_START && this->m_State != PUG_STATE_END)
	{
		gReady.Unload();

		gUtil.SayText(NULL, Player->entindex(), "\3%s\1 started Vote Map.",STRING(Player->edict()->v.netname));

		gVoteMap.Init();
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Cannot start an vote in \3%s\1 state.", PUG_MOD_STATES_STR[this->m_State]);
	}
}

void CPugMod::StartVoteTeam(CBasePlayer* Player)
{
	if (this->m_State == PUG_STATE_WARMUP)
	{
		gCvars.GetVoteMap()->value = 0.0f;

		gCvars.GetVoteTeamType()->value = -1.0f;

		gUtil.SayText(NULL, Player->entindex(), "\3%s\1 started Vote Team.", STRING(Player->edict()->v.netname));

		this->SetState(PUG_STATE_START);
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Cannot start vote team in \3%s\1 state.", PUG_MOD_STATES_STR[this->m_State]);
	}
}

void CPugMod::StartMatch(CBasePlayer* Player)
{
	if (this->m_State == PUG_STATE_WARMUP || this->m_State == PUG_STATE_HALFTIME)
	{
		gUtil.SayText(NULL, Player->entindex(), "\3%s\1 started match.", STRING(Player->edict()->v.netname));

		if (this->m_State == PUG_STATE_HALFTIME)
		{
			this->NextState(1.0);
		}
		else
		{
			this->SetState(PUG_STATE_FIRST_HALF);
		}
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Cannot start match in \3%s\1 state.", PUG_MOD_STATES_STR[this->m_State]);
	}
}

void CPugMod::StopMatch(CBasePlayer* Player)
{
	if (this->m_State >= PUG_STATE_FIRST_HALF && this->m_State <= PUG_STATE_OVERTIME)
	{
		gUtil.SayText(NULL, Player->entindex(), "\3%s\1 stopped match.", STRING(Player->edict()->v.netname));

		this->SetState(PUG_STATE_END);
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Cannot stop match in \3%s\1 state.", PUG_MOD_STATES_STR[this->m_State]);
	}
}

void CPugMod::RestarPeriod(CBasePlayer* Player)
{
	if (this->m_State == PUG_STATE_FIRST_HALF || this->m_State == PUG_STATE_SECOND_HALF || this->m_State == PUG_STATE_OVERTIME)
	{
		gUtil.SayText(NULL, Player->entindex(), "\3%s\1 restarted \4%s\1 period, get ready!.", STRING(Player->edict()->v.netname), PUG_MOD_STATES_STR[this->m_State]);

		this->m_Round[this->m_State] = 0;
		this->m_Score[this->m_State][TERRORIST] = 0;
		this->m_Score[this->m_State][CT] = 0;

		this->SetState(this->m_State);
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Cannot restart period in \3%s\1 state.", PUG_MOD_STATES_STR[this->m_State]);
	}
}

int CPugMod::GetRound()
{
	int Round = 0; 

	for (int i = PUG_STATE_DEAD; i <= PUG_STATE_END; i++)
	{
		Round += this->m_Round[i];
	}

	return Round;
}

int CPugMod::GetScores(int Team)
{
	int Score = 0;

	for (int i = PUG_STATE_DEAD; i <= PUG_STATE_END; i++)
	{
		Score += this->m_Score[i][Team];
	}

	return Score;
}

int CPugMod::GetWinner()
{
	if (this->GetScores(TERRORIST) != this->GetScores(CT))
	{
		return (this->GetScores(TERRORIST) > this->GetScores(CT)) ? TERRORIST : CT;
	}

	return UNASSIGNED;
}

void CPugMod::Help(CBasePlayer * Player)
{
	char HtmlFile[] = "cstrike/addons/pugmod/users_help.htm";

	gUtil.ShowMotd(Player->edict(), HtmlFile, sizeof(HtmlFile));
}

void CPugMod::Status(CBasePlayer* Player)
{
	gUtil.SayText
	(
		Player->edict(),
		PRINT_TEAM_DEFAULT,
		"Status: \4%s\1 (Players %d) (%d Required of %d Allowed)",
		PUG_MOD_STATES_STR[this->m_State],
		gPlayer.GetNum(),
		(int)gCvars.GetPlayersMin()->value,
		(int)gCvars.GetPlayersMax()->value
	);

	if (this->m_State >= PUG_STATE_FIRST_HALF && this->m_State <= PUG_STATE_END)
	{
		this->ViewScores(Player);
	}
}

void CPugMod::Scores(CBasePlayer* Player)
{
	if (this->m_State >= PUG_STATE_FIRST_HALF && this->m_State <= PUG_STATE_END)
	{
		this->ViewScores(Player);
	}
	else
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, "Unable to use this command now.");
	}
}

void CPugMod::ViewScores(CBasePlayer* Player)
{
	int Winner = this->GetWinner();

	if (Winner)
	{
		gUtil.SayText
		(
			Player ? Player->edict() : NULL,
			Winner == TERRORIST ? PRINT_TEAM_RED : PRINT_TEAM_BLUE,
			(this->m_State == PUG_STATE_END) ? "Game Over! The \3%s\1 have won the game: %d-%d" : "The \3%s\1 are winning: %d-%d",
			PUG_MOD_TEAM_STR[Winner],
			this->GetScores(Winner),
			this->GetScores(Winner == TERRORIST ? CT : TERRORIST)
		);
	}
	else
	{
		gUtil.SayText
		(
			Player ? Player->edict() : NULL,
			PRINT_TEAM_DEFAULT,
			(this->m_State == PUG_STATE_END) ? "Game Over! Score is tied: %d-%d" : "Score is tied: %d-%d",
			this->GetScores(TERRORIST),
			this->GetScores(CT)
		);
	}
}

void CPugMod::LO3(int Delay)
{
	if (Delay >= 1 && Delay <= 3) 
	{
		if (g_pGameRules)
		{
			CSGameRules()->m_bGameStarted = true;
		}

		CVAR_SET_FLOAT("sv_restart", (float)Delay);

		gTask.Create(RANDOM_LONG(200, 600), (float)Delay + 1.0f, false, gPugMod.LO3, (void*)(Delay - 1));
	}
	else
	{
		hudtextparms_t Hud = { -1.0f,0.3f,0,0,0xFF,0,0xFF,0,0xFF,0,0xFF,0.0f,0.0f,10.0f,10.0f,0 };

		gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, -1.0, 0.2, 0, 10.0, 10.0), "--- MATCH IS LIVE ---");
	}
}

void CPugMod::SwapTeams(CPugMod* PugMod)
{
	for (int iState = PUG_STATE_FIRST_HALF; iState <= PUG_STATE_OVERTIME; iState++)
	{
		int ScoreTR = PugMod->m_Score[iState][TERRORIST];

		PugMod->m_Score[iState][TERRORIST] = PugMod->m_Score[iState][CT];

		PugMod->m_Score[iState][CT] = ScoreTR;
	}

	if (g_pGameRules)
	{
		CSGameRules()->SwapAllPlayers();
	}

	gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "Changing teams automatically.");

	if ((PugMod->m_State == PUG_STATE_HALFTIME) && (gPlayer.GetNum() >= (int)gCvars.GetPlayersMin()->value))
	{
		PugMod->NextState(2.0f);
	}
}

void CPugMod::ClientConnected(edict_t* pEntity)
{
	if (gPlayer.GetNum() >= (int)gCvars.GetPlayersMax()->value)
	{
		if (!CVAR_GET_FLOAT("allow_spectators"))
		{
			gUtil.ServerCommand("kick #%d Server is full.", GETPLAYERUSERID(pEntity));
		}
	}
}

void CPugMod::ClientGetIntoGame(CBasePlayer* Player)
{
	this->m_Frags[Player->entindex()] = 0;
	this->m_Death[Player->entindex()] = 0;

	gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "%s Build %s (\3%s\1)", Plugin_info.name, Plugin_info.date, Plugin_info.author);
	gUtil.SayText(Player->edict(), PRINT_TEAM_RED, "Say \4.help\1 to view command list.");
}

void CPugMod::ClientDisconnected(edict_t* pEntity)
{
	if (this->m_State >= PUG_STATE_FIRST_HALF && this->m_State <= PUG_STATE_OVERTIME)
	{
		if (gPlayer.GetNum() < (gCvars.GetPlayersMin()->value / 2))
		{
			this->SetState(PUG_STATE_END);
		}
	}
}

bool CPugMod::ClientCommand(CBasePlayer * Player, const char * pcmd, const char * parg1)
{
	if (FStrEq(pcmd, "say") || FStrEq(pcmd, "say_team"))
	{
		if (parg1[0] == '.' || parg1[0] == '!')
		{
			if (CMD_ARGS())
			{
				CLIENT_COMMAND(Player->edict(), "%s\n", CMD_ARGS());
				return true;
			}
		}
	}
	else if (FStrEq(pcmd, "jointeam"))
	{
		if (this->ClientJoinTeam(Player, atoi(parg1)))
		{
			return true;
		}
	}
	else if (FStrEq(pcmd, "menuselect"))
	{
		int Key = atoi(parg1);

		if (Player->m_iMenu == Menu_OFF)
		{
			if (gMenu[Player->entindex()].Handle(Player->entindex(), Key))
			{
				return true;
			}
		}
		else if (Player->m_iMenu == Menu_ChooseTeam || Player->m_iMenu == Menu_IGChooseTeam)
		{
			if (this->ClientJoinTeam(Player, Key))
			{
				return true;
			}
		}
	}
	else if (FStrEq(pcmd, ".help"))
	{
		this->Help(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".status"))
	{
		this->Status(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".score"))
	{
		this->Scores(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".ready"))
	{
		gReady.Ready(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".notready"))
	{
		gReady.NotReady(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".vote"))
	{
		gVoteMenu.Menu(Player);
		return true;
	}
	else if (FStrEq(pcmd, "!menu"))
	{
		gAdmin.Menu(Player);
		return true;
	}
	else if (FStrEq(pcmd, "!msg"))
	{
		gAdmin.Chat(Player, CMD_ARGS());
		return true;
	}
	else if (FStrEq(pcmd, "!rcon"))
	{
		gAdmin.Rcon(Player, CMD_ARGS());
		return true;
	}
	else if (FStrEq(pcmd, ".hp"))
	{
		gStats.HP(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".dmg"))
	{
		gStats.Damage(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".rdmg"))
	{
		gStats.Received(Player);
		return true;
	}
	else if (FStrEq(pcmd, ".sum"))
	{
		gStats.Summary(Player);
		return true;
	}
	return false;
}

bool CPugMod::ClientAddAccount(CBasePlayer * Player, int amount, RewardType type, bool bTrackChange)
{
	if (type == RT_PLAYER_BOUGHT_SOMETHING)
	{
		if (this->m_State == PUG_STATE_WARMUP || this->m_State == PUG_STATE_START || this->m_State == PUG_STATE_HALFTIME)
		{
			return true;
		}
	}

	return false;
}

bool CPugMod::ClientHasRestrictItem(CBasePlayer * Player, ItemID item, ItemRestType type)
{
	if (this->m_State == PUG_STATE_WARMUP || this->m_State == PUG_STATE_START || this->m_State == PUG_STATE_HALFTIME)
	{
		if (type == ITEM_TYPE_BUYING)
		{
			if (item == ITEM_SHIELDGUN || item == ITEM_HEGRENADE || item == ITEM_FLASHBANG || item == ITEM_SMOKEGRENADE)
			{
				gUtil.ClientPrint(Player->edict(), PRINT_CENTER, "#Cstrike_TitlesTXT_Weapon_Not_Available");

				return true;
			}
		}
	}

	return false;
}

bool CPugMod::ClientJoinTeam(CBasePlayer* Player, int NewTeam)
{
	if (Player->m_iTeam == UNASSIGNED)
	{
		gPlayer.TeamInfo(Player->edict(), 33 + 1, "TERRORIST");
		gPlayer.TeamInfo(Player->edict(), 33 + 2, "CT");
	}

	if (NewTeam == 5)
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, "Auto Team Select is not allowed.");
		return true;
	}

	if (Player->m_iTeam == NewTeam)
	{
		gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, "You are already on the \3%s\1 team.", PUG_MOD_TEAM_STR[Player->m_iTeam]);
		return true;
	}

	if (NewTeam == TERRORIST || NewTeam == CT)
	{
		if (gPlayer.GetNum((TeamName)NewTeam) >= ((int)gCvars.GetPlayersMax()->value / 2))
		{
			gUtil.SayText(0, (NewTeam == TERRORIST) ? PRINT_TEAM_RED : PRINT_TEAM_BLUE, "The \3%s\1 team is complete.", PUG_MOD_TEAM_STR[NewTeam]);
			return true;
		}
	}

	if (Player->m_iTeam == UNASSIGNED || Player->m_iTeam == SPECTATOR)
	{
		if (this->m_State == PUG_STATE_START) 
		{
			if (NewTeam != 6)
			{
				gUtil.SayText(Player->edict(), PRINT_TEAM_GREY, "Please, join in \3Spectator\1 while vote session is running.");
				return true;
			}
		}
	}

	if (Player->m_iTeam == TERRORIST || Player->m_iTeam == CT)
	{
		if (this->m_State >= PUG_STATE_START && this->m_State <= PUG_STATE_OVERTIME)
		{
			gUtil.SayText(Player->edict(), PRINT_TEAM_DEFAULT, "Cannot switch when the game is started.");
			return true;
		}
	}

	if (NewTeam == 6)
	{
		if (!CVAR_GET_FLOAT("allow_spectators")) 
		{
			if (!gAdmin.Check(Player->edict()))
			{
				gUtil.SayText(Player->edict(), PRINT_TEAM_GREY, "\3Spectators\1 are not allowed.");
				return true;
			}
		}
	}

	return false;
}

void CPugMod::RoundStart()
{
	if (this->m_State == PUG_STATE_FIRST_HALF || this->m_State == PUG_STATE_SECOND_HALF || this->m_State == PUG_STATE_OVERTIME)
	{
		this->ViewScores(NULL);
	}
}

void CPugMod::RoundEnd(int winStatus, ScenarioEventEndRound event, float tmDelay)
{
	if (this->m_State == PUG_STATE_FIRST_HALF || this->m_State == PUG_STATE_SECOND_HALF || this->m_State == PUG_STATE_OVERTIME)
	{
		if (winStatus == WINSTATUS_TERRORISTS)
		{
			this->m_Round[this->m_State]++;

			this->m_Score[this->m_State][TERRORIST]++;

			gUtil.ClientPrint(NULL, PRINT_CONSOLE, "* Round %d won by: %s", this->GetRound(), PUG_MOD_TEAM_STR[TERRORIST]);
		}
		else if (winStatus == WINSTATUS_CTS)
		{
			this->m_Round[this->m_State]++;

			this->m_Score[this->m_State][CT]++;

			gUtil.ClientPrint(NULL, PRINT_CONSOLE, "* Round %d won by: %s", this->GetRound(), PUG_MOD_TEAM_STR[CT]);
		}
		else if (winStatus == WINSTATUS_DRAW)
		{
			gUtil.SayText(NULL, PRINT_TEAM_DEFAULT, "Round %d draw: No clear winner.", this->GetRound());
		}

		if (this->m_State == PUG_STATE_FIRST_HALF)
		{
			if (this->GetRound() >= ((int)gCvars.GetPlayRounds()->value / 2))
			{
				this->NextState(tmDelay);
			}
		}
		else if (this->m_State == PUG_STATE_SECOND_HALF)
		{
			int Half = (int)(gCvars.GetPlayRounds()->value / 2);

			if ((this->GetScores(TERRORIST) > Half) || (this->GetScores(CT) > Half))
			{
				this->NextState(tmDelay);
			}
			else if ((this->GetScores(TERRORIST) == Half) && (this->GetScores(CT) == Half))
			{
				if (gCvars.GetPlayRoundsOvertimeType()->value == 1)
				{
					this->NextState(tmDelay);
				}
			}
		}
		else if (this->m_State == PUG_STATE_OVERTIME)
		{
			if ((this->GetRound() % (int)gCvars.GetPlayRoundsOvertime()->value) == 0)
			{
				this->NextState(tmDelay);
			}
			else if ((this->m_Score[this->m_State][TERRORIST] - this->m_Score[this->m_State][CT]) > gCvars.GetPlayRoundsOvertime()->value)
			{
				this->NextState(tmDelay);
			}
			else if ((this->m_Score[this->m_State][CT] - this->m_Score[this->m_State][TERRORIST]) > gCvars.GetPlayRoundsOvertime()->value)
			{
				this->NextState(tmDelay);
			}
		}
	}
}

void CPugMod::RoundRestart()
{
	if (g_pGameRules)
	{
		if (this->m_State >= PUG_STATE_HALFTIME && this->m_State <= PUG_STATE_OVERTIME)
		{
			if (gCvars.GetShowScoreType()->value > 0)
			{
				if (!CSGameRules()->m_bCompleteReset)
				{
					CSGameRules()->m_iNumCTWins = this->GetScores(CT);
					CSGameRules()->m_iNumTerroristWins = this->GetScores(TERRORIST);

					CSGameRules()->UpdateTeamScores();

					if (gCvars.GetShowScoreType()->value == 2)
					{
						CBasePlayer* Players[32] = { NULL };

						int Num = gPlayer.GetList(Players);

						for (int i = 0; i < Num; i++)
						{
							if (this->m_Frags[i] || this->m_Death[i])
							{
								Players[i]->m_iDeaths = this->m_Death[i];

								Players[i]->AddPoints(this->m_Frags[i], TRUE);
							}
						}
					}
				}
			}
		}
	}
}