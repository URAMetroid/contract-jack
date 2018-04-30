// ----------------------------------------------------------------------- //
//
// MODULE  : ScreenHostTDMOptions.cpp
//
// PURPOSE : Interface screen for hosting multi player games
//
// (c) 1999-2002 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"
#include "ScreenHostTDMOptions.h"
#include "ScreenMgr.h"
#include "ScreenCommands.h"
#include "GameClientShell.h"
#include "MsgIDs.h"
#include "ResShared.h"

namespace
{
	const int kMaxScoreLimit = 200;
	const int kMaxTimeLimit = 60;
	const int kMaxRunSpeed = 150;
	const int kMaxRounds = 20;

	const uint32 CMD_TEAM1 = CMD_CUSTOM+1;
	const uint32 CMD_TEAM2 = CMD_CUSTOM+2;
}

extern uint8 g_nCurTeam;
extern bool g_bLAN;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenHostTDMOptions::CScreenHostTDMOptions()
{
	m_nMaxPlayers = 8;
	m_nRunSpeed = 130;
	m_nScoreLimit = 25;
	m_nTimeLimit = 10;
	m_nRounds = 1;
	m_bFriendlyFire = LTFALSE;

	m_nFragScore = 1;

	m_pMaxPlayers = NULL;
	m_bWeaponsStay = false;
}


CScreenHostTDMOptions::~CScreenHostTDMOptions()
{
}

// Build the screen
LTBOOL CScreenHostTDMOptions::Build()
{
	int kColumn = g_pLayoutMgr->GetScreenCustomInt(SCREEN_ID_HOST_DM_OPTIONS,"ColumnWidth");
	int kSlider = g_pLayoutMgr->GetScreenCustomInt(SCREEN_ID_HOST_DM_OPTIONS,"SliderWidth");

	CreateTitle(IDS_TITLE_HOST_OPTIONS);

	m_pTeam1 = AddTextItem(IDS_TEAM_1_OPTIONS, CMD_TEAM1, IDS_HELP_TEAM_1);
	m_pTeam2 = AddTextItem(IDS_TEAM_2_OPTIONS, CMD_TEAM2, IDS_HELP_TEAM_2);
/*
	LTIntPt tmp = m_nextPos;

	LTIntPt pos = m_pTeam1->GetBasePos();
	pos.x += kColumn;
	m_pTeam1Name = AddTextItem("<team one>",0,0,pos,LTTRUE);

	pos = m_pTeam2->GetBasePos();
	pos.x += kColumn;
	m_pTeam2Name = AddTextItem("<team two>",0,0,pos,LTTRUE);

	m_nextPos = tmp;
*/	
	m_pMaxPlayers = AddSlider(IDS_MAX_PLAYERS, IDS_MAX_PLAYERS_HELP, kColumn, kSlider, -1, &m_nMaxPlayers);
	m_pMaxPlayers->SetSliderRange(2, 16);
	m_pMaxPlayers->SetSliderIncrement(1);
	m_pMaxPlayers->SetNumericDisplay(LTTRUE);


	char szYes[16];
	char szNo[16];
	FormatString(IDS_YES,szYes,sizeof(szYes));
	FormatString(IDS_NO,szNo,sizeof(szNo));

	CLTGUIToggle* pToggle = AddToggle(IDS_FRIENDLY_FIRE,IDS_FRIENDLY_FIRE_HELP,kColumn,&m_bFriendlyFire);
	pToggle->SetOnString(szYes);
	pToggle->SetOffString(szNo);

	CLTGUISlider*	pSlider = AddSlider(IDS_RUN_SPEED, IDS_RUN_SPEED_HELP, kColumn, kSlider, -1, &m_nRunSpeed);
	pSlider->SetSliderRange(100, kMaxRunSpeed);
	pSlider->SetSliderIncrement(10);
	pSlider->SetNumericDisplay(LTTRUE);

	pSlider = AddSlider(IDS_FRAG_LIMIT, IDS_FRAG_LIMIT_HELP, kColumn, kSlider, -1, &m_nScoreLimit);
	pSlider->SetSliderRange(0,kMaxScoreLimit);
	pSlider->SetSliderIncrement(10);
	pSlider->SetNumericDisplay(LTTRUE);

	pSlider = AddSlider(IDS_TIME_LIMIT, IDS_TIME_LIMIT_HELP, kColumn, kSlider, -1, &m_nTimeLimit);
	pSlider->SetSliderRange(0,kMaxTimeLimit);
	pSlider->SetSliderIncrement(5);
	pSlider->SetNumericDisplay(LTTRUE);

	pSlider = AddSlider( IDS_ROUNDS, IDS_ROUNDS_HELP, kColumn, kSlider, -1, &m_nRounds );
	pSlider->SetSliderRange( 1, kMaxRounds );
	pSlider->SetSliderIncrement( 1 );
	pSlider->SetNumericDisplay( LTTRUE );

//	pSlider = AddSlider(IDS_FRAG_SCORE, IDS_FRAG_SCORE_HELP, kColumn, kSlider, -1, &m_nFragScore);
//	pSlider->SetSliderRange(0,3);
//	pSlider->SetSliderIncrement(1);
//	pSlider->SetNumericDisplay(LTTRUE);

	pToggle = AddToggle(IDS_WEAPONSSTAY, IDS_WEAPONSSTAY_HELP, kColumn,&m_bWeaponsStay);
	pToggle->SetOnString(szYes);
	pToggle->SetOffString(szNo);

 	// Make sure to call the base class
	return CBaseScreen::Build();
}

// Change in focus
void    CScreenHostTDMOptions::OnFocus(LTBOOL bFocus)
{
	CUserProfile *pProfile = g_pProfileMgr->GetCurrentProfile();
		
	if (bFocus)
	{
		m_pMaxPlayers->Enable(!g_pPlayerMgr->IsPlayerInWorld());
		if (g_bLAN)
		{
			m_pMaxPlayers->SetSliderRange(2, 16);
		}
		else
		{
			int nBandwidthMax = pProfile->m_ServerGameOptions.GetMaxPlayersForBandwidth();
			if (nBandwidthMax > 2)
			{
				m_pMaxPlayers->SetSliderRange(2, nBandwidthMax);
			}
			else
			{
				m_pMaxPlayers->SetSliderRange(1, 2);
				m_pMaxPlayers->Enable(LTFALSE);
			}
			m_nMaxPlayers = Min((int)pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nMaxPlayers,nBandwidthMax);
		}

		m_nRunSpeed = (int)pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nRunSpeed;
		m_nScoreLimit = (int)pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nScoreLimit;
		m_nTimeLimit = (int)pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nTimeLimit;
		m_nRounds = (int)pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nRounds;
		m_bFriendlyFire = pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_bFriendlyFire;

		m_bWeaponsStay = pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_bWeaponsStay;

		UpdateData(LTFALSE);

	}
	else
	{
		UpdateData();

		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nMaxPlayers = (uint8)m_nMaxPlayers;
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nRunSpeed = (uint8)m_nRunSpeed;
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nScoreLimit = (uint8)m_nScoreLimit;
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nTimeLimit = (uint8)m_nTimeLimit;
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nRounds = (uint8)m_nRounds;
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_bFriendlyFire = !!m_bFriendlyFire;

//		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_nFragScore = (uint8)m_nFragScore;
		
		pProfile->m_ServerGameOptions.GetTeamDeathmatch().m_bWeaponsStay = !!m_bWeaponsStay;

		pProfile->Save();

		if (g_pPlayerMgr->IsPlayerInWorld())
		{

			CAutoMessage cMsg;
			cMsg.Writeuint8( MID_MULTIPLAYER_OPTIONS );
			pProfile->m_ServerGameOptions.GetTeamDeathmatch().Write(cMsg);
		    g_pLTClient->SendToServer(cMsg.Read(), MESSAGE_GUARANTEED);
		}

	}
	CBaseScreen::OnFocus(bFocus);
}



uint32 CScreenHostTDMOptions::OnCommand(uint32 dwCommand, uint32 dwParam1, uint32 dwParam2)
{
	switch(dwCommand)
	{
	case CMD_TEAM1:
		{
			g_nCurTeam = 0;
			g_pInterfaceMgr->SwitchToScreen(SCREEN_ID_TEAM);
		} break;

	case CMD_TEAM2:
		{
			g_nCurTeam = 1;
			g_pInterfaceMgr->SwitchToScreen(SCREEN_ID_TEAM);
		} break;

	default:
		return CBaseScreen::OnCommand(dwCommand,dwParam1,dwParam2);
	}
	return 1;
};

