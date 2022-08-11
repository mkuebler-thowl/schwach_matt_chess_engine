#pragma once

#include <vector>
#include "defines.hpp"

#include "Position.hpp"
#include "Move.hpp"
#include "MinMaxResult.hpp"
#include "ChessValidation.hpp"
#include "RepitionMap.hpp"

namespace owl
{
	// Feature-Parameter
	constexpr UCHAR FT_NULL			= 0;
	constexpr UCHAR FT_ALPHA_BETA	= (1 << 0);
	constexpr UCHAR FT_SORT			= (1 << 1);
	constexpr UCHAR FT_NESTED		= (1 << 2);
	constexpr UCHAR FT_KILLER		= (1 << 3);
	// Optionale-Paramater (nicht implementiert):
	constexpr UCHAR FT_HISTORY		= (1 << 4);
	constexpr UCHAR FT_PVS			= (1 << 5);

	// Spieler
	constexpr short PLAYER_WHITE = 1;
	constexpr short PLAYER_BLACK = -1;

	// Maximalwert
	constexpr FLOAT INF = 999.0f;

	/// Schach-Engine-Klasse. Beinhaltet die Logik einer Schach-Engine, wie die Zugfindung und Bewertung eines Zugs usw.
	class ChessEngine
	{
	public:
		explicit ChessEngine();
		virtual ~ChessEngine();

		// TODO: Threshold f�r Random irgendwie berechnen/festlegen
		/// <summary>
		/// Funktion f�r die Zugfindung aus einer Spielposition heraus
		/// </summary>
		/// <param name="position:">Ausgangsstellung der Schachposition bzw. Spielposition</param>
		/// <param name="player:">Spieler, der als erstes bzw. n�chstes am Zug ist</param>
		/// <param name="depth:">Die maximale Suchtiefe d</param>
		/// <param name="parameterFlags:">F�r Min-Max-Erweiterungen k�nnen Feature-Parameter �ber Bitflags aktiviert werden</param>
		/// <param name="random:">Bei Aktivierung wird aus einer Menge bestm�glicher Z�ge (+-Threshold) ein Zug zuf�llig gew�hlt</param>
		/// <returns>Der bestm�gliche gefundene Zug innerhalb der Suchtiefe</returns>
		Move searchMove(Position& position, short player, UINT16 depth, UCHAR parameter_flags, BOOL random = false);
	private:
		/// <summary>
		/// Klassischer Min-Max-Algorithmus zur Zugfindung
		/// </summary>
		/// <param name="position:">Ausgangsstellung bzw. Spielposition</param>
		/// <param name="player:">Spieler, der als erstes bzw. n�chstes am Zug ist</param>
		/// <param name="depth:">maximale Suchtiefe d</param>
		/// <param name="sort:">Z�ge nach Nutzen sortieren?</param>
		/// <param name="killer">Killer-Heuristik verwenden?</param>
		/// <returns>Nutzwert</returns>
		MinMaxResult minMax(Position& position, short player, UINT16 depth);

		/// <summary>
		/// Alpha-Beta-Suche (Erweiterung des Min-Max)
		/// </summary>
		/// <param name="position">Ausgangsstellung bzw. Spielposition</param>
		/// <param name="player:">Spieler, der als erstes bzw. n�chstes am Zug ist</param>
		/// <param name="depth:">maximale Suchtiefe d</param>
		/// <param name="alpha:">Alpha-Wert</param>
		/// <param name="beta:">Beta-Wert</param>
		/// <param name="sort:">Z�ge nach Nutzen sortieren?</param>
		/// <returns></returns>
		MinMaxResult alphaBeta(Position& position, short player, UINT16 depth, FLOAT alpha, FLOAT beta, BOOL sort = false, BOOL killer = false);
	
		MinMaxResult nested(const Position& position, short player, UINT16 depth, BOOL sort = false);

		MinMaxResult nestedAlphaBeta(const Position& position, short player, UINT16 depth, FLOAT alpha, FLOAT beta, BOOL sort = false);
	public:
		VOID sortMoves(MoveList* moves, const Position& position, UINT16 depth, MinMaxResult* pResult, BOOL killer = false);

		static BOOL killerHeuristics(const Move& move, UINT16 depth, MinMaxResult* pResult);

		enum class Captures
		{
			kxP,kxN,kxB,kxR,kxQ,
			qxP,qxN,qxB,qxR,qxQ,
			rxP,rxN,rxB,rxR,rxQ,
			bxP,bxN,bxB,bxR,bxQ,
			nxP,nxN,nxB,nxR,nxQ,
			pxP,pxN,pxB,pxR,pxQ
		};
	
		static constexpr std::array<CHAR*, 30> s_capture_map =
		{
			"kxP","kxN","kxB","kxR","kxQ",
			"qxP","qxN","qxB","qxR","qxQ",
			"rxP","rxN","rxB","rxR","rxQ",
			"bxP","bxN","bxB","bxR","bxQ",
			"nxP","nxN","nxB","nxR","nxQ",
			"pxP","pxN","pxB","pxR","pxQ"
		};

		static Captures getCaptureValue(CHAR attacker, CHAR victim);
		static std::string getCaptureValueString(Captures capture);
	

		INT32 m_v1 = 0;
		INT32 m_v2 = 0;

		RepitionMap m_repitionMap;
	};
}