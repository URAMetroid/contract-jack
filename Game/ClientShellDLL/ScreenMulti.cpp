// ----------------------------------------------------------------------- //
//
// MODULE  : ScreenMulti.cpp
//
// PURPOSE : Interface screen for hosting and joining multi player games
//
// (c) 1999-2001 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"
#include "ScreenMulti.h"
#include "ScreenMgr.h"
#include "ScreenCommands.h"
#include "GameClientShell.h"
#include "ClientSaveLoadMgr.h"
#include "ClientMultiplayerMgr.h"
#include "msgids.h"
#include "WinUtil.h"
#include "ClientButeMgr.h"
#include "IGameSpy.h"

namespace
{
	const int kMaxCDKeyLength = 24;
	uint16 kWaitWidth = 160;
	uint16 kWaitHeight = 60;


	void EditCDKeyCallBack(LTBOOL bReturn, void *pData)
	{
		CScreenMulti *pThisScreen = (CScreenMulti *)g_pInterfaceMgr->GetScreenMgr()->GetScreenFromID(SCREEN_ID_MULTI);
		if (pThisScreen)
			pThisScreen->SendCommand(bReturn ? CMD_OK : CMD_CANCEL,(uint32)pData,CMD_EDIT_CDKEY);
	}
	void NewVersionCallBack(LTBOOL bReturn, void *pData)
	{
		CScreenMulti *pThisScreen = (CScreenMulti *)g_pInterfaceMgr->GetScreenMgr()->GetScreenFromID(SCREEN_ID_MULTI);
		if (pThisScreen)
			pThisScreen->SendCommand(CMD_OK,NULL,CMD_UPDATE);
	}

	unsigned char CDKeyFilter(unsigned char c)
	{
		return toupper(c);
	}
}


extern bool g_bLAN;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenMulti::CScreenMulti() :
	m_sCurCDKey(""),
	m_sLastValidCDKey(""),
	m_pCDKeyCtrl( NULL )
{
}

CScreenMulti::~CScreenMulti()
{

}

// Build the screen
LTBOOL CScreenMulti::Build()
{

	CreateTitle(IDS_TITLE_MULTI);

	//basic controls
	AddTextItem(IDS_PLAYER_SETUP, CMD_PLAYER, IDS_HELP_PLAYER);

#if !defined( _DEMO )

	m_pCDKeyCtrl = AddColumnCtrl(CMD_EDIT_CDKEY, IDS_HELP_CDKEY);
	m_pCDKeyCtrl->AddColumn(LoadTempString(IDS_CDKEY), 200);
	m_pCDKeyCtrl->AddColumn("  ", 320, LTTRUE);

#endif // !defined( _DEMO )

	m_pJoin = AddTextItem(IDS_JOIN, CMD_JOIN, IDS_HELP_JOIN);
	m_pHost = AddTextItem(IDS_HOST, CMD_HOST, IDS_HELP_HOST);


	m_pUpdate = AddTextItem(IDS_LAUNCH_UPDATE, CMD_UPDATE, IDS_HELP_LAUNCH_UPDATE);


	HTEXTURE hUp = g_pInterfaceResMgr->GetTexture("interface\\menu\\sprtex\\arrowup.dtx");
	HTEXTURE hUpH = g_pInterfaceResMgr->GetTexture("interface\\menu\\sprtex\\arrowup_h.dtx");
	HTEXTURE hDown = g_pInterfaceResMgr->GetTexture("interface\\menu\\sprtex\\arrowdn.dtx");
	HTEXTURE hDownH = g_pInterfaceResMgr->GetTexture("interface\\menu\\sprtex\\arrowdn_h.dtx");

	char szFrame[128];
	g_pLayoutMgr->GetScreenCustomString(SCREEN_ID_MULTI,"FrameTexture",szFrame,sizeof(szFrame));
	HTEXTURE hFrame = g_pInterfaceResMgr->GetTexture(szFrame);

	LTRect rect = g_pLayoutMgr->GetScreenCustomRect(SCREEN_ID_MULTI,"SystemMOTDRect");
	LTIntPt pos(rect.left,rect.top);
	LTIntPt size( (rect.right - rect.left),(rect.bottom - rect.top));
	uint8 nMOTDSize = (uint8)g_pLayoutMgr->GetScreenCustomInt(SCREEN_ID_MULTI,"MessageFontSize");
	uint8 nFont = g_pLayoutMgr->GetScreenFontFace((eScreenID)m_nScreenID);
	CUIFont* pFont = g_pInterfaceResMgr->GetFont(nFont);

	nFont = g_pLayoutMgr->GetDialogFontFace();
	pFont = g_pInterfaceResMgr->GetFont(nFont);
	uint8 nFontSize = g_pLayoutMgr->GetDialogFontSize();

	m_pWaitText = debug_new(CLTGUITextCtrl);
    if (!m_pWaitText->Create(LoadTempString(IDS_INTERNET), NULL, LTNULL, pFont, nFontSize, this))
	{
		debug_delete(m_pWaitText);
        return LTFALSE;
	}
	m_pWaitText->SetColors(argbBlack, argbBlack, argbBlack);
	m_pWaitText->Enable(LTFALSE);

	uint16 w = 16+m_pWaitText->GetBaseWidth();
	uint16 h = 16+m_pWaitText->GetBaseHeight();


	m_pWait = debug_new(CLTGUIWindow);

	char szBack[128] = "";
	g_pLayoutMgr->GetDialogFrame(szBack,sizeof(szBack));

	m_pWait->Create(g_pInterfaceResMgr->GetTexture(szBack),w,h);

	uint16 x = (640-w)/2;
	uint16 y = (480-h)/2;
	m_pWait->SetBasePos(LTIntPt(x,y));
	AddControl(m_pWait);

	
	m_pWait->AddControl(m_pWaitText,LTIntPt(8,8));


	// status text -----------------------------------------------
	pos = g_pLayoutMgr->GetScreenCustomPoint(SCREEN_ID_MULTI,"StatusPos");
	char szTmp[256] = "";
	LoadString(IDS_WAITING,szTmp,sizeof(szTmp));
	m_pStatusCtrl = AddTextItem(FormatTempString(IDS_STATUS_STRING,szTmp), 0, 0, pos, LTTRUE);
	m_pStatusCtrl->SetFont(NULL, nMOTDSize);

 	// Make sure to call the base class
	return CBaseScreen::Build();
}

uint32 CScreenMulti::OnCommand(uint32 dwCommand, uint32 dwParam1, uint32 dwParam2)
{
	if (m_eCurState == eState_Startup || m_eCurState == eState_ValidateCDKey) return 0;
	switch(dwCommand)
	{
	case CMD_UPDATE:
		{
			//LaunchSierraUp();
		} break;

	case CMD_OK:
		{
			return HandleCallback(dwParam1,dwParam2);
		}	break;

	case CMD_PLAYER:
		{
			m_pScreenMgr->SetCurrentScreen(SCREEN_ID_PLAYER);
			break;
		}
	case CMD_JOIN:
		{
			if (m_eCurState == eState_NoCDKey)
			{
				MBCreate mb;
				g_pInterfaceMgr->ShowMessageBox(IDS_CDKEY_INVALID,&mb);
				return 0;
			}
			m_pScreenMgr->SetCurrentScreen(SCREEN_ID_JOIN);
			
			break;
		}

	case CMD_HOST:
		{
			m_pScreenMgr->SetCurrentScreen(SCREEN_ID_HOST);
			break;
		}
	case CMD_EDIT_CDKEY :
		{
			ChangeCDKey();
			break;
		}
	default:
		return CBaseScreen::OnCommand(dwCommand,dwParam1,dwParam2);
	}
	return 1;
};


// Change in focus
void    CScreenMulti::OnFocus(LTBOOL bFocus)
{
	if (bFocus)
	{
		// Make sure we're disconnected from server.
		if(g_pLTClient->IsConnected())
		{
			g_pInterfaceMgr->SetIntentionalDisconnect( true );
			g_pClientMultiplayerMgr->ForceDisconnect();
		}

		if( !g_pMissionButeMgr->Init( MISSION_DM_FILE ))
		{
			g_pLTClient->ShutdownWithMessage("Could not load mission bute %s.", MISSION_DM_FILE );
			return;
		}

		char szCDKey[256];
		g_pVersionMgr->GetCDKey( szCDKey, ARRAY_LEN( szCDKey ));
		m_sCurCDKey = szCDKey;
		m_sLastValidCDKey = m_sCurCDKey;

		m_eCurState = eState_Ready;
		if( m_pCDKeyCtrl )
			m_pCDKeyCtrl->Show(LTTRUE);
		m_pJoin->Enable( m_pCDKeyCtrl ? m_sCurCDKey.length( ) : LTTRUE );
		m_pHost->Enable( m_pCDKeyCtrl ? m_sCurCDKey.length( ) : LTTRUE );
		m_pWait->Show(LTFALSE);
		m_pUpdate->Show(LTFALSE);
		m_pStatusCtrl->Show(LTFALSE);

		UpdateData(LTFALSE);
	}
	else
	{
		UpdateData();
	}

	CBaseScreen::OnFocus(bFocus);
	if (bFocus)
	{
		//since several failure states can drop us here, make sure that we return to main screen on escape
		m_pScreenMgr->AddScreenToHistory( SCREEN_ID_MAIN );

#if !defined( _DEMO )

		// If they don't have a cdkey, then ask them to put one in.
		if( m_sCurCDKey.length( ) == 0 )
		{
			ChangeCDKey( );
		}

#endif // !defined( _DEMO )

	}
}



uint32 CScreenMulti::HandleCallback(uint32 dwParam1, uint32 dwParam2)
{
	switch (dwParam2)
	{
		case CMD_EDIT_CDKEY :
		{
			char newCdKey[kMaxCDKeyLength+1];
			const char* pStr = ( const char* )dwParam1;

			int c = 0;
			int n = 0;
			while (*pStr && c < kMaxCDKeyLength)
			{
				newCdKey[c] = *pStr;
				c++;
				pStr++;
				n++;
				//insert dashes every 4th char as necessary
				if (n == 4 && c < kMaxCDKeyLength)
				{
					n = 0;
					//if there is a dash, copy it and move on
					if (*pStr == '-')
					{
						newCdKey[c] = *pStr;
						c++;
						pStr++;
					}
					else
					{
						newCdKey[c] = '-';
						c++;
					}
				}

			}
			newCdKey[c] = 0;
			
			m_sCurCDKey = newCdKey;
			m_pJoin->Enable( m_pCDKeyCtrl ? m_sCurCDKey.length( ) : LTTRUE );
			m_pHost->Enable( m_pCDKeyCtrl ? m_sCurCDKey.length( ) : LTTRUE );

			g_pVersionMgr->SetCDKey( m_sCurCDKey.c_str( ));

			// Make sure we have our browser.
			if( !g_pClientMultiplayerMgr->GetRetailServerBrowser( ))
			{
				g_pClientMultiplayerMgr->CreateServerBrowsers( );
			}

			IGameSpyBrowser* pBrowser = g_pClientMultiplayerMgr->GetRetailServerBrowser( );
			if( !pBrowser || !pBrowser->IsCDKeyValid( m_sCurCDKey.c_str( )))
			{
				MBCreate mb;
				g_pInterfaceMgr->ShowMessageBox(IDS_CDKEY_INVALID,&mb);

				m_pWait->Show(LTFALSE);
				if (GetCapture() == m_pWait)
					SetCapture(NULL);
				
				m_pCDKeyCtrl->SetString(1,m_sLastValidCDKey.c_str());
				m_pJoin->Enable(!m_sLastValidCDKey.empty());
				m_pHost->Enable(!m_sLastValidCDKey.empty());
				m_sCurCDKey = m_sLastValidCDKey;

				if (m_sLastValidCDKey.empty())
					m_eCurState = eState_NoCDKey;
			}
			else
			{
				m_sLastValidCDKey = m_sCurCDKey;
				g_pVersionMgr->SetCDKey( m_sCurCDKey.c_str( ));
			}

			break;
		}
	}
	return 1;
}

void CScreenMulti::ChangeCDKey()
{
	// Show the CD key change dialog
	MBCreate mb;
	mb.eType = LTMB_EDIT;
	mb.pFn = EditCDKeyCallBack;
	mb.pString = m_sCurCDKey.c_str();
	mb.nMaxChars = kMaxCDKeyLength;
	mb.pFilterFn = CDKeyFilter;
	g_pInterfaceMgr->ShowMessageBox(IDS_CDKEY_ENTER,&mb);
}


LTBOOL CScreenMulti::Render(HSURFACE hDestSurf)
{
	Update();
	return CBaseScreen::Render(hDestSurf);
}

void CScreenMulti::Update()
{
}


void CScreenMulti::Escape()
{
	// Switch save/load to back to use sp folders.
	g_pClientSaveLoadMgr->SetUseMultiplayerFolders( false );

	// Initialize to the sp mission bute.
	if( !g_pMissionButeMgr->Init( MISSION_DEFAULT_FILE ))
	{
		g_pLTClient->ShutdownWithMessage("Could not load mission bute %s.", MISSION_DEFAULT_FILE );
		return;
	}

	g_pClientMultiplayerMgr->SetupServerSinglePlayer( );

	CBaseScreen::Escape();
}
