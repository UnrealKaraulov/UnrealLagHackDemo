#define _WIN32_WINNT 0x0501 
#define WINVER 0x0501 
#define NTDDI_VERSION 0x05010000
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <time.h>
#include <vector>
#include <fstream>
#include <TlHelp32.h>

#define IsKeyPressed(CODE) (GetAsyncKeyState(CODE) & 0x8000) > 0

int GameDll = 0;



LPVOID TlsValue;
DWORD TlsIndex;
DWORD _W3XTlsIndex;

DWORD GetIndex( )
{
	return *( DWORD* )( _W3XTlsIndex );
}

DWORD GetW3TlsForIndex( DWORD index )
{
	DWORD pid = GetCurrentProcessId( );
	THREADENTRY32 te32;
	HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, pid );
	te32.dwSize = sizeof( THREADENTRY32 );

	if ( Thread32First( hSnap, &te32 ) )
	{
		do
		{
			if ( te32.th32OwnerProcessID == pid )
			{
				HANDLE hThread = OpenThread( THREAD_ALL_ACCESS, false, te32.th32ThreadID );
				if ( hThread )
				{
					CONTEXT ctx = { CONTEXT_SEGMENTS };
					LDT_ENTRY ldt;
					GetThreadContext( hThread, &ctx );
					GetThreadSelectorEntry( hThread, ctx.SegFs, &ldt );
					DWORD dwThreadBase = ldt.BaseLow | ( ldt.HighWord.Bytes.BaseMid <<
						16u ) | ( ldt.HighWord.Bytes.BaseHi << 24u );
					CloseHandle( hThread );
					if ( dwThreadBase == NULL )
						continue;
					DWORD* dwTLS = *( DWORD** )( dwThreadBase + 0xE10 + 4 * index );
					if ( dwTLS == NULL )
						continue;
					return ( DWORD )dwTLS;
				}
			}
		} while ( Thread32Next( hSnap, &te32 ) );
	}

	return NULL;
}

void SetTlsForMe( )
{
	TlsIndex = GetIndex( );
	LPVOID tls = ( LPVOID )GetW3TlsForIndex( TlsIndex );
	TlsValue = tls;
	TlsSetValue( TlsIndex, TlsValue );
}

/*
BOOL IsGame( ) // my offset + public 126a
{
//return *( int* ) ( ( UINT32 ) GameDll + 0xACF678 ) > 0 || *( int* ) ( ( UINT32 ) GameDll + 0xAB62A4 ) > 0; 126a
}

BOOL IsGame( ) // my offset + public 127a
{
	return ( *( int* ) ( ( UINT32 ) GameDll + 0xBE6530 ) > 0 || *( int* ) ( ( UINT32 ) GameDll + 0xBE6530 ) > 0 )/* && !IsLagScreen( )*/;
/*}



BOOL IsGame( ) // my offset + public 127b
{
	return ( *( int* )( ( UINT32 )GameDll + 0xD6AA98 ) > 0 || *( int* )( ( UINT32 )GameDll + 0xD682D8 ) > 0 )/* && !IsLagScreen( );

}
*/

//BOOL IsGame( ) // my offset + public // 128
//{
//	return ( *( int* )( ( UINT32 )GameDll + 0xD753E0 ) > 0 || *( int* )( ( UINT32 )GameDll + 0xD72C20 ) > 0 )/* && !IsLagScreen( )*/;
//
//}

BOOL IsGame( ) 
{
	return ( *( int* )( ( UINT32 )GameDll + 0xACD0FC ) > 0 || *( int* )( ( UINT32 )GameDll + 0xACECF0 ) > 0 )/* && !IsLagScreen( )*/;
}


struct Packet
{
	DWORD PacketClassPtr;	//+00, some unknown, but needed, Class Pointer
	BYTE* PacketData;		//+04
	DWORD _1;				//+08, zero
	DWORD _2;				//+0C, ??
	DWORD Size;				//+10, size of PacketData
	DWORD _3;				//+14, 0xFFFFFFFF
};


typedef void *( __fastcall * GAME_SendPacket_p ) ( Packet* packet, DWORD zero );
GAME_SendPacket_p GAME_SendPacket;


void SendPacket( BYTE* packetData, DWORD size )
{
	// @warning: this function thread-unsafe, do not use it in other thread.
	// note: this is very useful function, in fact this function
	// does wc3 ingame action, so you can use it for anything you want,
	// including unit commands and and gameplay commands,
	// i suppose its wc3 single action W3GS_INCOMING_ACTION (c) wc3noobpl.

	Packet packet;
	memset( &packet, 0, sizeof( Packet ) );

	//packet.PacketClassPtr = ( DWORD ) ( 0x932D2C + GameDll ); 126a// Packet Class
	//packet.PacketClassPtr = ( DWORD ) ( 0x973210 + GameDll ); // 127a

	//packet.PacketClassPtr = ( DWORD )( 0xAA1220 + GameDll ); // 127b

	//packet.PacketClassPtr = ( DWORD )( 0xAA9DE0 + GameDll );//128a
	packet.PacketClassPtr = ( DWORD )( 0x944E8C + GameDll );
	packet.PacketData = packetData;
	packet.Size = size;
	packet._2 = 0;
	packet._3 = 0xFFFFFFFF;
	GAME_SendPacket( &packet, 0 );
	//4C2160
}


void TryLagHack( int sleep, bool needclear = false )
{
	int ID = 0;
	Sleep( sleep );
	std::vector<BYTE> sendverybadpacketdata;
	for ( int i = 0; i < 15; i++ )
	{
		ID++;
		if ( ID == 0 )
		{
			sendverybadpacketdata.clear( );
			sendverybadpacketdata.push_back( 0x50 );
			sendverybadpacketdata.push_back( i );
			sendverybadpacketdata.push_back( 0xFF );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0xFF );
			SendPacket( &sendverybadpacketdata[ 0 ], sendverybadpacketdata.size( ) );
		}
		else if ( ID == 1 )
		{
			sendverybadpacketdata.clear( );
			sendverybadpacketdata.push_back( 0x50 );
			sendverybadpacketdata.push_back( i );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			SendPacket( &sendverybadpacketdata[ 0 ], sendverybadpacketdata.size( ) );
		}
		else if ( ID == 2 )
		{

			sendverybadpacketdata.clear( );
			sendverybadpacketdata.push_back( 0x50 );
			sendverybadpacketdata.push_back( i );
			sendverybadpacketdata.push_back( 0x3F );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			SendPacket( &sendverybadpacketdata[ 0 ], sendverybadpacketdata.size( ) );
		}
		else if ( ID == 3 )
		{
			sendverybadpacketdata.clear( );
			sendverybadpacketdata.push_back( 0x50 );
			sendverybadpacketdata.push_back( i );
			sendverybadpacketdata.push_back( 0x03 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			sendverybadpacketdata.push_back( 0x00 );
			SendPacket( &sendverybadpacketdata[ 0 ], sendverybadpacketdata.size( ) );
			ID = -1;
		}
	}

	if ( needclear )
	{
		for ( int i = 0; i < 15; i++ )
		{
			sendverybadpacketdata.clear( );
			sendverybadpacketdata.push_back( 0x50 );
			sendverybadpacketdata.push_back( i );
			sendverybadpacketdata.push_back( 0xFF );
			sendverybadpacketdata.push_back( 0xFF );
			sendverybadpacketdata.push_back( 0xFF );
			sendverybadpacketdata.push_back( 0xFF );
			SendPacket( &sendverybadpacketdata[ 0 ], sendverybadpacketdata.size( ) );
		}
	}
}


void StartLaghack( )
{
	if ( IsKeyPressed( VK_F6 ) )
	{
		while ( IsKeyPressed( VK_F6 ) )
		{
			TryLagHack( 9 );
		}
		Sleep( 300 );
		TryLagHack( 10, true );
		Sleep( 700 );
		TryLagHack( 10, true );
	}

	if ( IsKeyPressed( VK_F7 ) )
	{
		while ( IsKeyPressed( VK_F7 ) )
		{
			TryLagHack( 45 );
		}
		Sleep( 300 );
		TryLagHack( 10, true );
		Sleep( 700 );
		TryLagHack( 10, true );
	}

	if ( IsKeyPressed( VK_F8 ) )
	{
		while ( IsKeyPressed( VK_F8 ) )
		{
			TryLagHack( 120 );
		}
		Sleep( 300 );
		TryLagHack( 10, true );
		Sleep( 700 );
		TryLagHack( 10, true );
	}


}

DWORD WINAPI LaghackThr( LPVOID qertqer )
{

	GameDll = ( int ) GetModuleHandle( "Game.dll" );
	/*	GAME_SendPacket = ( GAME_SendPacket_p ) ( GameDll + 0x54D970 );*/ //126a
	/*GAME_SendPacket = ( GAME_SendPacket_p ) ( GameDll + 0x30F1B0 );*/ //127a
	//GAME_SendPacket = ( GAME_SendPacket_p )( GameDll + 0x32C920 );//127b

	//GAME_SendPacket = ( GAME_SendPacket_p )( GameDll + 0x333EA0 );//128a

	GAME_SendPacket = ( GAME_SendPacket_p )( GameDll + 0x54E470 );

	//_W3XTlsIndex = GameDll + 0xD3CB98;//127b
	//_W3XTlsIndex = GameDll + 0xD472C4;//128a
	_W3XTlsIndex = GameDll + 0xACEB54;

	Sleep( 3000 );

	SetTlsForMe( );

	while ( true )
	{
		while ( !IsGame( ) )
		{
			Sleep( 300 );
		}
		try
		{
			StartLaghack( );
		}
		catch ( ... )
		{
			Beep( 600, 1000 );
			SetTlsForMe( );
		}
		Sleep( 50 );
	}
	return 0;
}

HANDLE whid;

BOOL WINAPI DllMain( HINSTANCE hDLL, UINT reason, LPVOID reserved )
{

	if ( reason == DLL_PROCESS_ATTACH )
	{
		char * szPath = new char[ MAX_PATH ];
		srand( ( unsigned int )time( NULL ) );
		whid = CreateThread( 0, 0, LaghackThr, hDLL, 0, 0 );
	}
	else if ( reason == DLL_PROCESS_DETACH )
	{
		TerminateThread( whid, 0 );
	}

	return TRUE;
}

